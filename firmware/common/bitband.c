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

#include "bitband.h"

volatile uint32_t* peripheral_bitband_address(volatile void* const address, const uint_fast8_t bit_number) {
    const uint32_t bit_band_base = 0x42000000;
    const uint32_t byte_offset = (uint32_t)address - 0x40000000;
    const uint32_t bit_word_offset = (byte_offset * 32) + (bit_number * 4);
    const uint32_t bit_word_address = bit_band_base + bit_word_offset;
    return (volatile uint32_t*)bit_word_address;
}

void peripheral_bitband_set(volatile void* const peripheral_address, const uint_fast8_t bit_number) {
    volatile uint32_t* const bitband_address = peripheral_bitband_address(peripheral_address, bit_number);
    *bitband_address = 1;
}

void peripheral_bitband_clear(volatile void* const peripheral_address, const uint_fast8_t bit_number) {
    volatile uint32_t* const bitband_address = peripheral_bitband_address(peripheral_address, bit_number);
    *bitband_address = 0;
}

uint32_t peripheral_bitband_get(volatile void* const peripheral_address, const uint_fast8_t bit_number) {
    volatile uint32_t* const bitband_address = peripheral_bitband_address(peripheral_address, bit_number);
    return *bitband_address;
}
