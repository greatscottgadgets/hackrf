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

#ifndef __W25Q80BV_H__
#define __W25Q80BV_H__

#include <stdint.h>
#include <stddef.h>

#include "spi_bus.h"
#include "gpio.h"

typedef union
{
	uint64_t id_64b;
	uint32_t id_32b[2]; /* 2*32bits 64bits Unique ID */
	uint8_t id_8b[8]; /* 8*8bits 64bits Unique ID */
} w25q80bv_unique_id_t;

struct w25q80bv_driver_t;
typedef struct w25q80bv_driver_t w25q80bv_driver_t;

struct w25q80bv_driver_t {
	spi_bus_t* bus;
	gpio_t gpio_hold;
	gpio_t gpio_wp;
	void (*target_init)(w25q80bv_driver_t* const drv);
	size_t page_len;
	size_t num_pages;
	size_t num_bytes;
};

void w25q80bv_setup(w25q80bv_driver_t* const drv);
void w25q80bv_chip_erase(w25q80bv_driver_t* const drv);
void w25q80bv_program(w25q80bv_driver_t* const drv, uint32_t addr, uint32_t len, uint8_t* data);
uint8_t w25q80bv_get_device_id(w25q80bv_driver_t* const drv);
void w25q80bv_get_unique_id(w25q80bv_driver_t* const drv, w25q80bv_unique_id_t* unique_id);
void w25q80bv_read(w25q80bv_driver_t* const drv, uint32_t addr, uint32_t len, uint8_t* const data);

#endif//__W25Q80BV_H__
