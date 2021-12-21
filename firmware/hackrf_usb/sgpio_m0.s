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

There are two key code paths, with the following worst-case timings:

RX:             140 cycles
TX:             125 cycles

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
.equ TARGET_BUFFER_MASK,                   0x7fff

// Base address of the state structure.
.equ STATE_BASE,                           0x20007000

// Offsets into the state structure.
.equ OFFSET,                               0x00
.equ TX,                                   0x04

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

state             .req r13
buf_base          .req r12
buf_mask          .req r11
sgpio_data        .req r7
sgpio_int         .req r6
buf_ptr           .req r5

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
	ldr value, =TARGET_DATA_BUFFER                  // value = TARGET_DATA_BUFFER           // 2
	mov buf_base, value                             // buf_base = value                     // 1
	ldr value, =TARGET_BUFFER_MASK                  // value = TARGET_DATA_MASK             // 2
	mov buf_mask, value                             // buf_mask = value                     // 1
	ldr value, =STATE_BASE                          // value = STATE_BASE                   // 2
	mov state, value                                // state = value                        // 1

	// Initialise state.
	zero .req r0
	mov zero, #0                                    // zero = 0                             // 1
	str zero, [state, #OFFSET]                      // state.offset = zero                  // 2
	str zero, [state, #TX]                          // state.tx = zero                      // 2

loop:
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

	int_status .req r0
	scratch .req r1

	// Spin until we're ready to handle an SGPIO packet:
	// Grab the exchange interrupt staus...
	ldr int_status, [sgpio_int, #INT_STATUS]        // int_status = SGPIO_STATUS_1          // 10, twice

	// ... check to see if bit #0 (slice A) was set, by shifting it into the carry bit...
	lsr scratch, int_status, #1                     // scratch = int_status >> 1            // 1, twice

	// ... and if not, jump back to the beginning.
	bcc loop                                        // if !carry: goto loop                 // 3, then 1

	// Clear the interrupt pending bits that were set.
	str int_status, [sgpio_int, #INT_CLEAR]         // SGPIO_CLR_STATUS_1 = int_status      // 8

	// ... and grab the address of the buffer segment we want to write to / read from.
	ldr buf_ptr, [state, #OFFSET]                   // buf_ptr = state.offset               // 2
	add buf_ptr, buf_base                           // buf_ptr += buf_base                  // 1

	tx .req r0

	// Load direction (TX or RX)
	ldr tx, [state, #TX]                            // tx = state.tx                        // 2

	// TX?
	lsr tx, #1                                      // tx >>= 1                             // 1
	bcc direction_rx                                // if !carry: goto direction_rx         // 1 thru, 3 taken

direction_tx:

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

	b done                                          // goto done                            // 3

direction_rx:

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

done:
	offset .req r0

	// Finally, update the buffer location...
	mov offset, buf_mask                            // offset = buf_mask                    // 1
	and offset, buf_ptr                             // offset &= buf_ptr                    // 1

	// ... and store the new position.
	str offset, [state, #OFFSET]                    // state.offset = offset                // 2

	b loop                                          // goto loop                            // 3
