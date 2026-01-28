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
		gpio.hp             = &GPIO2_0;
		gpio.lp             = &GPIO2_10;
		gpio.tx_mix_bp      = &GPIO2_11;
		gpio.no_mix_bypass  = &GPIO1_0;
		gpio.rx_mix_bp      = &GPIO2_12;
		gpio.tx_amp         = &GPIO2_15;
		gpio.tx             = &GPIO5_15;
		gpio.mix_bypass     = &GPIO5_16;
		gpio.rx             = &GPIO5_5;
		gpio.no_tx_amp_pwr  = &GPIO3_5;
		gpio.amp_bypass     = &GPIO0_14;
		gpio.rx_amp         = &GPIO1_11;
		gpio.no_rx_amp_pwr  = &GPIO1_12;
        // HackRF One rev 9
		gpio.h1r9_rx         = &GPIO0_7;
		gpio.h1r9_no_ant_pwr = &GPIO2_4;
        break;
		case BOARD_ID_RAD1O:
		gpio.tx_rx_n         = &GPIO1_11;
		gpio.tx_rx           = &GPIO0_14;
		gpio.by_mix          = &GPIO1_12;
		gpio.by_mix_n        = &GPIO2_10;
		gpio.by_amp          = &GPIO1_0;
		gpio.by_amp_n        = &GPIO5_5;
		gpio.mixer_en        = &GPIO5_16;
		gpio.low_high_filt   = &GPIO2_11;
		gpio.low_high_filt_n = &GPIO2_12;
		gpio.tx_amp          = &GPIO2_15;
		gpio.rx_lna          = &GPIO5_15;
        break;
    case BOARD_ID_PRALINE:
		gpio.tx_en         = &GPIO3_4;
		gpio.mix_en_n      = &GPIO3_2;
		gpio.mix_en_n_r1_0 = &GPIO5_6;
		gpio.lpf_en        = &GPIO4_8;
		gpio.rf_amp_en     = &GPIO4_9;
		gpio.ant_bias_en_n = &GPIO1_12;
		break;
	case BOARD_ID_JELLYBEAN:
	case BOARD_ID_JAWBREAKER:
		break;
	default:
		break;
	}

	_platform_gpio = &gpio;

	return _platform_gpio;
}

// clang-format on
