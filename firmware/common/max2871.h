/*
 * Copyright 2017 Great Scott Gadgets <info@greatscottgadgets.com>
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

#ifndef MAX2871_H
#define MAX2871_H

#include "gpio.h"

#include <stdint.h>

typedef struct {
	gpio_t gpio_vco_ce;
	gpio_t gpio_vco_sclk;
	gpio_t gpio_vco_sdata;
	gpio_t gpio_vco_le;
	gpio_t gpio_synt_rfout_en;
	gpio_t gpio_vco_mux;
} max2871_driver_t;

extern void max2871_setup(max2871_driver_t* const drv);
extern uint64_t max2871_set_frequency(max2871_driver_t* const drv, uint16_t mhz);
extern void max2871_enable(max2871_driver_t* const drv);
extern void max2871_disable(max2871_driver_t* const drv);
#endif
