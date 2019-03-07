/*
 * Copyright 2019 Jared Boone <jared@sharebrained.com>
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

#ifndef __CPLD_XC2C_H__
#define __CPLD_XC2C_H__

#include <stdint.h>
#include <stdbool.h>

#include "cpld_jtag.h"

/* Xilinx CoolRunner II XC2C64A bitstream attributes */
#define CPLD_XC2C64A_ROWS (98)
#define CPLD_XC2C64A_BITS_IN_ROW (274)
#define CPLD_XC2C64A_BYTES_IN_ROW ((CPLD_XC2C64A_BITS_IN_ROW + 7) / 8)

typedef struct {
	uint8_t data[CPLD_XC2C64A_BYTES_IN_ROW];
} cpld_xc2c64a_row_data_t;

typedef struct {
	cpld_xc2c64a_row_data_t row[CPLD_XC2C64A_ROWS];
} cpld_xc2c64a_program_t;

typedef struct {
	uint8_t value[CPLD_XC2C64A_BYTES_IN_ROW];
} cpld_xc2c64a_row_mask_t;

typedef struct {
	cpld_xc2c64a_row_mask_t mask[6];
	uint8_t mask_index[CPLD_XC2C64A_ROWS];
} cpld_xc2c64a_verify_t;

bool cpld_xc2c64a_jtag_checksum(
	const jtag_t* const jtag,
	const cpld_xc2c64a_verify_t* const verify,
	uint32_t* const crc_value
);
void cpld_xc2c64a_jtag_sram_write(
	const jtag_t* const jtag,
	const cpld_xc2c64a_program_t* const program
);
bool cpld_xc2c64a_jtag_sram_verify(
	const jtag_t* const jtag,
	const cpld_xc2c64a_program_t* const program,
	const cpld_xc2c64a_verify_t* const verify
);

extern const cpld_xc2c64a_program_t cpld_hackrf_program_sram;
extern const cpld_xc2c64a_verify_t cpld_hackrf_verify;

#endif/*__CPLD_XC2C_H__*/
