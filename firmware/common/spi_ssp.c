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

#include "spi_ssp.h"

#include <libopencm3/lpc43xx/rgu.h>
#include <libopencm3/lpc43xx/ssp.h>

void spi_ssp_start(spi_bus_t* const bus, const void* const _config) {
	const ssp_config_t* const config = _config;

	if( bus->obj == (void*)SSP0_BASE ) {
		/* Reset SPIFI peripheral before to Erase/Write SPIFI memory through SPI */
		RESET_CTRL1 = RESET_CTRL1_SPIFI_RST;
	}

	gpio_set(config->gpio_select);
	gpio_output(config->gpio_select);

	SSP_CR1(bus->obj) = 0;
	SSP_CPSR(bus->obj) = config->clock_prescale_rate;
	SSP_CR0(bus->obj) =
		  (config->serial_clock_rate << 8)
		| SSP_CPOL_0_CPHA_0
		| SSP_FRAME_SPI
		| config->data_bits
		;
	SSP_CR1(bus->obj) =
		  SSP_SLAVE_OUT_ENABLE
		| SSP_MASTER
		| SSP_ENABLE
		| SSP_MODE_NORMAL
		;

	bus->config = config;
}

void spi_ssp_stop(spi_bus_t* const bus) {
	SSP_CR1(bus->obj) = 0;
}

static void spi_ssp_wait_for_tx_fifo_not_full(spi_bus_t* const bus) {
	while( (SSP_SR(bus->obj) & SSP_SR_TNF) == 0 );
}

static void spi_ssp_wait_for_rx_fifo_not_empty(spi_bus_t* const bus) {
	while( (SSP_SR(bus->obj) & SSP_SR_RNE) == 0 );
}

static void spi_ssp_wait_for_not_busy(spi_bus_t* const bus) {
	while( SSP_SR(bus->obj) & SSP_SR_BSY );
}

static uint32_t spi_ssp_transfer_word(spi_bus_t* const bus, const uint32_t data) {
	spi_ssp_wait_for_tx_fifo_not_full(bus);
	SSP_DR(bus->obj) = data;
	spi_ssp_wait_for_not_busy(bus);
	spi_ssp_wait_for_rx_fifo_not_empty(bus);
	return SSP_DR(bus->obj);
}

void spi_ssp_transfer_gather(spi_bus_t* const bus, const spi_transfer_t* const transfers, const size_t count) {
	const ssp_config_t* const config = bus->config;

	const bool word_size_u16 = (SSP_CR0(bus->obj) & 0xf) > SSP_DATA_8BITS;

	gpio_clear(config->gpio_select);
	for(size_t i=0; i<count; i++) {
		const size_t data_count = transfers[i].count;

		if( word_size_u16 ) {
			uint16_t* const data = transfers[i].data;
			for(size_t j=0; j<data_count; j++) {
				data[j] = spi_ssp_transfer_word(bus, data[j]);
			}
		} else {
			uint8_t* const data = transfers[i].data;
			for(size_t j=0; j<data_count; j++) {
				data[j] = spi_ssp_transfer_word(bus, data[j]);
			}
		}
	}
	gpio_set(config->gpio_select);
}

void spi_ssp_transfer(spi_bus_t* const bus, void* const data, const size_t count) {
	const spi_transfer_t transfers[] = {
		{ data, count },
	};
	spi_ssp_transfer_gather(bus, transfers, 1);
}
