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

#include "cpld_xc2c.h"

#include "crc.h"

#include <stddef.h>
#include <string.h>

static const uint8_t cpld_xc2c64a_row_address[CPLD_XC2C64A_ROWS] = {
	0x00, 0x40, 0x60, 0x20, 0x30, 0x70, 0x50, 0x10, 0x18, 0x58, 0x78, 0x38, 0x28, 0x68, 0x48, 0x08,
	0x0c, 0x4c, 0x6c, 0x2c, 0x3c, 0x7c, 0x5c, 0x1c, 0x14, 0x54, 0x74, 0x34, 0x24, 0x64, 0x44, 0x04,
	0x06, 0x46, 0x66, 0x26, 0x36, 0x76, 0x56, 0x16, 0x1e, 0x5e, 0x7e, 0x3e, 0x2e, 0x6e, 0x4e, 0x0e,
	0x0a, 0x4a, 0x6a, 0x2a, 0x3a, 0x7a, 0x5a, 0x1a, 0x12, 0x52, 0x72, 0x32, 0x22, 0x62, 0x42, 0x02,
	0x03, 0x43, 0x63, 0x23, 0x33, 0x73, 0x53, 0x13, 0x1b, 0x5b, 0x7b, 0x3b, 0x2b, 0x6b, 0x4b, 0x0b,
	0x0f, 0x4f, 0x6f, 0x2f, 0x3f, 0x7f, 0x5f, 0x1f, 0x17, 0x57, 0x77, 0x37, 0x27, 0x67, 0x47, 0x07,
	0x05, 0x45,
};

typedef enum {
	CPLD_XC2C_IR_INTEST           = 0b00000010,
	CPLD_XC2C_IR_BYPASS           = 0b11111111,
	CPLD_XC2C_IR_SAMPLE           = 0b00000011,
	CPLD_XC2C_IR_EXTEST           = 0b00000000,
	CPLD_XC2C_IR_IDCODE           = 0b00000001,
	CPLD_XC2C_IR_USERCODE         = 0b11111101,
	CPLD_XC2C_IR_HIGHZ            = 0b11111100,
	CPLD_XC2C_IR_ISC_ENABLE_CLAMP = 0b11101001,
	CPLD_XC2C_IR_ISC_ENABLE_OTF   = 0b11100100,
	CPLD_XC2C_IR_ISC_ENABLE       = 0b11101000,
	CPLD_XC2C_IR_ISC_SRAM_READ    = 0b11100111,
	CPLD_XC2C_IR_ISC_WRITE        = 0b11100110,
	CPLD_XC2C_IR_ISC_ERASE        = 0b11101101,
	CPLD_XC2C_IR_ISC_PROGRAM      = 0b11101010,
	CPLD_XC2C_IR_ISC_READ         = 0b11101110,
	CPLD_XC2C_IR_ISC_INIT         = 0b11110000,
	CPLD_XC2C_IR_ISC_DISABLE      = 0b11000000,
	CPLD_XC2C_IR_TEST_ENABLE      = 0b00010001,
	CPLD_XC2C_IR_BULKPROG         = 0b00010010,
	CPLD_XC2C_IR_ERASE_ALL        = 0b00010100,
	CPLD_XC2C_IR_MVERIFY          = 0b00010011,
	CPLD_XC2C_IR_TEST_DISABLE     = 0b00010101,
	CPLD_XC2C_IR_STCTEST          = 0b00010110,
	CPLD_XC2C_IR_ISC_NOOP         = 0b11100000,
} cpld_xc2c_ir_t;

static bool cpld_xc2c_jtag_clock(const jtag_t* const jtag, const uint32_t tms, const uint32_t tdi) {
	// 8 ns TMS/TDI to TCK setup
	gpio_write(jtag->gpio->gpio_tdi, tdi);
	gpio_write(jtag->gpio->gpio_tms, tms);

	// 20 ns TCK high time
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");

	gpio_clear(jtag->gpio->gpio_tck);

	// 25 ns TCK falling edge to TDO valid
	// 20 ns TCK low time
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	
	gpio_set(jtag->gpio->gpio_tck);

	// 15 ns TCK to TMS/TDI hold time
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");

	return gpio_read(jtag->gpio->gpio_tdo);
}

