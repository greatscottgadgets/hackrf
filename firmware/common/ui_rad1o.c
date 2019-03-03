/*
 * Copyright 2019 Dominic Spill
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
#include "ui_rad1o.h"

/* Weak functions from rad1o app */
void hackrf_ui_init(void) __attribute__((weak));
void hackrf_ui_setFrequency(uint64_t _freq) __attribute__((weak));
void hackrf_ui_setSampleRate(uint32_t _sample_rate) __attribute__((weak));
void hackrf_ui_setDirection(const rf_path_direction_t _direction) __attribute__((weak));
void hackrf_ui_setFilterBW(uint32_t bw) __attribute__((weak));
void hackrf_ui_setLNAPower(bool _lna_on) __attribute__((weak));
void hackrf_ui_setBBLNAGain(const uint32_t gain_db) __attribute__((weak));
void hackrf_ui_setBBVGAGain(const uint32_t gain_db) __attribute__((weak));
void hackrf_ui_setBBTXVGAGain(const uint32_t gain_db) __attribute__((weak));
void hackrf_ui_setFirstIFFrequency(const uint64_t freq) __attribute__((weak));
void hackrf_ui_setFilter(const rf_path_filter_t filter) __attribute__((weak));
void hackrf_ui_setAntennaBias(bool antenna_bias) __attribute__((weak));

static void rad1o_ui_init(void) {
    hackrf_ui_init();
}

static void rad1o_ui_set_frequency(uint64_t frequency) {
    hackrf_ui_setFrequency(frequency);
}

static void rad1o_ui_set_sample_rate(uint32_t sample_rate) {
    hackrf_ui_setSampleRate(sample_rate);
}

static void rad1o_ui_set_direction(const rf_path_direction_t direction) {
    hackrf_ui_setDirection(direction);
}

static void rad1o_ui_set_filter_bw(uint32_t bandwidth) {
    hackrf_ui_setFilterBW(bandwidth);
}

static void rad1o_ui_set_lna_power(bool lna_on) {
    hackrf_ui_setLNAPower(lna_on);
}

static void rad1o_ui_set_bb_lna_gain(const uint32_t gain_db) {
	hackrf_ui_setBBLNAGain(gain_db);
}

static void rad1o_ui_set_bb_vga_gain(const uint32_t gain_db) {
	hackrf_ui_setBBVGAGain(gain_db);
}

static void rad1o_ui_set_bb_tx_vga_gain(const uint32_t gain_db) {
	hackrf_ui_setBBTXVGAGain(gain_db);
}

static void rad1o_ui_set_first_if_frequency(const uint64_t frequency) {
	hackrf_ui_setFirstIFFrequency(frequency);
}

static void rad1o_ui_set_filter(const rf_path_filter_t filter) {
    hackrf_ui_setFilter(filter);
}

static void rad1o_ui_set_antenna_bias(bool antenna_bias) {
    hackrf_ui_setAntennaBias(antenna_bias);
}

static const hackrf_ui_t rad1o_ui = {
	&rad1o_ui_init,
	&rad1o_ui_set_frequency,
	&rad1o_ui_set_sample_rate,
	&rad1o_ui_set_direction,
	&rad1o_ui_set_filter_bw,
	&rad1o_ui_set_lna_power,
	&rad1o_ui_set_bb_lna_gain,
	&rad1o_ui_set_bb_vga_gain,
	&rad1o_ui_set_bb_tx_vga_gain,
	&rad1o_ui_set_first_if_frequency,
	&rad1o_ui_set_filter,
	&rad1o_ui_set_antenna_bias,
};

const hackrf_ui_t* rad1o_ui_setup(void) {
	return &rad1o_ui;
}
