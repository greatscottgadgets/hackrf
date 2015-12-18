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

#ifndef __SPI_SSP_H__
#define __SPI_SSP_H__

#include <stdint.h>
#include <stddef.h>

#include "spi_bus.h"

#include "gpio.h"

#include <libopencm3/lpc43xx/ssp.h>

typedef struct ssp_config_t {
	ssp_datasize_t data_bits;
	uint8_t serial_clock_rate;
	uint8_t clock_prescale_rate;
	gpio_t gpio_select;
} ssp_config_t;

void spi_ssp_start(spi_bus_t* const bus, const void* const config);
void spi_ssp_stop(spi_bus_t* const bus);
void spi_ssp_transfer(spi_bus_t* const bus, void* const data, const size_t count);
void spi_ssp_transfer_gather(spi_bus_t* const bus, const spi_transfer_t* const transfers, const size_t count);

#endif/*__SPI_SSP_H__*/
