/*
 * Copyright 2018-2022 Great Scott Gadgets <info@greatscottgadgets.com>
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

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ARRAY_SIZEOF(x) (sizeof(x) / sizeof(x[0]))

typedef struct {
	uint16_t v;
} ui_color_t;

typedef struct {
	int16_t x;
	int16_t y;
} ui_point_t;

typedef struct {
	int16_t width;
	int16_t height;
} ui_size_t;

typedef struct {
	ui_point_t point;
	ui_size_t size;
} ui_rect_t;

typedef struct {
	ui_size_t size;
	const uint8_t* const data;
} ui_bitmap_t;

typedef struct {
	const ui_size_t glyph_size;
	const uint8_t* const data;
	char c_start;
	size_t c_count;
	size_t data_stride;
} ui_font_t;

typedef struct {
} portapack_t;

bool portapack_init(void);

void portapack_if_init(void);

void portapack_audio_reset_state(const bool active);

void portapack_lcd_reset_state(const bool active);

void portapack_lcd_reset(void);

void portapack_lcd_init(void);

void portapack_addr(const bool value);

void portapack_dir_read(void);

void portapack_dir_write(void);

void portapack_data_mask_set(void);

void portapack_data_write_high(const uint32_t value);

void portapack_data_write_low(const uint32_t value);

void portapack_io_write(const bool address, const uint_fast16_t value);

void portapack_lcd_command(const uint32_t value);

void portapack_lcd_write_data(const uint32_t value);

void portapack_lcd_data_write_command_and_data(
	const uint_fast8_t command,
	const uint8_t* data,
	const size_t data_count);

void portapack_lcd_sleep_out(void);

void portapack_lcd_display_on(void);

bool portapack_present(void);

void portapack_backlight(const bool on);

void portapack_reference_oscillator(const bool on);

void portapack_fill_rectangle(const ui_rect_t rect, const ui_color_t color);

void portapack_clear_display(const ui_color_t color);

void portapack_draw_bitmap(
	const ui_point_t point,
	const ui_bitmap_t bitmap,
	const ui_color_t foreground,
	const ui_color_t background);

ui_bitmap_t portapack_font_glyph(const ui_font_t* const font, const char c);
