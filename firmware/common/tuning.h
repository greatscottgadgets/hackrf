/*
 * Copyright 2012-2022 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2012 Jared Boone
 * Copyright 2013 Benjamin Vernoux
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

#include <stdbool.h>
#include <stdint.h>

#include "platform_detect.h"
#include "rf_path.h"
#include "transceiver_mode.h"
#if defined(IS_PRALINE)
	#include "tune_config.h"
#endif

#define FREQ_ONE_MHZ (1000ULL * 1000)

void tuning_setup(void);

bool set_freq(const uint64_t freq, const transceiver_mode_t opmode);
bool set_freq_explicit(
	const uint64_t if_freq_hz,
	const uint64_t lo_freq_hz,
	const rf_path_filter_t path,
	const transceiver_mode_t opmode);

#if defined(IS_PRALINE)
bool tuning_set_frequency(
	const tune_config_t* cfg,
	const uint64_t freq,
	const uint32_t offset,
	const transceiver_mode_t opmode);
#endif
