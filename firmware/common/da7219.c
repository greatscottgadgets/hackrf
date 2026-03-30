/*
 * Copyright 2026 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include <stdint.h>

#include "da7219.h"
#include "hackrf_core.h"
#include "i2c_bus.h"

#define DA7219_REG_CHIP_ID1 0x81
#define DA7219_REG_CHIP_ID2 0x82

i2c_bus_t* const da7219_bus = &i2c0;

uint8_t da7219_read_reg(i2c_bus_t* const bus, uint8_t reg)
{
	const uint8_t data_tx[] = {reg};
	uint8_t data_rx[] = {0x00};
	i2c_bus_transfer(bus, DA7219_ADDRESS, data_tx, 1, data_rx, 1);
	return data_rx[0];
}

bool da7219_detect(void)
{
	uint8_t chip_id1 = da7219_read_reg(da7219_bus, DA7219_REG_CHIP_ID1);
	uint8_t chip_id2 = da7219_read_reg(da7219_bus, DA7219_REG_CHIP_ID2);
	return (chip_id1 == 0x23) && (chip_id2 == 0x93);
}
