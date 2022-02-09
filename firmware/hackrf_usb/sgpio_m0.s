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

To implement the different functions of HackRF, the M0 operates in one of
five modes, configured by the M4:

IDLE:           Do nothing.
WAIT:           Do nothing, but increment byte counter for timing purposes.
RX:             Read data from SGPIO and write it to the buffer.
TX_START:       Write zeroes to SGPIO until there is data in the buffer.
TX_RUN:         Read data from the buffer and write it to SGPIO.

In all modes except IDLE, the M0 advances a byte counter, which increases by
32 each time that many bytes are exchanged with the buffer (or skipped over,
in WAIT mode).

As the M4 core produces or consumes these bytes, it advances its own counter.
The difference between the two counter values therefore indicates the number
of bytes available.

If the M4 does not advance its count in time, a TX underrun or RX overrun
occurs.  Collectively, these events are referred to as shortfalls, and the
handling is similar for both.

In an RX shortfall, data is discarded. In TX mode, zeroes are written to
SGPIO. When in a shortfall, the byte counter does not advance.

The M0 maintains statistics on the the number of shortfalls, and the length of
the longest shortfall.

The M0 can be configured to abort TX or RX and return to IDLE mode, if the
length of a shortfall exceeds a configured limit.

The M0 can also be configured to switch modes automatically when its byte
counter matches a threshold value. This feature can be used to implement
timed operations.

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

RX, normal:     152 cycles
RX, overrun:    76 cycles
TX, normal:     140 cycles
TX, underrun:   145 cycles

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

Structure
=========

Each mode has its own loop routine. TX_START and TX_RUN use a single TX loop.

Code shared between different modes is implemented in macros and duplicated
within each mode's own loop.

At startup, the main routine sets up registers and memory, then falls through
to the idle loop.

The idle loop waits for a mode to be set, then jumps to that mode's start
label.

Code following the start label is executed only on a transition from IDLE. It
is at this point that the buffer statistics are reset.

Each mode's start code then falls through to its loop label.

The first step in each loop is to wait for an SGPIO interrupt and clear it,
which is implemented by the await_sgpio macro.

Then, the mode setting is loaded from memory. If the M4 has reset the mode to
idle, control jumps back to the idle loop after handling any cleanup needed.

Next, any SGPIO operations are carried out. For RX and TX, this begins with
calculating the buffer margin, and branching if there is a shortfall. Then
the pointer within the buffer is updated.

SGPIO reads and writes are implemented in 16-byte chunks. The four lowest
registers, r0-r3, are used to temporarily hold the data for each chunk. Data
is stored in-order in the buffer, but out-of-order in the SGPIO shadow
registers, due to the SGPIO architecture. A combination of single and
multiple load/stores is used to reorder the data in each chunk.

After completing SGPIO operations, counters are updated and the threshold
setting is checked. If the byte count has reached the threshold, the next
mode is set and a jump is made directly to the corresponding loop label.
Code at the start label of the new mode is not executed, so stats and
counters are maintained across a sequence of TX/RX/WAIT operations.

When a shortfall occurs, a branch is taken to a separate handler routine,
which branches back to the mode's normal loop when complete.

Most of the code for shortfall handling is common to RX and TX, and is
implemented in the handle_shortfall macro. This is primarily concerned with
updating statistics, but also handles switching back to IDLE mode if a
shortfall exceeds the configured limit.

There is a rollback mechanism implemented in the shortfall handling. This is
necessary because it is common for a harmless shortfall to occur during
shutdown, which produces misleading statistics. The code detects this case
when the mode is changed to IDLE whilst a shortfall is ongoing. If this
happens, statistics are rolled back to their values at the beginning of the
shortfall.

The backup of previous values is implemented in handle_shortfall when a new
shortfall is detected, and the rollback is implemented by the
checked_rollback routine. This routine is executed by the TX and RX loops
before returning to the idle loop.

Organisation
============

The rest of this file is organised as follows:

- Constant definitions
- Fixed register allocations
- Macros
- Ordering constraints
- Finally, the actual code!

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
.equ REQUESTED_MODE,                       0x00
.equ ACTIVE_MODE,                          0x04
.equ M0_COUNT,                             0x08
.equ M4_COUNT,                             0x0C
.equ NUM_SHORTFALLS,                       0x10
.equ LONGEST_SHORTFALL,                    0x14
.equ SHORTFALL_LIMIT,                      0x18
.equ THRESHOLD,                            0x1C
.equ NEXT_MODE,                            0x20
.equ ERROR,                                0x24

// Private variables stored after state.
.equ PREV_LONGEST_SHORTFALL,               0x28

// Operating modes.
.equ MODE_IDLE,                            0
.equ MODE_WAIT,                            1
.equ MODE_RX,                              2
.equ MODE_TX_START,                        3
.equ MODE_TX_RUN,                          4

