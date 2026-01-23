/*
 * Copyright 2012-2026 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include <stddef.h>

#include "platform_gpio.h"

// clang-format off

// JAWBREAKER
static const platform_gpio_t platform_gpio_jawbreaker = {.jawbreaker = {}};

// RAD1O
static const platform_gpio_t platform_gpio_rad1o = {
	.rad1o = {
		/* RF switch control */
		.tx_rx_n		 = GPIO(1, 11),
		.tx_rx			 = GPIO(0, 14),
		.by_mix			 = GPIO(1, 12),
		.by_mix_n		 = GPIO(2, 10),
		.by_amp			 = GPIO(1, 0),
		.by_amp_n		 = GPIO(5, 5),
		.mixer_en		 = GPIO(5, 16),
		.low_high_filt	 = GPIO(2, 11),
		.low_high_filt_n = GPIO(2, 12),
		.tx_amp			 = GPIO(2, 15),
		.rx_lna			 = GPIO(5, 15),
	}};

// HACKRF_ONE
static const platform_gpio_t platform_gpio_hackrf_one = {
	.hackrf_one =
		{
			/* RF switch control */
			.hp				= GPIO(2, 0),
			.lp				= GPIO(2, 10),
			.tx_mix_bp		= GPIO(2, 11),
			.no_mix_bypass	= GPIO(1, 0),
			.rx_mix_bp		= GPIO(2, 12),
			.tx_amp			= GPIO(2, 15),
			.tx				= GPIO(5, 15),
			.mix_bypass		= GPIO(5, 16),
			.rx				= GPIO(5, 5),
			.no_tx_amp_pwr	= GPIO(3, 5),
			.amp_bypass		= GPIO(0, 14),
			.rx_amp			= GPIO(1, 11),
			.no_rx_amp_pwr	= GPIO(1, 12),
		},
	.hackrf1_r9 = {
		/* RF switch control */
		.rx			 = GPIO(0, 7),
		.no_ant_pwr	 = GPIO(2, 4),
	}};

// PRALINE
static const platform_gpio_t platform_gpio_praline = {
	.praline = {
		/* RF switch control */
		.tx_en			= GPIO(3, 4),
		.mix_en_n		= GPIO(3, 2),
		.mix_en_n_r1_0	= GPIO(5, 6),
		.lpf_en			= GPIO(4, 8),
		.rf_amp_en		= GPIO(4, 9),
		.ant_bias_en_n	= GPIO(1, 12),
	}};

// clang-format on

const platform_gpio_t* platform_gpio()
{
	static const platform_gpio_t* _platform_gpio = NULL;
	if (_platform_gpio != NULL) {
		return _platform_gpio;
	}

	board_id_t board_id = detected_platform();
	board_rev_t board_rev = detected_revision();
	(void) board_rev; // TODO silence warning until we use this

	switch (board_id) {
	case BOARD_ID_JELLYBEAN:
		break;
	case BOARD_ID_JAWBREAKER:
		_platform_gpio = &platform_gpio_jawbreaker;
		break;
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
		_platform_gpio = &platform_gpio_hackrf_one;
		break;
	case BOARD_ID_RAD1O:
		_platform_gpio = &platform_gpio_rad1o;
		break;
	case BOARD_ID_PRALINE:
		_platform_gpio = &platform_gpio_praline;
		break;
	default:
		// TODO handle UNRECOGNIZED & UNDETECTED
		break;
	}

	return _platform_gpio;
}