static void cpld_xc2c_jtag_shift_ptr_tms(const jtag_t* const jtag, uint8_t* const tdi_tdo, const size_t start, const size_t end, const bool tms) {
	for(size_t i=start; i<end; i++) {
		const size_t byte_n = i >> 3;
		const size_t bit_n = i & 7;
		const uint32_t mask = (1U << bit_n);

		const uint32_t tdo = cpld_xc2c_jtag_clock(jtag, tms, tdi_tdo[byte_n] & mask) ? 1 : 0;

		tdi_tdo[byte_n] &= ~mask;
		tdi_tdo[byte_n] |= (tdo << bit_n);
	}
}

static void cpld_xc2c_jtag_shift_ptr(const jtag_t* const jtag, uint8_t* const tdi_tdo, const size_t count) {
	if( count > 0 ) {
		cpld_xc2c_jtag_shift_ptr_tms(jtag, tdi_tdo, 0, count - 1, false);
		cpld_xc2c_jtag_shift_ptr_tms(jtag, tdi_tdo, count - 1, count, true);
	}
}

static uint32_t cpld_xc2c_jtag_shift_u32(const jtag_t* const jtag, const uint32_t tms, const uint32_t tdi, const size_t count) {
	uint32_t tdo = 0;

	for(size_t i=0; i<count; i++) {
		const uint32_t mask = (1U << i);
		tdo |= cpld_xc2c_jtag_clock(jtag, tms & mask, tdi & mask) << i;
	}

	return tdo;
}

static void cpld_xc2c_jtag_clocks(const jtag_t* const jtag, const size_t count) {
	for(size_t i=0; i<count; i++) {
		cpld_xc2c_jtag_clock(jtag, 0, 0);
	}
}

static void cpld_xc2c_jtag_pause(const jtag_t* const jtag, const size_t count) {
	for(size_t i=0; i<count; i++) {
		cpld_xc2c_jtag_clock(jtag, (i == (count - 1)), 0);
	}
}

static void cpld_xc2c_jtag_shift_dr_ir(const jtag_t* const jtag, uint8_t* const tdi_tdo, const size_t bit_count, const size_t pause_count) {
	/* Run-Test/Idle or Select-DR-Scan -> Shift-DR or Shift-IR */
	cpld_xc2c_jtag_shift_u32(jtag, 0b001, 0b000, 3);
	/* Shift-[DI]R -> Exit1-[DI]R */
	cpld_xc2c_jtag_shift_ptr(jtag, tdi_tdo, bit_count);
	if( pause_count ) {
		/* Exit1-[DI]R -> Pause-[DI]R */
		cpld_xc2c_jtag_shift_u32(jtag, 0b0, 0, 1);
		/* Pause-[DI]R -> Exit2-[DI]R */
		cpld_xc2c_jtag_pause(jtag, pause_count);
	}
	/* Exit1-[DI]R or Exit2-[DI]R -> Run-Test/Idle */
	cpld_xc2c_jtag_shift_u32(jtag, 0b01, 0, 2);
}

static void cpld_xc2c_jtag_shift_dr(const jtag_t* const jtag, uint8_t* const tdi_tdo, const size_t bit_count, const size_t pause_count) {
	cpld_xc2c_jtag_shift_dr_ir(jtag, tdi_tdo, bit_count, pause_count);
}

static uint8_t cpld_xc2c_jtag_shift_ir_pause(const jtag_t* const jtag, const cpld_xc2c_ir_t ir, const size_t pause_count) {
	/* Run-Test/Idle -> Select-DR-Scan */
	cpld_xc2c_jtag_shift_u32(jtag, 0b1, 0b0, 1);
	uint8_t value = ir;
	cpld_xc2c_jtag_shift_dr_ir(jtag, &value, 8, pause_count);
	return value;
}

static uint8_t cpld_xc2c_jtag_shift_ir(const jtag_t* const jtag, const cpld_xc2c_ir_t ir) {
	return cpld_xc2c_jtag_shift_ir_pause(jtag, ir, 0);
}

static void cpld_xc2c_jtag_reset(const jtag_t* const jtag) {
	/* Five TMS=1 to reach Test-Logic-Reset from any point in the TAP state diagram.
	 */
	cpld_xc2c_jtag_shift_u32(jtag, 0b11111, 0, 5);
}

