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

#ifndef __RFPATH_H__
#define __RFPATH_H__

#include <stdint.h>

#include "gpio.h"

typedef enum {
	RF_PATH_DIRECTION_OFF,
	RF_PATH_DIRECTION_RX,
	RF_PATH_DIRECTION_TX,
	// #ifdef PRALINE
	RF_PATH_DIRECTION_TX_CALIBRATION,
	RF_PATH_DIRECTION_RX_CALIBRATION,
} rf_path_direction_t;

typedef enum {
	RF_PATH_FILTER_BYPASS = 0,
	RF_PATH_FILTER_LOW_PASS = 1,
	RF_PATH_FILTER_HIGH_PASS = 2,
} rf_path_filter_t;

typedef struct rf_path_t {
	uint8_t switchctrl;

	struct {
		// HACKRF_ONE
		gpio_t gpio_hp;
		gpio_t gpio_lp;
		gpio_t gpio_tx_mix_bp;
		gpio_t gpio_no_mix_bypass;
		gpio_t gpio_rx_mix_bp;
		gpio_t gpio_tx_amp;
		gpio_t gpio_tx;
		gpio_t gpio_mix_bypass;
		gpio_t gpio_rx;
		gpio_t gpio_no_tx_amp_pwr;
		gpio_t gpio_amp_bypass;
		gpio_t gpio_rx_amp;
		gpio_t gpio_no_rx_amp_pwr;
		// In HackRF One r9 this control signal has been moved to the microcontroller.
		gpio_t gpio_h1r9_no_ant_pwr;

		// RAD1O
		gpio_t gpio_tx_rx_n;
		gpio_t gpio_tx_rx;
		gpio_t gpio_by_mix;
		gpio_t gpio_by_mix_n;
		gpio_t gpio_by_amp;
		gpio_t gpio_by_amp_n;
		gpio_t gpio_mixer_en;
		gpio_t gpio_low_high_filt;
		gpio_t gpio_low_high_filt_n;
		// gpio_t gpio_tx_amp;
		gpio_t gpio_rx_lna;

		// PRALINE
		gpio_t gpio_tx_en;
		gpio_t gpio_mix_en_n;
		gpio_t gpio_lpf_en;
		gpio_t gpio_rf_amp_en;
		gpio_t gpio_ant_bias_en_n;
	};
} rf_path_t;

void rf_path_pin_setup(rf_path_t* const rf_path);
void rf_path_init(rf_path_t* const rf_path);

void rf_path_set_direction(rf_path_t* const rf_path, const rf_path_direction_t direction);

void rf_path_set_filter(rf_path_t* const rf_path, const rf_path_filter_t filter);

void rf_path_set_lna(rf_path_t* const rf_path, const uint_fast8_t enable);
void rf_path_set_antenna(rf_path_t* const rf_path, const uint_fast8_t enable);

#endif /*__RFPATH_H__*/
