/*
 * Copyright 2019-2022 Great Scott Gadgets
 *
 * This file is part of HackRF.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/*

Introduction
============

This file contains the code that runs on the Cortex-M0 core of the LPC43xx.

The M0 core is used to implement all the timing-critical usage of the SGPIO
peripheral, which interfaces to the MAX5864 ADC/DAC via the CPLD.

The M0 reads or writes 32 bytes at a time from the SGPIO registers,
transferring these bytes to or from a shared USB bulk buffer. The M4 core
handles transferring data between this buffer and the USB host.

The SGPIO peripheral is set up and enabled by the M4 core. All the M0 needs to
do is handle the SGPIO exchange interrupt, which indicates that new data can
now be read from or written to the SGPIO shadow registers.

Timing
======

This code has tight timing constraints.

We have to complete a read or write from SGPIO every 163 cycles.

The CPU clock is 204MHz. We exchange 32 bytes at a time in the SGPIO
registers, which is 16 samples worth of IQ data. At the maximum sample rate of
20MHz, the SGPIO update rate is 20 / 16 = 1.25MHz. So we have 204 / 1.25 =
163.2 cycles available.

Access to the SGPIO peripheral is slow, due to the asynchronous bridge that
connects it to the AHB bus matrix. Section 20.4.1 of the LPC43xx user manual
(UM10503) specifies the access latencies as:

Read:  4 x MCLK + 4 x CLK_PERIPH_SGPIO
Write: 4 x MCLK + 2 x CLK_PERIPH_SGPIO

In our case both these clocks are at 204MHz so reads add 8 cycles and writes
add 6. These are latencies that add to the usual M0 instruction timings, so an
ldr from SGPIO takes 10 cycles, and an str to SGPIO takes 8 cycles.

These latencies are assumed to apply to all accesses to the SGPIO peripheral's
address space, which includes its interrupt control registers as well as the
shadow registers.

There are four key code paths, with the following worst-case timings:

RX, normal:     143 cycles
RX, overrun:    73 cycles
TX, normal:     132 cycles
TX, underrun:   140 cycles

Design
======

Due to the timing constraints, this code is highly optimised.

This is the only code that runs on the M0, so it does not need to follow
calling conventions, nor use features of the architecture in standard ways.

The SGPIO handling does not run as an ISR. It polls the interrupt status.
This saves the cycle costs of interrupt entry and exit, and allows all
registers to be used freely.

All possible registers, including the stack pointer and link register, can be
used to store values needed in the code, to minimise memory loads and stores.

There are no function calls. There is no stack usage. All values are in
registers and fixed memory addresses.

*/

// Constants that point to registers we'll need to modify in the SGPIO block.
.equ SGPIO_SHADOW_REGISTERS_BASE,          0x40101100
.equ SGPIO_EXCHANGE_INTERRUPT_BASE,        0x40101F00

// Offsets into the interrupt control registers.
.equ INT_CLEAR,                            0x30
.equ INT_STATUS,                           0x2C

// Buffer that we're funneling data to/from.
.equ TARGET_DATA_BUFFER,                   0x20008000
.equ TARGET_BUFFER_SIZE,                   0x8000
.equ TARGET_BUFFER_MASK,                   0x7fff

// Base address of the state structure.
.equ STATE_BASE,                           0x20007000

// Offsets into the state structure.
.equ MODE,                                 0x00
.equ M0_COUNT,                             0x04
.equ M4_COUNT,                             0x08
.equ NUM_SHORTFALLS,                       0x0C
.equ LONGEST_SHORTFALL,                    0x10
.equ SHORTFALL_LIMIT,                      0x14

// Private variables stored after state.
.equ PREV_LONGEST_SHORTFALL,               0x20

// Operating modes.
.equ MODE_IDLE,                            0
.equ MODE_RX,                              1
.equ MODE_TX_START,                        2
.equ MODE_TX_RUN,                          3

// Our slice chain is set up as follows (ascending data age; arrows are reversed for flow):
//     L  -> F  -> K  -> C -> J  -> E  -> I  -> A
// Which has equivalent shadow register offsets:
//     44 -> 20 -> 40 -> 8 -> 36 -> 16 -> 32 -> 0
.equ SLICE0,                               44
.equ SLICE1,                               20
.equ SLICE2,                               40
.equ SLICE3,                               8
.equ SLICE4,                               36
.equ SLICE5,                               16
.equ SLICE6,                               32
.equ SLICE7,                               0