static void cpld_xc2c_jtag_reset_and_idle(const jtag_t* const jtag) {
	/* Five TMS=1 to reach Test-Logic-Reset from any point in the TAP state diagram.
	 * One TMS=0 to move from Test-Logic-Reset to Run-Test-Idle.
	 */
	cpld_xc2c_jtag_reset(jtag);
	cpld_xc2c_jtag_shift_u32(jtag, 0, 0, 1);
}

static uint32_t cpld_xc2c_jtag_idcode(const jtag_t* const jtag) {
	/* Enter and end at Run-Test-Idle state. */
	cpld_xc2c_jtag_shift_ir(jtag, CPLD_XC2C_IR_IDCODE);
	uint32_t result = 0;
	cpld_xc2c_jtag_shift_dr(jtag, (uint8_t*)&result, 32, 0);
	return result;
}

static bool cpld_xc2c64a_jtag_idcode_ok(const jtag_t* const jtag) {
	return ((cpld_xc2c_jtag_idcode(jtag) ^ 0xf6e5f093) & 0x0fff8fff) == 0;
}

static void cpld_xc2c_jtag_conld(const jtag_t* const jtag) {
	cpld_xc2c_jtag_shift_ir(jtag, CPLD_XC2C_IR_ISC_DISABLE);
	cpld_xc2c_jtag_clocks(jtag, 100);
}

static void cpld_xc2c_jtag_enable(const jtag_t* const jtag) {
	cpld_xc2c_jtag_shift_ir(jtag, CPLD_XC2C_IR_ISC_ENABLE);
	cpld_xc2c_jtag_clocks(jtag, 800);
}

static void cpld_xc2c_jtag_disable(const jtag_t* const jtag) {
	cpld_xc2c_jtag_shift_ir(jtag, CPLD_XC2C_IR_ISC_DISABLE);
	cpld_xc2c_jtag_clocks(jtag, 100);
}

static void cpld_xc2c_jtag_sram_write(const jtag_t* const jtag) {
	cpld_xc2c_jtag_shift_ir(jtag, CPLD_XC2C_IR_ISC_WRITE);
}

static void cpld_xc2c_jtag_sram_read(const jtag_t* const jtag) {
	cpld_xc2c_jtag_shift_ir(jtag, CPLD_XC2C_IR_ISC_SRAM_READ);
}

static uint32_t cpld_xc2c_jtag_bypass(const jtag_t* const jtag, const bool shift_dr) {
	const uint8_t result = cpld_xc2c_jtag_shift_ir(jtag, CPLD_XC2C_IR_BYPASS);
	if( shift_dr ) {
		uint8_t dr = 0;
		cpld_xc2c_jtag_shift_dr(jtag, &dr, 1, 0);
	}
	return result;
}

static bool cpld_xc2c_jtag_read_write_protect(const jtag_t* const jtag) {
	/* Enter and end at Run-Test-Idle state. */
	return ((cpld_xc2c_jtag_bypass(jtag, false) ^ 0x01) & 0x03) == 0;
}

static bool cpld_xc2c_jtag_is_done(const jtag_t* const jtag) {
	return ((cpld_xc2c_jtag_bypass(jtag, false) ^ 0x05) & 0x07) == 0;
}

static void cpld_xc2c_jtag_init_special(const jtag_t* const jtag) {
	cpld_xc2c_jtag_shift_ir(jtag, CPLD_XC2C_IR_ISC_INIT);
	cpld_xc2c_jtag_clocks(jtag, 20);
	/* Run-Test/Idle -> Shift-IR */
	cpld_xc2c_jtag_shift_u32(jtag, 0b0011, 0b0000, 4);
	/* Shift-IR: 0xf0 -> Exit1-IR */
	cpld_xc2c_jtag_shift_u32(jtag, 0x80, CPLD_XC2C_IR_ISC_INIT, 8);
	/* Exit1-IR -> Pause-IR */
	cpld_xc2c_jtag_shift_u32(jtag, 0b0, 0, 1);
	/* Pause-IR -> Exit2-IR -> Update-IR -> Select-DR-Scan -> Capture-DR -> Exit1-DR -> Update-DR -> Run-Test/Idle */
	cpld_xc2c_jtag_shift_u32(jtag, 0b0110111, 0, 7);
	cpld_xc2c_jtag_clocks(jtag, 800);
}

