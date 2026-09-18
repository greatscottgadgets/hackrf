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

#include <hackrf.h>

#include "platform.h"

int platform_supported_values(
	uint8_t board_id,
	enum radio_config_mode mode,
	platform_values_t* values)
{
	// All platforms
	values->frequency_rf.min = 1000000;    /* 1 MHz */
	values->frequency_rf.max = 6000000000; /* 6 GHz */
	values->frequency_rf_abs.min = 0;
	values->frequency_rf_abs.max = 7250000000; /* 7.25 GHz */

	values->frequency_if.min = 2170000000;     /* 2.17 GHz */
	values->frequency_if.max = 2740000000;     /* 2.74 GHz */
	values->frequency_if_abs.min = 2000000000; /* 2 GHz */
	values->frequency_if_abs.max = 3000000000; /* 3 GHz */

	values->frequency_lo.min = 84375000;   /* 84.375 MHz */
	values->frequency_lo.max = 5400000000; /* 5.4 GHz */

	values->sample_rate.min = 2000000;  /* 2 MHz */
	values->sample_rate.max = 20000000; /* 20 MHz */

	values->bb_bandwidth.min = 1750000;  /* 1.75 MHz */
	values->bb_bandwidth.max = 28000000; /* 28 MHz */

	// TODO Individual platforms
	if (board_id == BOARD_ID_PRALINE) {
		switch (mode) {
		case RADIO_CONFIG_STANDARD:
			break;
		case RADIO_CONFIG_EXT_PRECISION_RX:
			break;
		case RADIO_CONFIG_EXT_PRECISION_TX:
			break;
		case RADIO_CONFIG_HALF_PRECISION:
			break;
		default:
			return HACKRF_ERROR_INVALID_PARAM;
		}
	} else {
		if (mode != RADIO_CONFIG_STANDARD) {
			return HACKRF_ERROR_INVALID_PARAM;
		}
	}

	return HACKRF_SUCCESS;
}
