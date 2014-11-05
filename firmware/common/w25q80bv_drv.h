/*
 * Copyright 2013 Michael Ossmann
 * Copyright 2013 Benjamin Vernoux
 * Copyright 2014 Jared Boone, ShareBrained Technology
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

#ifndef __W25Q80BV_DRV_H__
#define __W25Q80BV_DRV_H__

#include <stdint.h>
#include <stddef.h>

typedef struct {
	uint8_t* const data;
	const size_t count;
} w25q80bv_transfer_t;

typedef struct {
	/* Empty for now */
} w25q80bv_hw_t;

void w25q80bv_hw_init(w25q80bv_hw_t* const hw);
void w25q80bv_hw_transfer(w25q80bv_hw_t* const hw, uint8_t* data, const size_t count);
void w25q80bv_hw_transfer_multiple(
	w25q80bv_hw_t* const hw,
	const w25q80bv_transfer_t* const transfers,
	const size_t transfer_count
);

#endif//__W25Q80BV_DRV_H__