static void cpld_xc2c_jtag_read(const jtag_t* const jtag) {
	cpld_xc2c_jtag_shift_ir_pause(jtag, CPLD_XC2C_IR_ISC_READ, 1);
}

static void cpld_xc2c64a_jtag_read_row(const jtag_t* const jtag, uint8_t address, uint8_t* const dr) {
	cpld_xc2c_jtag_shift_dr(jtag, &address, 7, 20);
	cpld_xc2c_jtag_clocks(jtag, 100);

	/* Set array to all ones so we don't transmit memory contents over TDI, and if we're not
	 * reading a full byte's worth of bits, the excess bits will be zero.
	 */
	memset(dr, 0xff, CPLD_XC2C64A_BYTES_IN_ROW);
	cpld_xc2c_jtag_shift_dr(jtag, dr, CPLD_XC2C64A_BITS_IN_ROW, 0);
	cpld_xc2c_jtag_clocks(jtag, 100);
}

bool cpld_xc2c64a_jtag_checksum(
	const jtag_t* const jtag,
	const cpld_xc2c64a_verify_t* const verify,
	uint32_t* const crc_value
) {
	cpld_xc2c_jtag_reset_and_idle(jtag);

	if( cpld_xc2c64a_jtag_idcode_ok(jtag) && cpld_xc2c_jtag_read_write_protect(jtag) &&
		cpld_xc2c64a_jtag_idcode_ok(jtag) && cpld_xc2c_jtag_read_write_protect(jtag) ) {
		
		cpld_xc2c_jtag_bypass(jtag, false);

		cpld_xc2c_jtag_enable(jtag);
		cpld_xc2c_jtag_enable(jtag);
		cpld_xc2c_jtag_enable(jtag);

		cpld_xc2c_jtag_read(jtag);

		crc32_t crc;
		crc32_init(&crc);

		uint8_t dr[CPLD_XC2C64A_BYTES_IN_ROW];
		for(size_t row=0; row<CPLD_XC2C64A_ROWS; row++) {
			const size_t address = cpld_xc2c64a_row_address[row];
			cpld_xc2c64a_jtag_read_row(jtag, address, dr);

			const size_t mask_index = verify->mask_index[row];
			for(size_t i=0; i<CPLD_XC2C64A_BYTES_IN_ROW; i++) {
				dr[i] &= verify->mask[mask_index].value[i];
			}

			/* Important checksum calculation NOTE:
			 * Do checksum of all bits in row bytes, but ensure that invalid bits
			 * are set to zero by masking. This subtlety just wasted several hours
			 * of my life...
			 */
			crc32_update(&crc, dr, CPLD_XC2C64A_BYTES_IN_ROW);
		}

		*crc_value = crc32_digest(&crc);

		cpld_xc2c_jtag_init_special(jtag);
		cpld_xc2c_jtag_conld(jtag);

		if( cpld_xc2c64a_jtag_idcode_ok(jtag) && cpld_xc2c_jtag_is_done(jtag) ) {
			cpld_xc2c_jtag_conld(jtag);
			cpld_xc2c_jtag_bypass(jtag, false);
			cpld_xc2c_jtag_bypass(jtag, true);

			return true;
		}
	}

	cpld_xc2c_jtag_reset_and_idle(jtag);

	return false;
}

static void cpld_xc2c64a_jtag_sram_write_row(const jtag_t* const jtag, uint8_t address, const uint8_t* const data) {
	uint8_t write[CPLD_XC2C64A_BYTES_IN_ROW];
	memcpy(&write[0], data, sizeof(write));

	/* Update-IR or Run-Test/Idle -> Shift-DR */
	cpld_xc2c_jtag_shift_u32(jtag, 0b001, 0b000, 3);

	/* Shift-DR -> Shift-DR */
	cpld_xc2c_jtag_shift_ptr_tms(jtag, &write[0], 0, CPLD_XC2C64A_BITS_IN_ROW, false);

	/* Shift-DR -> Exit1-DR */
	cpld_xc2c_jtag_shift_u32(jtag, 0b1000000, address, 7);

	/* Exit1-DR -> Update-DR -> Run-Test/Idle */
	cpld_xc2c_jtag_shift_u32(jtag, 0b01, 0b00, 2);
}

