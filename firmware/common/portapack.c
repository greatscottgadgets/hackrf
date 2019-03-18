/*
 * Copyright 2018 Jared Boone
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

#include "portapack.h"
#include "hackrf_core.h"

static bool jtag_pp_tck(const bool tms_value) {
	gpio_write(jtag_cpld.gpio->gpio_pp_tms, tms_value);

	// 8 ns TMS/TDI to TCK setup
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");

	gpio_set(jtag_cpld.gpio->gpio_tck);

	// 15 ns TCK to TMS/TDI hold time
	// 20 ns TCK high time
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");

	gpio_clear(jtag_cpld.gpio->gpio_tck);

	// 20 ns TCK low time
	// 25 ns TCK falling edge to TDO valid
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");

	return gpio_read(jtag_cpld.gpio->gpio_pp_tdo);
}

static uint32_t jtag_pp_shift(const uint32_t tms_bits, const size_t count) {
	uint32_t result = 0;
	size_t bit_in_index = count - 1;
	size_t bit_out_index = 0;
	while(bit_out_index < count) {
		const uint32_t tdo = jtag_pp_tck((tms_bits >> bit_in_index) & 1) & 1;
		result |= (tdo << bit_out_index);

		bit_in_index--;
		bit_out_index++;
	}

	return result;
}

static uint32_t jtag_pp_idcode(void) {
	cpld_jtag_take(&jtag_cpld);

	/* TODO: Check if PortaPack TMS is floating or driven by an external device. */
	gpio_output(jtag_cpld.gpio->gpio_pp_tms);

	/* Test-Logic/Reset -> Run-Test/Idle -> Select-DR/Scan -> Capture-DR */
	jtag_pp_shift(0b11111010, 8);

	/* Shift-DR */
	const uint32_t idcode = jtag_pp_shift(0, 32);

	/* Exit1-DR -> Update-DR -> Run-Test/Idle -> ... -> Test-Logic/Reset */
	jtag_pp_shift(0b11011111, 8);

	cpld_jtag_release(&jtag_cpld);

	return idcode;
}

static bool portapack_detect(void) {
	return jtag_pp_idcode() == 0x020A50DD;
}

extern const hackrf_ui_t portapack_hackrf_ui;

const portapack_t portapack = {
	&portapack_hackrf_ui,
};

const portapack_t* portapack_init(void) {
	if( portapack_detect() ) {
		return &portapack;
	} else {
		return NULL;
	}
}