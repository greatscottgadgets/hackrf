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

#include <stdbool.h>
#if defined(PRALINE)
	#include <stdint.h>
#endif

#include "fixed_point.h"

typedef enum {
	CLOCK_SOURCE_HACKRF = 0,
	CLOCK_SOURCE_EXTERNAL = 1,
	CLOCK_SOURCE_PORTAPACK = 2,
} clock_source_t;

void clock_gen_init(void);
void clock_gen_shutdown(void);

clock_source_t activate_best_clock_source(void);

fp_40_24_t sample_rate_set(const fp_40_24_t sample_rate, const bool program);

void trigger_enable(const bool enable);

#ifdef PRALINE
typedef enum {
	P1_SIGNAL_TRIGGER_IN = 0,
	P1_SIGNAL_AUX_CLK1 = 1,
	P1_SIGNAL_CLKIN = 2,
	P1_SIGNAL_TRIGGER_OUT = 3,
	P1_SIGNAL_P22_CLKIN = 4,
	P1_SIGNAL_P2_5 = 5,
	P1_SIGNAL_NC = 6,
	P1_SIGNAL_AUX_CLK2 = 7,
} p1_ctrl_signal_t;

typedef enum {
	P2_SIGNAL_CLK3 = 0,
	P2_SIGNAL_TRIGGER_IN = 2,
	P2_SIGNAL_TRIGGER_OUT = 3,
} p2_ctrl_signal_t;

void p1_ctrl_set(const p1_ctrl_signal_t signal);
void p2_ctrl_set(const p2_ctrl_signal_t signal);

void pps_out_set(const uint8_t value);
#endif

#ifdef __cplusplus
}
#endif
