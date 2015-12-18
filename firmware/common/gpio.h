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

#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdbool.h>

typedef const struct gpio_t* gpio_t;

void gpio_init();
void gpio_set(gpio_t gpio);
void gpio_clear(gpio_t gpio);
void gpio_toggle(gpio_t gpio);
void gpio_output(gpio_t gpio);
void gpio_input(gpio_t gpio);
void gpio_write(gpio_t gpio, const bool value);
bool gpio_read(gpio_t gpio);

#endif/*__GPIO_H__*/
