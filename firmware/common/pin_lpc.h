/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __PIN_LPC_H__
#define __PIN_LPC_H__

#include <stdint.h>

#include "pin.h"

struct pin_t {
	uint32_t gpio;
	uint32_t mask;
	uint32_t gpio_w;
};

#define PIN_LPC(_port_num, _pin_num) { \
	.gpio = (GPIO0) + (_port_num) * 4, \
	.mask = (1UL << (_pin_num)), \
	.gpio_w = GPIO_PORT_BASE + 0x1000 + ((_port_num) * 0x80) + ((_pin_num) * 4), \
};

#endif/*__PIN_LPC_H__*/
