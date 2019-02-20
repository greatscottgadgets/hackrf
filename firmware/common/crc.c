/*
 * Copyright 2019 Jared Boone <jared@sharebrained.com>
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

#include "crc.h"

#include <stdbool.h>

void crc32_init(crc32_t* const crc) {
	crc->remainder = 0xffffffff;
	crc->reversed_polynomial = 0xedb88320;
	crc->final_xor = 0xffffffff;
}

void crc32_update(crc32_t* const crc, const uint8_t* const data, const size_t byte_count) {
	uint32_t remainder = crc->remainder;
	const size_t bit_count = byte_count * 8;
	for(size_t bit_n=0; bit_n<bit_count; bit_n++) {
		const bool bit_in = data[bit_n >> 3] & (1 << (bit_n & 7));
		remainder ^= (bit_in ? 1 : 0);
		const bool bit_out = (remainder & 1);
		remainder >>= 1;
		if( bit_out ) {
			remainder ^= crc->reversed_polynomial;
		}
	}
	crc->remainder = remainder;
}

uint32_t crc32_digest(const crc32_t* const crc) {
	return crc->remainder ^ crc->final_xor;
}
