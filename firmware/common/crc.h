/*
 * Copyright 2019-2022 Great Scott Gadgets <info@greatscottgadgets.com>
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

#ifndef __CRC_H__
#define __CRC_H__

#include <stdint.h>
#include <stddef.h>

typedef struct {
	uint32_t remainder;
	uint32_t reversed_polynomial;
	uint32_t final_xor;
} crc32_t;

void crc32_init(crc32_t* const crc);
void crc32_update(crc32_t* const crc, const uint8_t* const data, const size_t byte_count);
uint32_t crc32_digest(const crc32_t* const crc);

#endif //__CRC_H__
