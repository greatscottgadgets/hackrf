/*
 * Copyright 2024 Great Scott Gadgets <info@greatscottgadgets.com>
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

#ifndef __ICE40_SPI_H
#define __ICE40_SPI_H

#include <stdint.h>
#include <stdlib.h>
#include "gpio.h"
#include "spi_bus.h"

struct ice40_spi_driver_t;
typedef struct ice40_spi_driver_t ice40_spi_driver_t;

struct ice40_spi_driver_t {
	spi_bus_t* const bus;
	gpio_t gpio_select;
	gpio_t gpio_creset;
	gpio_t gpio_cdone;
};

void ice40_spi_target_init(ice40_spi_driver_t* const drv);
uint8_t ice40_spi_read(ice40_spi_driver_t* const drv, uint8_t r);
void ice40_spi_write(ice40_spi_driver_t* const drv, uint8_t r, uint16_t v);
bool ice40_spi_syscfg_program(
	ice40_spi_driver_t* const drv,
	size_t (*read_block_cb)(void* ctx, uint8_t* buffer),
	void* read_ctx);

#endif // __ICE40_SPI_H
