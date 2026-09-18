/*
 * Copyright 2016-2022 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2016 Dominic Spill <dominicgs@gmail.com>
 * Copyright 2016 Mike Walters <mike@flomp.net>
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

#pragma once

#include <hackrf.h>

typedef struct {
	uint64_t min;
	uint64_t max;
} radio_range_t;

typedef struct {
	radio_range_t frequency_rf;
	radio_range_t frequency_rf_abs;
	radio_range_t frequency_if;
	radio_range_t frequency_if_abs;
	radio_range_t frequency_lo;
	radio_range_t sample_rate;
	radio_range_t bb_bandwidth;
} platform_values_t;

int platform_supported_values(
	uint8_t board_id,
	enum radio_config_mode mode,
	platform_values_t* values);
