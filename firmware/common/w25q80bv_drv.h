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

typedef struct {
	/* Empty for now */
} w25q80bv_driver_t;

void w25q80bv_spi_init(w25q80bv_driver_t* const drv);

void w25q80bv_spi_select(w25q80bv_driver_t* const drv);
uint16_t w25q80bv_spi_transfer(w25q80bv_driver_t* const drv, const uint16_t tx_data);
void w25q80bv_spi_unselect(w25q80bv_driver_t* const drv);

#endif//__W25Q80BV_DRV_H__
