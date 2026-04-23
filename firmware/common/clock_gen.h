/*
 * Copyright 2026 Great Scott Gadgets <info@greatscottgadgets.com>
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

#ifdef __cplusplus
extern "C" {
#endif

#include "platform_detect.h" // IWYU pragma: keep

#include <stdbool.h>

#include "fixed_point.h"

typedef enum {
	CLOCK_SOURCE_HACKRF = 0,
	CLOCK_SOURCE_EXTERNAL = 1,
	CLOCK_SOURCE_PORTAPACK = 2,
} clock_source_t;

void clock_gen_init(void);
void clock_gen_shutdown(void);

clock_source_t activate_best_clock_source(void);

fp_28_36_t sample_rate_set(const fp_28_36_t sample_rate, const bool program);

#ifdef __cplusplus
}
#endif
