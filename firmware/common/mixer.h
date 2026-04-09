/*
 * Copyright 2012-2026 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2014 Jared Boone <jared@sharebrained.com>
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

#include "fixed_point.h"

#if defined(JAWBREAKER) || defined(HACKRF_ONE) || defined(PRALINE)
	#include "rffc5071.h"
typedef rffc5071_driver_t mixer_driver_t;
#elif defined(RAD1O)
	#include "max2871.h"
typedef max2871_driver_t mixer_driver_t;
#endif

#include <stdint.h>
extern void mixer_bus_setup(mixer_driver_t* const mixer);
extern void mixer_setup(mixer_driver_t* const mixer);

/* Set frequency (Hz). */
extern fp_40_24_t mixer_set_frequency(
	mixer_driver_t* const mixer,
	fp_40_24_t lo,
	bool program);

extern void mixer_enable(mixer_driver_t* const mixer);
extern void mixer_disable(mixer_driver_t* const mixer);
extern void mixer_set_gpo(mixer_driver_t* const drv, uint8_t gpo);
