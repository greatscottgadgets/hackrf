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

#include "max5864_spi.h"

#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/ssp.h>

#include "hackrf_core.h"

void max5864_spi_init(spi_t* const spi) {
	(void)spi;

	/* FIXME speed up once everything is working reliably */
	/*
	// Freq About 0.0498MHz / 49.8KHz => Freq = PCLK / (CPSDVSR * [SCR+1]) with PCLK=PLL1=204MHz
	const uint8_t serial_clock_rate = 32;
	const uint8_t clock_prescale_rate = 128;
	*/
	// Freq About 4.857MHz => Freq = PCLK / (CPSDVSR * [SCR+1]) with PCLK=PLL1=204MHz
	const uint8_t serial_clock_rate = 21;
	const uint8_t clock_prescale_rate = 2;
	
	ssp_init(SSP1_NUM,
		SSP_DATA_8BITS,
		SSP_FRAME_SPI,
		SSP_CPOL_0_CPHA_0,
		serial_clock_rate,
		clock_prescale_rate,
		SSP_MODE_NORMAL,
		SSP_MASTER,
		SSP_SLAVE_OUT_ENABLE);

	/* Configure SSP1 Peripheral (to be moved later in SSP driver) */
	scu_pinmux(SCU_SSP1_MISO, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP1_MOSI, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP1_SCK,  (SCU_SSP_IO | SCU_CONF_FUNCTION1));
}

void max5864_spi_transfer_gather(spi_t* const spi, const spi_transfer_t* const transfers, const size_t count) {
	(void)spi;

	gpio_clear(PORT_AD_CS, PIN_AD_CS);
	for(size_t i=0; i<count; i++) {
		const size_t data_count = transfers[i].count;
		uint8_t* const data = transfers[i].data;
		for(size_t j=0; j<data_count; j++) {
			data[j] = ssp_transfer(SSP1_NUM, data[j]);
		}
	}		
	gpio_set(PORT_AD_CS, PIN_AD_CS);
}

void max5864_spi_transfer(spi_t* const spi, void* const data, const size_t count) {
	const spi_transfer_t transfers[] = {
		{ data, count },
	};
	max5864_spi_transfer_gather(spi, transfers, 1);
}
