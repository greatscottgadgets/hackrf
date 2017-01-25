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

#include "spi_bus.h"

void spi_bus_start(spi_bus_t* const bus, const void* const config) {
	bus->start(bus, config);
}

void spi_bus_stop(spi_bus_t* const bus) {
	bus->stop(bus);
}

void spi_bus_transfer(spi_bus_t* const bus, void* const data, const size_t count) {
	bus->transfer(bus, data, count);
}

void spi_bus_transfer_gather(spi_bus_t* const bus, const spi_transfer_t* const transfers, const size_t count) {
	bus->transfer_gather(bus, transfers, count);
}
