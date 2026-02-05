/*
 * Copyright 2012-2022 Great Scott Gadgets <info@greatscottgadgets.com>
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

#ifndef __MIXER_H
#define __MIXER_H

#include "rffc5071.h"
#include "max2871.h"

typedef enum {
	RFFC5071_VARIANT,
	MAX2871_VARIANT,
} mixer_variant_t;

typedef struct {
	mixer_variant_t type;

	union {
		rffc5071_driver_t rffc5071;
		max2871_driver_t max2871;
	};
} mixer_driver_t;

#include <stdint.h>
extern void mixer_bus_setup(mixer_driver_t* const mixer);
extern void mixer_setup(mixer_driver_t* const mixer, mixer_variant_t type);

/* Set frequency (Hz). */
extern uint64_t mixer_set_frequency(mixer_driver_t* const mixer, uint64_t hz);

extern void mixer_enable(mixer_driver_t* const mixer);
extern void mixer_disable(mixer_driver_t* const mixer);
extern void mixer_set_gpo(mixer_driver_t* const drv, uint8_t gpo);

#endif // __MIXER_H
