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

#ifndef __PIN_H__
#define __PIN_H__

#include <stdbool.h>

typedef const struct pin_t* pin_t;

void pin_set(pin_t pin);
void pin_clear(pin_t pin);
void pin_toggle(pin_t pin);
void pin_output(pin_t pin);
void pin_input(pin_t pin);
void pin_write(pin_t pin, const bool value);
bool pin_read(pin_t pin);

#endif/*__PIN_H__*/