static void cpld_xc2c64a_jtag_sram_read_row(const jtag_t* const jtag, uint8_t* const data, const uint8_t next_address) {
	/* Run-Test/Idle -> Shift-DR */
	cpld_xc2c_jtag_shift_u32(jtag, 0b001, 0b000, 3);

	/* Shift-DR */
	cpld_xc2c_jtag_shift_ptr_tms(jtag, data, 0, CPLD_XC2C64A_BITS_IN_ROW, false);

	/* Shift-DR -> Exit1-DR */
	cpld_xc2c_jtag_shift_u32(jtag, 0b1000000, next_address, 7);

	/* Weird, non-IEEE1532 compliant path through TAP machine, described in Xilinx
	 * Programmer Qualification Specification, applicable only to XC2C64/A.
	 * Exit1-DR -> Pause-DR -> Exit2-DR -> Update-DR -> Run-Test/Idle
	 */
	cpld_xc2c_jtag_shift_u32(jtag, 0b0110, 0b0000, 4);
}

static bool cpld_xc2c64a_jtag_sram_compare_row(const jtag_t* const jtag, const uint8_t* const expected, const uint8_t* const mask, const uint8_t next_address) {
	/* Run-Test/Idle -> Shift-DR */
	uint8_t read[CPLD_XC2C64A_BYTES_IN_ROW];
	memset(read, 0xff, sizeof(read));
	cpld_xc2c64a_jtag_sram_read_row(jtag, &read[0], next_address);

	bool matched = true;
	if( (expected != NULL) && (mask != NULL) ) {
		for(size_t i=0; i<CPLD_XC2C64A_BYTES_IN_ROW; i++) {
			const uint8_t significant_differences = (read[i] ^ expected[i]) & mask[i];
			matched &= (significant_differences == 0);
		}
	}

	return matched;
}

void cpld_xc2c64a_jtag_sram_write(
	const jtag_t* const jtag,
	const cpld_xc2c64a_program_t* const program
) {
	cpld_xc2c_jtag_reset_and_idle(jtag);
	cpld_xc2c_jtag_enable(jtag);

	cpld_xc2c_jtag_sram_write(jtag);

	for(size_t row=0; row<CPLD_XC2C64A_ROWS; row++) {
		const uint8_t address = cpld_xc2c64a_row_address[row];
		cpld_xc2c64a_jtag_sram_write_row(jtag, address, &program->row[row].data[0]);
	}

	cpld_xc2c_jtag_disable(jtag);
	cpld_xc2c_jtag_bypass(jtag, false);
	cpld_xc2c_jtag_reset(jtag);
}

bool cpld_xc2c64a_jtag_sram_verify(
	const jtag_t* const jtag,
	const cpld_xc2c64a_program_t* const program,
	const cpld_xc2c64a_verify_t* const verify
) {
	cpld_xc2c_jtag_reset_and_idle(jtag);
	cpld_xc2c_jtag_enable(jtag);

	cpld_xc2c_jtag_sram_read(jtag);

	/* Tricky loop to read dummy row first, then first address, then loop back to get
	 * the first row's data.
	 */
	bool matched = true;
	for(size_t address_row=0; address_row<=CPLD_XC2C64A_ROWS; address_row++) {
		const int data_row = (int)address_row - 1;
		const size_t mask_index = (data_row >= 0) ? verify->mask_index[data_row] : 0;
		const uint8_t* const expected = (data_row >= 0) ? &program->row[data_row].data[0] : NULL;
		const uint8_t* const mask = (data_row >= 0) ? &verify->mask[mask_index].value[0] : NULL;
		const uint8_t next_address = (address_row < CPLD_XC2C64A_ROWS) ? cpld_xc2c64a_row_address[address_row] : 0;
		matched &= cpld_xc2c64a_jtag_sram_compare_row(jtag, expected, mask, next_address);
	}

	cpld_xc2c_jtag_disable(jtag);
	cpld_xc2c_jtag_bypass(jtag, false);
	cpld_xc2c_jtag_reset(jtag);

	return matched;
}
