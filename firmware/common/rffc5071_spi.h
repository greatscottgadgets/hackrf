/*
 * Copyright 2012 Michael Ossmann
 * Copyright 2014 Jared Boone <jared@sharebrained.com>
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

#ifndef __RFFC5071_SPI_H
#define __RFFC5071_SPI_H

#include "spi.h"

/* 31 registers, each containing 16 bits of data. */
#define RFFC5071_NUM_REGS 31

typedef struct {
	spi_t* const spi;
	uint16_t regs[RFFC5071_NUM_REGS];
	uint32_t regs_dirty;
} rffc5071_driver_t;

void rffc5071_spi_init(spi_t* const spi);
void rffc5071_spi_transfer(spi_t* const spi, uint16_t* const data, const size_t count);

#endif // __RFFC5071_SPI_H
