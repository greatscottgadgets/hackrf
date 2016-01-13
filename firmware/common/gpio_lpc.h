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

#ifndef __GPIO_LPC_H__
#define __GPIO_LPC_H__

#include <stdint.h>

#include "gpio.h"

/* NOTE: libopencm3 constants and functions not used here due to naming
 * conflicts. I'd recommend changes to libopencm3 design to separate
 * register #defines and API declarations into separate header files.
 */

typedef struct gpio_port_t {
	volatile uint32_t dir;		/* +0x000 */
	uint32_t _reserved0[31];
	volatile uint32_t mask;		/* +0x080 */
	uint32_t _reserved1[31];
	volatile uint32_t pin;		/* +0x100 */
	uint32_t _reserved2[31];
	volatile uint32_t mpin;		/* +0x180 */
	uint32_t _reserved3[31];
	volatile uint32_t set;		/* +0x200 */
	uint32_t _reserved4[31];
	volatile uint32_t clr;		/* +0x280 */
	uint32_t _reserved5[31];
	volatile uint32_t not;		/* +0x300 */
} gpio_port_t;

struct gpio_t {
	const uint32_t mask;
	gpio_port_t* const port;
	volatile uint32_t* const gpio_w;
};

#define GPIO_LPC_BASE (0x400f4000)
#define GPIO_LPC_B_OFFSET (0x0)
#define GPIO_LPC_W_OFFSET (0x1000)
#define GPIO_LPC_PORT_OFFSET (0x2000)

#define GPIO_LPC_PORT(_n) ((gpio_port_t*)((GPIO_LPC_BASE + GPIO_LPC_PORT_OFFSET) + (_n) * 4))
#define GPIO_LPC_W(_port_num, _pin_num) (volatile uint32_t*)((GPIO_LPC_BASE + GPIO_LPC_W_OFFSET) + ((_port_num) * 0x80) + ((_pin_num) * 4))

#define GPIO(_port_num, _pin_num) { \
	.mask = (1UL << (_pin_num)), \
	.port = GPIO_LPC_PORT(_port_num), \
	.gpio_w = GPIO_LPC_W(_port_num, _pin_num), \
}

#endif/*__GPIO_LPC_H__*/
