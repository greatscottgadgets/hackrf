/*
 * Copyright 2025 Great Scott Gadgets <info@greatscottgadgets.com>
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

#ifndef __SELFTEST_H
#define __SELFTEST_H

#include <stdbool.h>
#include <stdint.h>

#define NUM_LOCK_ATTEMPTS 3

enum {
	FAILED = 0,
	PASSED = 1,
	SKIPPED = 2,
	TIMEOUT = 3,
};

typedef uint8_t test_result_t;

typedef struct {
	uint16_t mixer_id;
#ifndef RAD1O
	bool mixer_locks[NUM_LOCK_ATTEMPTS];
#endif
#ifdef PRALINE
	uint16_t max2831_mux_rssi_1;
	uint16_t max2831_mux_temp;
	uint16_t max2831_mux_rssi_2;
	bool max2831_mux_test_ok;
#else
	uint16_t max283x_readback_bad_value;
	uint16_t max283x_readback_expected_value;
	uint8_t max283x_readback_register_count;
	uint8_t max283x_readback_total_registers;
#endif
	uint8_t si5351_rev_id;
	bool si5351_readback_ok;
#ifdef PRALINE
	test_result_t fpga_image_load;
	test_result_t fpga_spi;
	test_result_t sgpio_rx;
	test_result_t xcvr_loopback;

	struct xcvr_measurements {
		uint32_t zcs_i;
		uint32_t zcs_q;
		uint8_t max_mag_i;
		uint8_t max_mag_q;
		uint32_t avg_mag_sq_i;
		uint32_t avg_mag_sq_q;
	} xcvr_measurements[4];
#endif
	struct {
		bool pass;
		char msg[511];
	} report;
} selftest_t;

extern selftest_t selftest;

#endif // __SELFTEST_H