/* Allocations of single-use registers */

buf_size_minus_32 .req r14
state             .req r13
buf_base          .req r12
buf_mask          .req r11
shortfall_length  .req r10
hi_zero           .req r9
sgpio_data        .req r7
sgpio_int         .req r6
count             .req r5
buf_ptr           .req r4

/* Macros */

.macro reset_counts
	// Reset counts.
	//
	// Clobbers:
	zero .req r0

	mov zero, #0                                    // zero = 0                             // 1
	str zero, [state, #M0_COUNT]                    // state.m0_count = zero                // 2
	str zero, [state, #M4_COUNT]                    // state.m4_count = zero                // 2
	str zero, [state, #NUM_SHORTFALLS]              // state.num_shortfalls = zero          // 2
	str zero, [state, #LONGEST_SHORTFALL]           // state.longest_shortfall = zero       // 2
	str zero, [state, #PREV_LONGEST_SHORTFALL]      // prev_longest_shortfall = zero        // 2
	mov shortfall_length, zero                      // shortfall_length = zero              // 1
	mov count, zero                                 // count = zero                         // 1
.endm

.macro await_sgpio name
	// Wait for, then clear, SGPIO exchange interrupt flag.
	//
	// Clobbers:
	int_status .req r0
	scratch .req r1

	// The worst case timing is assumed to occur when reading the interrupt
	// status register *just* misses the flag being set - so we include the
	// cycles required to check it a second time.
	//
	// We also assume that we can spend a full 10 cycles doing an ldr from
	// SGPIO the first time (2 for ldr, plus 8 for SGPIO-AHB bus latency),
	// and still miss a flag that was set at the start of those 10 cycles.
	//
	// This latter asssumption is probably slightly pessimistic, since the
	// sampling of the flag on the SGPIO side must occur some time after
	// the ldr instruction begins executing on the M0. However, we avoid
	// relying on any assumptions about the timing details of a read over
	// the SGPIO to AHB bridge.

\name\()_int_wait:
	// Spin on the exchange interrupt status, shifting the slice A flag to the carry flag.
	ldr int_status, [sgpio_int, #INT_STATUS]        // int_status = SGPIO_STATUS_1          // 10, twice
	lsr scratch, int_status, #1                     // scratch = int_status >> 1            // 1, twice
	bcc \name\()_int_wait                           // if !carry: goto int_wait             // 3, then 1

	// Clear the interrupt pending bits that were set.
	str int_status, [sgpio_int, #INT_CLEAR]         // SGPIO_CLR_STATUS_1 = int_status      // 8
.endm

.macro update_buf_ptr
	// Update the address of the buffer segment we want to write to / read from.
	mov buf_ptr, buf_mask                           // buf_ptr = buf_mask                   // 1
	and buf_ptr, count                              // buf_ptr &= count                     // 1
	add buf_ptr, buf_base                           // buf_ptr += buf_base                  // 1
.endm

.macro update_counts
	// Update counts after successful SGPIO operation.

	// Update the byte count and store the new value.
	add count, #32                                  // count += 32                          // 1
	str count, [state, #M0_COUNT]                   // state.m0_count = count               // 2

	// We didn't have a shortfall, so the current shortfall length is zero.
	mov shortfall_length, hi_zero                   // shortfall_length = hi_zero           // 1
.endm

.macro handle_shortfall name
	// Handle a shortfall.
	//
	// Clobbers:
	length .req r0
	num .req r1
	longest .req r1
	limit .req r1

	// Get current shortfall length from high register.
	mov length, shortfall_length                    // length = shortfall_length            // 1

	// Is this a new shortfall?
	cmp length, #0                                  // if length > 0:                       // 1
	bgt \name\()_extend_shortfall                   //      goto extend_shortfall           // 1 thru, 3 taken

	// If so, increase the shortfall count.
	ldr num, [state, #NUM_SHORTFALLS]               // num = state.num_shortfalls           // 2
	add num, #1                                     // num += 1                             // 1
	str num, [state, #NUM_SHORTFALLS]               // state.num_shortfalls = num           // 2

	// Back up previous longest shortfall.
	prev .req r0
	ldr prev, [state, #LONGEST_SHORTFALL]           // prev = state.longest_shortfall       // 2
	str prev, [state, #PREV_LONGEST_SHORTFALL]      // prev_longest_shortfall = prev        // 2

\name\()_extend_shortfall:

	// Extend the length of the current shortfall, and store back in high register.
	add length, #32                                 // length += 32                         // 1
	mov shortfall_length, length                    // shortfall_length = length            // 1

	// Is this now the longest shortfall?
	ldr longest, [state, #LONGEST_SHORTFALL]        // longest = state.longest_shortfall    // 2
	cmp length, longest                             // if length <= longest:                // 1
	blt \name\()_loop                               //      goto loop                       // 1 thru, 3 taken
	str length, [state, #LONGEST_SHORTFALL]         // state.longest_shortfall = length     // 2

	// Is this shortfall long enough to trigger a timeout?
	ldr limit, [state, #SHORTFALL_LIMIT]            // limit = state.shortfall_limit        // 2
	cmp limit, #0                                   // if limit == 0:                       // 1
	beq \name\()_loop                               //      goto loop                       // 1 thru, 3 taken
	cmp length, limit                               // if length < limit:                   // 1
	blt \name\()_loop                               //      goto loop                       // 1 thru, 3 taken

	// If so, reset mode to idle and return to idle loop.
	mode .req r3
	mov mode, #MODE_IDLE                            // mode = MODE_IDLE                     // 1
	str mode, [state, #MODE]                        // state.mode = mode                    // 2
	b idle                                          // goto idle                            // 3
.endm

// Entry point. At this point, the libopencm3 startup code has set things up as
// normal; .data and .bss are initialised, the stack is set up, etc.  However,
// we don't actually use any of that.  All the code in this file would work
// fine if the M0 jumped straight to main at reset.
.global main
.thumb_func
main:                                                                                           // Cycle counts:
	// Initialise registers used for constant values.
	value .req r0
	ldr sgpio_int, =SGPIO_EXCHANGE_INTERRUPT_BASE   // sgpio_int = SGPIO_INT_BASE           // 2
	ldr sgpio_data, =SGPIO_SHADOW_REGISTERS_BASE    // sgpio_data = SGPIO_REG_SS            // 2
	ldr value, =(TARGET_BUFFER_SIZE - 32)           // value = TARGET_BUFFER_SIZE - 32      // 2
	mov buf_size_minus_32, value                    // buf_size_minus_32 = value            // 1
	ldr value, =TARGET_DATA_BUFFER                  // value = TARGET_DATA_BUFFER           // 2
	mov buf_base, value                             // buf_base = value                     // 1
	ldr value, =TARGET_BUFFER_MASK                  // value = TARGET_DATA_MASK             // 2
	mov buf_mask, value                             // buf_mask = value                     // 1
	ldr value, =STATE_BASE                          // value = STATE_BASE                   // 2
	mov state, value                                // state = value                        // 1
	zero .req r0
	mov zero, #0                                    // zero = 0                             // 1
	mov hi_zero, zero                               // hi_zero = zero                       // 1

	// Initialise state.
	str zero, [state, #MODE]                        // state.mode = zero                    // 2
	str zero, [state, #M0_COUNT]                    // state.m0_count = zero                // 2
	str zero, [state, #M4_COUNT]                    // state.m4_count = zero                // 2
	str zero, [state, #NUM_SHORTFALLS]              // state.num_shortfalls = zero          // 2
	str zero, [state, #LONGEST_SHORTFALL]           // state.longest_shortfall = zero       // 2
	str zero, [state, #SHORTFALL_LIMIT]             // state.shortfall_limit = zero         // 2

idle:
	// Wait for RX or TX mode to be set.
	mode .req r3
	ldr mode, [state, #MODE]                        // mode = state.mode                    // 2
	cmp mode, #MODE_RX                              // if mode == RX:                       // 1
	beq rx_start                                    //      goto rx_start                   // 1 thru, 3 taken
	bgt tx_start                                    // elif mode > RX: goto tx_start        // 1 thru, 3 taken
	b idle                                          // goto idle                            // 3

checked_rollback:
	// Checked rollback handler. This code is run when the M0 is in a TX or RX mode, and is
	// placed back into IDLE mode by the M4. If there is an ongoing shortfall at this point,
	// it is assumed to be a shutdown artifact and rolled back.

	// If there is no ongoing shortfall, there's nothing to do - jump back to idle loop.
	length .req r0
	mov length, shortfall_length                    // length = shortfall_length            // 1
	cmp length, #0                                  // if length == 0:                      // 1
	beq idle                                        //      goto idle                       // 3

	// Otherwise, roll back the state to ignore the current shortfall, then jump to idle.
	prev .req r0
	ldr prev, [state, #PREV_LONGEST_SHORTFALL]      // prev = prev_longest_shortfall        // 2
	str prev, [state, #LONGEST_SHORTFALL]           // state.longest_shortfall = prev       // 2
	ldr prev, [state, #NUM_SHORTFALLS]              // prev = num_shortfalls                // 2
	sub prev, #1                                    // prev -= 1                            // 1
	str prev, [state, #NUM_SHORTFALLS]              // state.num_shortfalls = prev          // 2

	b idle                                          // goto idle                            // 3

tx_start:

	// Reset counts.
	reset_counts                                    // reset_counts()                       // 10

tx_loop:

	// Wait for and clear SGPIO interrupt.
	await_sgpio tx                                  // await_sgpio()                        // 34

	// Check if a return to idle mode was requested.
	// If so, we may need to roll back shortfall stats.
	mode .req r3
	ldr mode, [state, #MODE]                        // mode = state.mode                    // 2
	cmp mode, #MODE_IDLE                            // if mode == IDLE:                     // 1
	beq checked_rollback                            //      goto checked_rollback           // 1 thru, 3 taken

	// Check if there is enough data in the buffer.
	//
	// The number of bytes in the buffer is given by (m4_count - m0_count).
	// We need 32 bytes available to proceed. So our margin, which we want
	// to be positive or zero, is:
	//
	// buf_margin = m4_count - m0_count - 32
	//
	// If there is insufficient data, transmit zeros instead.
	buf_margin .req r0
	ldr buf_margin, [state, #M4_COUNT]              // buf_margin = m4_count                // 2
	sub buf_margin, count                           // buf_margin -= count                  // 1
	sub buf_margin, #32                             // buf_margin -= 32                     // 1
	bmi tx_zeros                                    // if buf_margin < 0: goto tx_zeros     // 1 thru, 3 taken

	// Update buffer pointer.
	update_buf_ptr                                  // update_buf_ptr()                     // 3

	// At this point we know there is TX data available.
	// If still in TX start mode, switch to TX run.
	cmp mode, #MODE_TX_START                        // if mode != TX_START:                 // 1
	bne tx_write                                    //      goto tx_write                   // 1 thru, 3 taken
	mov mode, #MODE_TX_RUN                          // mode = TX_RUN                        // 1
	str mode, [state, #MODE]                        // state.mode = mode                    // 2

tx_write:

	// Write data to SGPIO.
	ldm buf_ptr!, {r0-r3}                           // r0-r3 = buf_ptr[0:16]; buf_ptr += 16 // 5
	str r0, [sgpio_data, #SLICE0]                   // SGPIO_REG_SS[SLICE0] = r0            // 8
	str r1, [sgpio_data, #SLICE1]                   // SGPIO_REG_SS[SLICE1] = r1            // 8
	str r2, [sgpio_data, #SLICE2]                   // SGPIO_REG_SS[SLICE2] = r2            // 8
	str r3, [sgpio_data, #SLICE3]                   // SGPIO_REG_SS[SLICE3] = r3            // 8
	ldm buf_ptr!, {r0-r3}                           // r0-r3 = buf_ptr[0:16]; buf_ptr += 16 // 5
	str r0, [sgpio_data, #SLICE4]                   // SGPIO_REG_SS[SLICE4] = r0            // 8
	str r1, [sgpio_data, #SLICE5]                   // SGPIO_REG_SS[SLICE5] = r1            // 8
	str r2, [sgpio_data, #SLICE6]                   // SGPIO_REG_SS[SLICE6] = r2            // 8
	str r3, [sgpio_data, #SLICE7]                   // SGPIO_REG_SS[SLICE7] = r3            // 8

	// Update counts.
	update_counts                                   // update_counts()                      // 4

	// Jump back to TX loop start.
	b tx_loop                                       // goto tx_loop                         // 3

tx_zeros:

	// Write zeros to SGPIO.
	mov zero, #0                                    // zero = 0                             // 1
	str zero, [sgpio_data, #SLICE0]                 // SGPIO_REG_SS[SLICE0] = zero          // 8
	str zero, [sgpio_data, #SLICE1]                 // SGPIO_REG_SS[SLICE1] = zero          // 8
	str zero, [sgpio_data, #SLICE2]                 // SGPIO_REG_SS[SLICE2] = zero          // 8
	str zero, [sgpio_data, #SLICE3]                 // SGPIO_REG_SS[SLICE3] = zero          // 8
	str zero, [sgpio_data, #SLICE4]                 // SGPIO_REG_SS[SLICE4] = zero          // 8
	str zero, [sgpio_data, #SLICE5]                 // SGPIO_REG_SS[SLICE5] = zero          // 8
	str zero, [sgpio_data, #SLICE6]                 // SGPIO_REG_SS[SLICE6] = zero          // 8
	str zero, [sgpio_data, #SLICE7]                 // SGPIO_REG_SS[SLICE7] = zero          // 8

	// If in TX start mode, don't count this as a shortfall.
	cmp mode, #MODE_TX_START                        // if mode == TX_START:                 // 1
	beq tx_loop                                     //      goto tx_loop                    // 1 thru, 3 taken

	// Run common shortfall handling and jump back to TX loop start.
	handle_shortfall tx                             // handle_shortfall()                   // 24

rx_start:

	// Reset counts.
	reset_counts                                    // reset_counts()                       // 10

rx_loop:

	// Wait for and clear SGPIO interrupt.
	await_sgpio rx                                  // await_sgpio()                        // 34

	// Check if a return to idle mode was requested.
	// If so, we may need to roll back shortfall stats.
	mode .req r3
	ldr mode, [state, #MODE]                        // mode = state.mode                    // 2
	cmp mode, #MODE_IDLE                            // if mode == IDLE:                     // 1
	beq checked_rollback                            //      goto checked_rollback           // 1 thru, 3 taken

	// Check if there is enough space in the buffer.
	//
	// The number of bytes in the buffer is given by (m0_count - m4_count).
	// We need space for another 32 bytes to proceed. So our margin, which
	// we want to be positive or zero, is:
	//
	// buf_margin = buf_size - (m0_count - state.m4_count) - 32
	//
	// which can be rearranged for efficiency as:
	//
	// buf_margin = m4_count + (buf_size - 32) - m0_count
	//
	// If there is insufficient space, jump to shortfall handling.
	buf_margin .req r0
	ldr buf_margin, [state, #M4_COUNT]              // buf_margin = state.m4_count          // 2
	add buf_margin, buf_size_minus_32               // buf_margin += buf_size_minus_32      // 1
	sub buf_margin, count                           // buf_margin -= count                  // 1
	bmi rx_shortfall                                // if buf_margin < 0: goto rx_shortfall // 1 thru, 3 taken

	// Update buffer pointer.
	update_buf_ptr                                  // update_buf_ptr()                     // 3

	// Read data from SGPIO.
	ldr r0, [sgpio_data, #SLICE0]                   // r0 = SGPIO_REG_SS[SLICE0]            // 10
	ldr r1, [sgpio_data, #SLICE1]                   // r1 = SGPIO_REG_SS[SLICE1]            // 10
	ldr r2, [sgpio_data, #SLICE2]                   // r2 = SGPIO_REG_SS[SLICE2]            // 10
	ldr r3, [sgpio_data, #SLICE3]                   // r3 = SGPIO_REG_SS[SLICE3]            // 10
	stm buf_ptr!, {r0-r3}                           // buf_ptr[0:16] = r0-r3; buf_ptr += 16 // 5
	ldr r0, [sgpio_data, #SLICE4]                   // r0 = SGPIO_REG_SS[SLICE4]            // 10
	ldr r1, [sgpio_data, #SLICE5]                   // r1 = SGPIO_REG_SS[SLICE5]            // 10
	ldr r2, [sgpio_data, #SLICE6]                   // r2 = SGPIO_REG_SS[SLICE6]            // 10
	ldr r3, [sgpio_data, #SLICE7]                   // r3 = SGPIO_REG_SS[SLICE7]            // 10
	stm buf_ptr!, {r0-r3}                           // buf_ptr[0:16] = r0-r3; buf_ptr += 16 // 5

	// Update counts.
	update_counts                                   // update_counts()                      // 4

	// Jump back to RX loop start.
	b rx_loop                                       // goto rx_loop                         // 3

rx_shortfall:

	// Run common shortfall handling and jump back to RX loop.
	handle_shortfall rx                             // handle_shortfall()                   // 24

// The linker will put a literal pool here, so add a label for clearer objdump output:
constants:
