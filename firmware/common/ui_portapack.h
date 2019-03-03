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

const hackrf_ui_t* portapack_detect(void);

#endif/*__UI_PORTAPACK_H__*/
