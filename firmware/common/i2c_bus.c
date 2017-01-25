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

#include "i2c_bus.h"

void i2c_bus_start(i2c_bus_t* const bus, const void* const config) {
	bus->start(bus, config);
}

void i2c_bus_stop(i2c_bus_t* const bus) {
	bus->stop(bus);
}

void i2c_bus_transfer(
	i2c_bus_t* const bus,
	const uint_fast8_t slave_address,
	const uint8_t* const tx, const size_t tx_count,
	uint8_t* const rx, const size_t rx_count
) {
	bus->transfer(bus, slave_address, tx, tx_count, rx, rx_count);
}
