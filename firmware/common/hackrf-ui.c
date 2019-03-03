/*
 * Copyright (C) 2019 Jared Boone, ShareBrained Technology, Inc.
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

#include "hackrf-ui.h"

#include "ui_portapack.h"
#include "ui_rad1o.h"

#include <stddef.h>

#define UNUSED(x) (void)(x)

/* Stub functions for null UI function table */
void hackrf_ui_init_null(void) { }
void hackrf_ui_set_frequency_null(uint64_t frequency) { UNUSED(frequency); }
void hackrf_ui_set_sample_rate_null(uint32_t sample_rate) { UNUSED(sample_rate); }
void hackrf_ui_set_direction_null(const rf_path_direction_t direction) { UNUSED(direction); }
void hackrf_ui_set_filter_bw_null(uint32_t bandwidth) { UNUSED(bandwidth); }
void hackrf_ui_set_lna_power_null(bool lna_on) { UNUSED(lna_on); }
void hackrf_ui_set_bb_lna_gain_null(const uint32_t gain_db) { UNUSED(gain_db); }
void hackrf_ui_set_bb_vga_gain_null(const uint32_t gain_db) { UNUSED(gain_db); }
void hackrf_ui_set_bb_tx_vga_gain_null(const uint32_t gain_db) { UNUSED(gain_db); }
void hackrf_ui_set_first_if_frequency_null(const uint64_t frequency) { UNUSED(frequency); }
void hackrf_ui_set_filter_null(const rf_path_filter_t filter) { UNUSED(filter); }
void hackrf_ui_set_antenna_bias_null(bool antenna_bias) { UNUSED(antenna_bias); }

/* Null UI function table, used if there's no hardware UI detected. Eliminates the
 * need to check for null UI before calling a function in the table.
 */
static const hackrf_ui_t hackrf_ui_null = {
	&hackrf_ui_init_null,
	&hackrf_ui_set_frequency_null,
	&hackrf_ui_set_sample_rate_null,
	&hackrf_ui_set_direction_null,
	&hackrf_ui_set_filter_bw_null,
	&hackrf_ui_set_lna_power_null,
	&hackrf_ui_set_bb_lna_gain_null,
	&hackrf_ui_set_bb_vga_gain_null,
	&hackrf_ui_set_bb_tx_vga_gain_null,
	&hackrf_ui_set_first_if_frequency_null,
	&hackrf_ui_set_filter_null,
	&hackrf_ui_set_antenna_bias_null,
};

const hackrf_ui_t* portapack_detect(void) __attribute__((weak));
const hackrf_ui_t* rad1o_ui_setup(void) __attribute__((weak));

static const hackrf_ui_t* ui = NULL;

const hackrf_ui_t* hackrf_ui(void) {
	/* Detect on first use. If no UI hardware is detected, use a stub function table. */
	if( ui == NULL ) {
#ifdef HACKRF_ONE
		if( portapack_detect ) {
			ui = portapack_detect();
		}
#endif
#ifdef RAD1O
		if( rad1o_ui_setup ) {
			ui = rad1o_ui_setup();
		}
#endif
	}

	if( ui == NULL ) {
		ui = &hackrf_ui_null;
	}
	return ui;
}
