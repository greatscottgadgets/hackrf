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

#include "pin_lpc.h"

#include <libopencm3/lpc43xx/gpio.h>

void pin_set(pin_t pin) {
	GPIO_SET(pin->gpio) = pin->mask;
}

void pin_clear(pin_t pin) {
	GPIO_CLR(pin->gpio) = pin->mask;
}

void pin_toggle(pin_t pin) {
	GPIO_NOT(pin->gpio) = pin->mask;
}

void pin_output(pin_t pin) {
	GPIO_DIR(pin->gpio) |= pin->mask;
}

void pin_input(pin_t pin) {
	GPIO_DIR(pin->gpio) &= ~pin->mask;
}

void pin_write(pin_t pin, const bool value) {
	MMIO32(pin->gpio_w) = value;
}

bool pin_read(pin_t pin) {
	return MMIO32(pin->gpio_w);
}
