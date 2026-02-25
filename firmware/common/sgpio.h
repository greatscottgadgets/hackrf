/*
 * Copyright 2012-2022 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2012 Jared Boone <jared@sharebrained.com>
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

#ifndef __SGPIO_H__
#define __SGPIO_H__

#include <stdint.h>
#include <stdbool.h>

#include <libopencm3/lpc43xx/sgpio.h>

#include "gpio.h"

typedef enum {
	SGPIO_DIRECTION_RX,
	SGPIO_DIRECTION_TX,
} sgpio_direction_t;

typedef struct sgpio_config_t {
	gpio_t gpio_q_invert;
#if !defined(PRALINE) || defined(UNIVERSAL)
	gpio_t gpio_trigger_enable;
#endif
	bool slice_mode_multislice;
} sgpio_config_t;

void sgpio_configure_pin_functions(sgpio_config_t* const config);
void sgpio_test_interface(sgpio_config_t* const config);
void sgpio_set_slice_mode(sgpio_config_t* const config, const bool multi_slice);
void sgpio_configure(sgpio_config_t* const config, const sgpio_direction_t direction);
void sgpio_cpld_stream_enable(sgpio_config_t* const config);
void sgpio_cpld_stream_disable(sgpio_config_t* const config);
bool sgpio_cpld_stream_is_enabled(sgpio_config_t* const config);

void sgpio_cpld_set_mixer_invert(sgpio_config_t* const config, uint_fast8_t invert);

#endif //__SGPIO_H__
