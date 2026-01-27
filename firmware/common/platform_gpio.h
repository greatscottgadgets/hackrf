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

#ifndef __PLATFORM_GPIO_H
#define __PLATFORM_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gpio_lpc.h"
#include "platform_detect.h"

typedef struct {
	/* RF switch control */
	// HACKRF_ONE
	struct gpio_t hp;
	struct gpio_t lp;
	struct gpio_t tx_mix_bp;
	struct gpio_t no_mix_bypass;
	struct gpio_t rx_mix_bp;
	struct gpio_t tx_amp; // also used by RAD1O
	struct gpio_t tx;
	struct gpio_t mix_bypass;
	struct gpio_t rx;
	struct gpio_t no_tx_amp_pwr;
	struct gpio_t amp_bypass;
	struct gpio_t rx_amp;
	struct gpio_t no_rx_amp_pwr;
	/* HACKRF_ONE r9 */
	struct gpio_t h1r9_no_ant_pwr; // used to live in rf_path.c
	struct gpio_t h1r9_rx;         // TODO also exists in usb_api_board_info.c
	// RAD1O
	struct gpio_t tx_rx_n;
	struct gpio_t tx_rx;
	struct gpio_t by_mix;
	struct gpio_t by_mix_n;
	struct gpio_t by_amp;
	struct gpio_t by_amp_n;
	struct gpio_t mixer_en;
	struct gpio_t low_high_filt;
	struct gpio_t low_high_filt_n;
	//struct gpio_t tx_amp;
	struct gpio_t rx_lna;
	// PRALINE
	struct gpio_t tx_en;
	struct gpio_t mix_en_n;
	struct gpio_t mix_en_n_r1_0;
	struct gpio_t lpf_en;
	struct gpio_t rf_amp_en;
	struct gpio_t ant_bias_en_n;
} platform_gpio_t;

// Detects and returns the global platform gpio instance of the active board id and revision.
const platform_gpio_t* platform_gpio();

#ifdef __cplusplus
}
#endif

#endif /* __PLATFORM_GPIO_H */