// Error codes.
.equ ERROR_NONE,                           0
.equ ERROR_RX_TIMEOUT,                     1
.equ ERROR_TX_TIMEOUT,                     2

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

.macro on_request label
	// Check if a new mode change request was made, and if so jump to the given label.
	mode .req r3
	flag .req r2
	ldr mode, [state, #REQUESTED_MODE]              // mode = state.requested_mode          // 2
	lsr flag, mode, #16                             // flag = mode >> 16                    // 1
	bne \label                                      // if flag != 0: goto label             // 1 thru, 3 taken
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

.macro jump_next_mode name
	// Jump to next mode if the byte count threshold has been reached.
	//
	// Clobbers:
	threshold .req r0
	new_mode .req r1

	// Check count against threshold. If not a match, return to start of current loop.
	ldr threshold, [state, #THRESHOLD]              // threshold = state.threshold          // 2
	cmp count, threshold                            // if count != threshold:               // 1
	bne \name\()_loop                               //      goto loop                       // 1 thru, 3 taken

	// Otherwise, load and set new mode.
	ldr new_mode, [state, #NEXT_MODE]               // new_mode = state.next_mode           // 2
	str new_mode, [state, #ACTIVE_MODE]             // state.active_mode = new_mode         // 2

	// Branch according to new mode.
	cmp new_mode, #MODE_RX                          // if new_mode == RX:                   // 1
	beq rx_loop                                     //      goto rx_loop                    // 1 thru, 3 taken
	bgt tx_loop                                     // elif new_mode > RX: goto tx_loop     // 1 thru, 3 taken
	cmp new_mode, #MODE_WAIT                        // if new_mode == WAIT:                 // 1
	beq wait_loop                                   //      goto wait_loop                  // 1 thru, 3 taken
	b idle                                          // goto idle                            // 3
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

	// If so, reset mode to idle and return to idle loop, logging an error.
	//
	// Modes are mapped to errors as follows:
	//
	// MODE_RX (2)     -> ERROR_RX_TIMEOUT (1)
	// MODE_TX_RUN (4) -> ERROR_TX_TIMEOUT (2)
	//
	// As such, the error code can be obtained by shifting the mode right by 1 bit.

	mode .req r3
	error .req r2
	ldr mode, [state, #ACTIVE_MODE]                 // mode = state.active_mode             // 2
	lsr error, mode, #1                             // error = mode >> 1                    // 1
	str error, [state, #ERROR]                      // state.error = error                  // 2
	mov mode, #MODE_IDLE                            // mode = MODE_IDLE                     // 1
	str mode, [state, #ACTIVE_MODE]                 // state.active_mode = mode             // 2
	b idle                                          // goto idle                            // 3
.endm

/*

Ordering constraints
====================

The following routines are in an unusual order, to preserve the ability to
use PC-relative conditional branches between them ("b<cond> label"). The
ordering has been chosen to ensure that all routines are close enough to each
other for the limited range of these instructions (−256 bytes to +254 bytes).

The ordering of routines, and which others each needs to be able to reach, is
as follows:

Routine:                Uses conditional branches to:

idle                    tx_loop, wait_loop
tx_zeros                tx_loop
checked_rollback        idle
tx_loop                 tx_zeros, checked_rollback, rx_loop, wait_loop
wait_loop               rx_loop, tx_loop
rx_loop                 rx_shortfall, checked_rollback, tx_loop, wait_loop
rx_shortfall            rx_loop

If any of these routines are reordered, or made longer, you may get an error
from the assembler saying that a branch is out of range.

*/

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
	str zero, [state, #REQUESTED_MODE]              // state.requested_mode = zero          // 2
	str zero, [state, #ACTIVE_MODE]                 // state.active_mode = zero             // 2
	str zero, [state, #M0_COUNT]                    // state.m0_count = zero                // 2
	str zero, [state, #M4_COUNT]                    // state.m4_count = zero                // 2
	str zero, [state, #NUM_SHORTFALLS]              // state.num_shortfalls = zero          // 2
	str zero, [state, #LONGEST_SHORTFALL]           // state.longest_shortfall = zero       // 2
	str zero, [state, #SHORTFALL_LIMIT]             // state.shortfall_limit = zero         // 2
	str zero, [state, #THRESHOLD]                   // state.threshold = zero               // 2
	str zero, [state, #NEXT_MODE]                   // state.next_mode = zero               // 2
	str zero, [state, #ERROR]                       // state.error = zero                   // 2

idle:
	// Wait for a mode to be requested, then set up the new mode and acknowledge the request.
	mode .req r3
	flag .req r2
	zero .req r0

	// Read the requested mode and check flag to see if this is a new request. If not, ignore.
	ldr mode, [state, #REQUESTED_MODE]              // mode = state.requested_mode          // 2
	lsr flag, mode, #16                             // flag = mode >> 16                    // 1
	beq idle                                        // if flag == 0: goto idle              // 1 thru, 3 taken

	// Otherwise, this is a new request. The M4 is blocked at this point,
	// waiting for us to clear the request flag. So we can safely write to
	// all parts of the state.

	// Set the new mode as both active & next.
	uxth mode, mode                                 // mode = mode & 0xFFFF                 // 1
	str mode, [state, #ACTIVE_MODE]                 // state.active_mode = mode             // 2
	str mode, [state, #NEXT_MODE]                   // state.next_mode = mode               // 2

	// Don't reset counts on a transition to IDLE.
	cmp mode, #MODE_IDLE                            // if mode == IDLE:                     // 1
	beq ack_request                                 //     goto ack_request                 // 1 thru, 3 taken

	// For all other transitions, reset counts.
	mov zero, #0                                    // zero = 0                             // 1
	str zero, [state, #M0_COUNT]                    // state.m0_count = zero                // 2
	str zero, [state, #M4_COUNT]                    // state.m4_count = zero                // 2
	str zero, [state, #NUM_SHORTFALLS]              // state.num_shortfalls = zero          // 2
	str zero, [state, #LONGEST_SHORTFALL]           // state.longest_shortfall = zero       // 2
	str zero, [state, #THRESHOLD]                   // state.threshold = zero               // 2
	str zero, [state, #PREV_LONGEST_SHORTFALL]      // prev_longest_shortfall = zero        // 2
	str zero, [state, #ERROR]                       // state.error = zero                   // 2
	mov shortfall_length, zero                      // shortfall_length = zero              // 1
	mov count, zero                                 // count = zero                         // 1

ack_request:
	// Clear SGPIO interrupt flag, which the M4 set to get our attention.
	str flag, [sgpio_int, #INT_CLEAR]               // SGPIO_CLR_STATUS_1 = flag            // 8

	// Write back requested mode with the flag cleared to acknowledge the request.
	str mode, [state, #REQUESTED_MODE]              // state.requested_mode = mode          // 2

	// Dispatch to appropriate loop.
	//
	// This code is arranged such that the branch to rx_loop is the
	// unconditional one - which is necessary since it's too far away to
	// use a conditional branch instruction.
	cmp mode, #MODE_WAIT                            // if mode < WAIT:                      // 1
	blt idle                                        //      goto idle                       // 1 thru, 3 taken
	beq wait_loop                                   // elif mode == WAIT: goto wait_loop    // 1 thru, 3 taken
	cmp mode, #MODE_RX                              // if mode > RX:                        // 1
	bgt tx_loop                                     //      goto tx_loop                    // 1 thru, 3 taken
	b rx_loop                                       // goto rx_loop                         // 3

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
	ldr mode, [state, #ACTIVE_MODE]                 // mode = state.active_mode             // 2
	cmp mode, #MODE_TX_START                        // if mode == TX_START:                 // 1
	beq tx_loop                                     //      goto tx_loop                    // 1 thru, 3 taken

	// Run common shortfall handling and jump back to TX loop start.
	handle_shortfall tx                             // handle_shortfall()                   // 24

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

tx_loop:

	// Wait for and clear SGPIO interrupt.
	await_sgpio tx                                  // await_sgpio()                        // 34

	// Check if there is a mode change request.
	// If so, we may need to roll back shortfall stats.
	on_request checked_rollback                                                             // 4

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
	// Set active mode to TX_RUN (it might still be TX_START).
	mov mode, #MODE_TX_RUN                          // mode = TX_RUN                        // 1
	str mode, [state, #ACTIVE_MODE]                 // state.active_mode = mode             // 2

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

	// Jump to next mode if threshold reached, or back to TX loop start.
	jump_next_mode tx                               // jump_next_mode()                     // 13

wait_loop:

	// Wait for and clear SGPIO interrupt.
	await_sgpio wait                                // await_sgpio()                        // 34

	// Check if there is a mode change request.
	// If so, return to idle.
	on_request idle                                                                         // 4

	// Update counts.
	update_counts                                   // update_counts()                      // 4

	// Jump to next mode if threshold reached, or back to wait loop start.
	jump_next_mode wait                             // jump_next_mode()                     // 15

rx_loop:

	// Wait for and clear SGPIO interrupt.
	await_sgpio rx                                  // await_sgpio()                        // 34

	// Check if there is a mode change request.
	// If so, we may need to roll back shortfall stats.
	on_request checked_rollback                                                             // 4

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

	// Jump to next mode if threshold reached, or back to RX loop start.
	jump_next_mode rx                               // jump_next_mode()                     // 12

rx_shortfall:

	// Run common shortfall handling and jump back to RX loop.
	handle_shortfall rx                             // handle_shortfall()                   // 24

// The linker will put a literal pool here, so add a label for clearer objdump output:
constants:
