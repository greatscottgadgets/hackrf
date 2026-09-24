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

#include <string.h>
#include <hackrf.h>

#include "radio_limits.h"

#define ONE_KHZ (1000ull)
#define ONE_MHZ (1000000ull)
#define ONE_GHZ (1000000000ull)

#define KHZ(khz) (khz##ULL * ONE_KHZ)
#define MHZ(mhz) (mhz##ULL * ONE_MHZ)
#define GHZ(ghz) (ghz##ULL * ONE_GHZ)

int radio_limits(uint8_t board_id, enum radio_config_mode mode, radio_limits_t* limits)
{
	if (board_id != BOARD_ID_PRALINE && mode != RADIO_CONFIG_STANDARD ||
	    limits == NULL) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	memset(limits, 0, sizeof(*limits));
	limits->board_id = board_id;
	limits->config_mode = mode;

	// Absolute min / max
	limits->frequency_rf_abs.min = 0;
	limits->frequency_rf_abs.max = 7250000000; /* 7.25 GHz */
	limits->frequency_if_abs.min = 2000000000; /* 2 GHz */
	limits->frequency_if_abs.max = 3000000000; /* 3 GHz */

	// MAX2831, MAX2837, MAX2839
	limits->gain_rf_rx.min = 0;  /* 0 dB */
	limits->gain_rf_rx.max = 62; /* 62 dB */
	limits->gain_rf_rx.step = 2; /* 2 dB */

	// RFFC5072
	limits->frequency_lo.min = 84375000;   /* 84.375 MHz */
	limits->frequency_lo.max = 5400000000; /* 5.4 GHz */

	// MAX5864, MAX5865
	limits->sample_rate.min = 2000000;  /* 2 MHz */
	limits->sample_rate.max = 20000000; /* 20 MHz */

	// Board-specific limits
	if (board_id == BOARD_ID_PRALINE) {
		limits->frequency_rf.min = 100000;     /* 100 kHz */
		limits->frequency_rf.max = 6000000000; /* 6 GHz TODO verify */

		// MAX2831
		limits->frequency_if.min = 2320000000; /* 2.32 GHz */
		limits->frequency_if.max = 2580000000; /* 2.58 GHz */
		limits->gain_if_rx.min = 0;            /* 0 dB */
		limits->gain_if_rx.max = 33;           /* 33 dB */
		limits->gain_if_rx.step = 8;           /* TODO 8dB */
		limits->gain_if_tx.min = 0;            /* 0 dB */
		limits->gain_if_tx.max = 31;           /* 31 dB */
		limits->gain_if_tx.step = 1;           /* TODO 1dB */
		// + Narrowband Filter + FPGA
		limits->bb_bandwidth_rx.min = 1500000;  /* 1.5 MHz */
		limits->bb_bandwidth_rx.max = 31100000; /* 31.1 MHz */
		limits->bb_bandwidth_tx.min = 1500000;  /* 1.5 MHz */
		limits->bb_bandwidth_tx.max = 36000000; /* 36 MHz */

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
		limits->frequency_rf.min = 1000000;    /* 1 MHz */
		limits->frequency_rf.max = 6000000000; /* 6 GHz */

		// MAX2837, MAX2839
		limits->frequency_if.min = 2170000000;  /* 2.17 GHz */
		limits->frequency_if.max = 2740000000;  /* 2.74 GHz */
		limits->gain_if_rx.min = 0;             /* 0 dB */
		limits->gain_if_rx.max = 40;            /* 40 dB */
		limits->gain_if_rx.step = 8;            /* 8dB */
		limits->gain_if_tx.min = 0;             /* 0 dB */
		limits->gain_if_tx.max = 47;            /* 47 dB */
		limits->gain_if_tx.step = 1;            /* 1dB */
		limits->bb_bandwidth_rx.min = 1750000;  /* 1.75 MHz */
		limits->bb_bandwidth_rx.max = 28000000; /* 28 MHz */
		limits->bb_bandwidth_tx.min = 1750000;  /* 1.75 MHz */
		limits->bb_bandwidth_tx.max = 28000000; /* 28 MHz */
	}

	return HACKRF_SUCCESS;
}
