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

const platform_gpio_t* platform_gpio()
{
	static const platform_gpio_t* _platform_gpio = NULL;
	if (_platform_gpio != NULL) {
		return _platform_gpio;
	}

	board_id_t board_id = detected_platform();
	board_rev_t board_rev = detected_revision();
	static platform_gpio_t gpio;
	(void) board_rev; // TODO silence warning until we use this

	/* RF switch control */
	switch (board_id) {
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
		gpio.hp             = GPIO(2, 0);
		gpio.lp             = GPIO(2, 10);
		gpio.tx_mix_bp      = GPIO(2, 11);
		gpio.no_mix_bypass  = GPIO(1, 0);
		gpio.rx_mix_bp      = GPIO(2, 12);
		gpio.tx_amp         = GPIO(2, 15);
		gpio.tx             = GPIO(5, 15);
		gpio.mix_bypass     = GPIO(5, 16);
		gpio.rx             = GPIO(5, 5);
		gpio.no_tx_amp_pwr  = GPIO(3, 5);
		gpio.amp_bypass     = GPIO(0, 14);
		gpio.rx_amp         = GPIO(1, 11);
		gpio.no_rx_amp_pwr  = GPIO(1, 12);
        // HackRF One rev 9
		gpio.h1r9_rx         = GPIO(0, 7);
		gpio.h1r9_no_ant_pwr = GPIO(2, 4);
        break;
    case BOARD_ID_RAD1O:
		gpio.tx_rx_n         = GPIO(1, 11);
		gpio.tx_rx           = GPIO(0, 14);
		gpio.by_mix          = GPIO(1, 12);
		gpio.by_mix_n        = GPIO(2, 10);
		gpio.by_amp          = GPIO(1, 0);
		gpio.by_amp_n        = GPIO(5, 5);
		gpio.mixer_en        = GPIO(5, 16);
		gpio.low_high_filt   = GPIO(2, 11);
		gpio.low_high_filt_n = GPIO(2, 12);
		gpio.tx_amp          = GPIO(2, 15);
		gpio.rx_lna          = GPIO(5, 15);
        break;
    case BOARD_ID_PRALINE:
		gpio.tx_en         = GPIO(3, 4);
		gpio.mix_en_n      = GPIO(3, 2);
		gpio.mix_en_n_r1_0 = GPIO(5, 6);
		gpio.lpf_en        = GPIO(4, 8);
		gpio.rf_amp_en     = GPIO(4, 9);
		gpio.ant_bias_en_n = GPIO(1, 12);
		break;
	case BOARD_ID_JELLYBEAN:
	case BOARD_ID_JAWBREAKER:
		break;
	default:
		// TODO handle UNRECOGNIZED & UNDETECTED
		break;
	}

	_platform_gpio = &gpio;

	return _platform_gpio;
}

// clang-format on
