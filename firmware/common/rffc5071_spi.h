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

#include "spi_bus.h"

#include "gpio.h"

typedef struct rffc5071_spi_config_t {
	gpio_t gpio_select;
	gpio_t gpio_clock;
	gpio_t gpio_data;
} rffc5071_spi_config_t;

void rffc5071_spi_start(spi_bus_t* const bus, const void* const config);
void rffc5071_spi_stop(spi_bus_t* const bus);
void rffc5071_spi_transfer(spi_bus_t* const bus, void* const data, const size_t count);
void rffc5071_spi_transfer_gather(spi_bus_t* const bus, const spi_transfer_t* const transfer, const size_t count);

#endif // __RFFC5071_SPI_H
