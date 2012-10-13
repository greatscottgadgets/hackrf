/*
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

#ifndef __BITBAND_H__
#define __BITBAND_H__

#include <stdint.h>

volatile uint32_t* peripheral_bitband_address(volatile void* const address, const uint_fast8_t bit_number);
void peripheral_bitband_set(volatile void* const peripheral_address, const uint_fast8_t bit_number);
void peripheral_bitband_clear(volatile void* const peripheral_address, const uint_fast8_t bit_number);
uint32_t peripheral_bitband_get(volatile void* const peripheral_address, const uint_fast8_t bit_number);

#endif//__BITBAND_H__
