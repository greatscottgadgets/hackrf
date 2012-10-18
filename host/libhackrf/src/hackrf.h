/*
 * Copyright 2012 Jared Boone <jared@sharebrained.com>
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

#ifndef __HACKRF_H__
#define __HACKRF_H__

#include <stdint.h>
#include <stdbool.h>

enum hackrf_error {
	HACKRF_SUCCESS = 0,
	HACKRF_ERROR_INVALID_PARAM = -2,
	HACKRF_ERROR_NOT_FOUND = -5,
	HACKRF_ERROR_BUSY = -6,
	HACKRF_ERROR_NO_MEM = -11,
	HACKRF_ERROR_LIBUSB = -1000,
	HACKRF_ERROR_THREAD = -1001,
	HACKRF_ERROR_OTHER = -9999,
};

typedef struct hackrf_device hackrf_device;

typedef struct {
	hackrf_device* device;
	uint8_t* buffer;
	int buffer_length;
	int valid_length;
} hackrf_transfer;

typedef int (*hackrf_sample_block_cb_fn)(hackrf_transfer* transfer);

int hackrf_init();
int hackrf_exit();

int hackrf_open(hackrf_device** device);
int hackrf_close(hackrf_device* device);

int hackrf_start_rx(hackrf_device* device, hackrf_sample_block_cb_fn callback);
int hackrf_stop_rx(hackrf_device* device);

int hackrf_start_tx(hackrf_device* device, hackrf_sample_block_cb_fn callback);
int hackrf_stop_tx(hackrf_device* device);

bool hackrf_is_streaming(hackrf_device* device);

int hackrf_max2837_read(hackrf_device* device, uint8_t register_number, uint16_t* value);
int hackrf_max2837_write(hackrf_device* device, uint8_t register_number, uint16_t value);

int hackrf_si5351c_read(hackrf_device* device, uint16_t register_number, uint16_t* value);
int hackrf_si5351c_write(hackrf_device* device, uint16_t register_number, uint16_t value);

int hackrf_sample_rate_set(hackrf_device* device, const uint32_t sampling_rate_hz);
int hackrf_baseband_filter_bandwidth_set(hackrf_device* device, const uint32_t bandwidth_hz);

const char* hackrf_error_name(enum hackrf_error errcode);

#endif//__HACKRF_H__
