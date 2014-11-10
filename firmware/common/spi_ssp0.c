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

#include "spi_ssp0.h"

#include <libopencm3/lpc43xx/rgu.h>
#include <libopencm3/lpc43xx/ssp.h>

#include "hackrf_core.h"

void spi_ssp0_init(spi_t* const spi, const void* const _config) {
	/* Reset SPIFI peripheral before to Erase/Write SPIFI memory through SPI */
	RESET_CTRL1 = RESET_CTRL1_SPIFI_RST;
	
	const ssp0_config_t* const config = _config;

	ssp_init(SSP0_NUM,
		config->data_bits,
		SSP_FRAME_SPI,
		SSP_CPOL_0_CPHA_0,
		config->serial_clock_rate,
		config->clock_prescale_rate,
		SSP_MODE_NORMAL,
		SSP_MASTER,
		SSP_SLAVE_OUT_ENABLE);

	spi->config = config;
}

void spi_ssp0_transfer_gather(spi_t* const spi, const spi_transfer_t* const transfers, const size_t count) {
	const ssp0_config_t* const config = spi->config;

	const size_t word_size = (SSP0_CR0 & 0xf) + 1;

	config->select(spi);
	for(size_t i=0; i<count; i++) {
		const size_t data_count = transfers[i].count;

		if( word_size > 8 ) {
			uint16_t* const data = transfers[i].data;
			for(size_t j=0; j<data_count; j++) {
				data[j] = ssp_transfer(SSP0_NUM, data[j]);
			}
		} else {
			uint8_t* const data = transfers[i].data;
			for(size_t j=0; j<data_count; j++) {
				data[j] = ssp_transfer(SSP0_NUM, data[j]);
			}
		}
	}
	config->unselect(spi);
}

void spi_ssp0_transfer(spi_t* const spi, void* const data, const size_t count) {
	const spi_transfer_t transfers[] = {
		{ data, count },
	};
	spi_ssp0_transfer_gather(spi, transfers, 1);
}
