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

#ifndef __MAX5864_SPI_H__
#define __MAX5864_SPI_H__

#include <stddef.h>

#include "spi.h"

void max5864_spi_init(spi_t* const spi);
void max5864_spi_transfer(spi_t* const spi, void* const value, const size_t count);
void max5864_spi_transfer_gather(spi_t* const spi, const spi_transfer_t* const transfers, const size_t count);

#endif/*__MAX5864_SPI_H__*/
