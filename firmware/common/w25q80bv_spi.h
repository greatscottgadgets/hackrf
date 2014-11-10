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

#ifndef __W25Q80BV_SPI_H__
#define __W25Q80BV_SPI_H__

#include <stddef.h>

#include "spi.h"

void w25q80bv_spi_init(spi_t* const spi);
void w25q80bv_spi_transfer_gather(spi_t* const spi, const spi_transfer_t* const transfers, const size_t transfer_count);
void w25q80bv_spi_transfer(spi_t* const spi, void* const data, const size_t count);

#endif/*__W25Q80BV_SPI_H__*/
