/*
 * Copyright 2026 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include "delay.h"

void delay(uint32_t duration)
{
	uint32_t i;

	for (i = 0; i < duration; i++) {
		__asm__("nop");
	}
}

void delay_us_at_mhz(uint32_t us, uint32_t mhz)
{
#if defined(LPC43XX_M4)
	// The loop below takes 3 cycles per iteration.
	uint32_t loop_iterations = (us * mhz) / 3;
	asm volatile("start%=:\n"
		     "    subs %[ITERATIONS], #1\n" // 1 cycle
		     "    bpl start%=\n"            // 2 cycles
		     :
		     : [ITERATIONS] "r"(loop_iterations));
#elif defined(LPC43XX_M0)
	// The loop below takes 4 cycles per iteration.
	uint32_t loop_iterations = (us * mhz) / 4;
	asm volatile("start%=:\n"
		     "    sub %[ITERATIONS], #1\n" // 1 cycle
		     "    bpl start%=\n"           // 3 cycles
		     :
		     : [ITERATIONS] "r"(loop_iterations));
#else
	#error "No delay loop implementation"
#endif
}
