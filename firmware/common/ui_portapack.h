/*
 * Copyright 2018 Jared Boone
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

#ifndef __UI_PORTAPACK_H__
#define __UI_PORTAPACK_H__

#include <stddef.h>

typedef struct ui_color_t {
	uint16_t v;
} ui_color_t;

typedef struct ui_point_t {
	int16_t x;
	int16_t y;
} ui_point_t;

typedef struct ui_size_t {
	int16_t width;
	int16_t height;
} ui_size_t;

typedef struct ui_rect_t {
	ui_point_t point;
	ui_size_t size;
} ui_rect_t;

typedef struct ui_bitmap_t {
	ui_size_t size;
	const uint8_t* const data;
} ui_bitmap_t;

typedef struct ui_font_t {
	const ui_size_t glyph_size;
	const uint8_t* const data;
	char c_start;
	size_t c_count;
	size_t data_stride;
} ui_font_t;

typedef void (*hackrf_ui_init_fn)(void);
typedef void (*hackrf_ui_set_frequency_fn)(uint64_t frequency);
typedef void (*hackrf_ui_set_sample_rate_fn)(uint32_t sample_rate);
typedef void (*hackrf_ui_set_direction_fn)(const rf_path_direction_t direction);
typedef void (*hackrf_ui_set_filter_bw_fn)(uint32_t bandwidth);
typedef void (*hackrf_ui_set_lna_power_fn)(bool lna_on);
typedef void (*hackrf_ui_set_bb_lna_gain_fn)(const uint32_t gain_db);
typedef void (*hackrf_ui_set_bb_vga_gain_fn)(const uint32_t gain_db);
typedef void (*hackrf_ui_set_bb_tx_vga_gain_fn)(const uint32_t gain_db);
typedef void (*hackrf_ui_set_first_if_frequency_fn)(const uint64_t frequency);
typedef void (*hackrf_ui_set_filter_fn)(const rf_path_filter_t filter);
typedef void (*hackrf_ui_set_antenna_bias_fn)(bool antenna_bias);

typedef struct {
	hackrf_ui_init_fn init;
	hackrf_ui_set_frequency_fn set_frequency;
	hackrf_ui_set_sample_rate_fn set_sample_rate;
	hackrf_ui_set_direction_fn set_direction;
	hackrf_ui_set_filter_bw_fn set_filter_bw;
	hackrf_ui_set_lna_power_fn set_lna_power;
	hackrf_ui_set_bb_lna_gain_fn set_bb_lna_gain;
	hackrf_ui_set_bb_vga_gain_fn set_bb_vga_gain;
	hackrf_ui_set_bb_tx_vga_gain_fn set_bb_tx_vga_gain;
	hackrf_ui_set_first_if_frequency_fn set_first_if_frequency;
	hackrf_ui_set_filter_fn set_filter;
	hackrf_ui_set_antenna_bias_fn set_antenna_bias;
} hackrf_ui_t;

#endif/*__UI_PORTAPACK_H__*/
