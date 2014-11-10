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

#include "w25q80bv_spi.h"

#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/rgu.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/ssp.h>

#include "hackrf_core.h"

void w25q80bv_spi_init(spi_t* const spi) {
	(void)spi;
	
	const uint8_t serial_clock_rate = 2;
	const uint8_t clock_prescale_rate = 2;

	/* Reset SPIFI peripheral before to Erase/Write SPIFI memory through SPI */
	RESET_CTRL1 = RESET_CTRL1_SPIFI_RST;
	
	/* initialize SSP0 */
	ssp_init(SSP0_NUM,
			SSP_DATA_8BITS,
			SSP_FRAME_SPI,
			SSP_CPOL_0_CPHA_0,
			serial_clock_rate,
			clock_prescale_rate,
			SSP_MODE_NORMAL,
			SSP_MASTER,
			SSP_SLAVE_OUT_ENABLE);

	/* Init SPIFI GPIO to Normal GPIO */
	scu_pinmux(P3_3, (SCU_SSP_IO | SCU_CONF_FUNCTION2));    // P3_3 SPIFI_SCK => SSP0_SCK
	scu_pinmux(P3_4, (SCU_GPIO_FAST | SCU_CONF_FUNCTION0)); // P3_4 SPIFI SPIFI_SIO3 IO3 => GPIO1[14]
	scu_pinmux(P3_5, (SCU_GPIO_FAST | SCU_CONF_FUNCTION0)); // P3_5 SPIFI SPIFI_SIO2 IO2 => GPIO1[15]
	scu_pinmux(P3_6, (SCU_GPIO_FAST | SCU_CONF_FUNCTION0)); // P3_6 SPIFI SPIFI_MISO IO1 => GPIO0[6]
	scu_pinmux(P3_7, (SCU_GPIO_FAST | SCU_CONF_FUNCTION4)); // P3_7 SPIFI SPIFI_MOSI IO0 => GPIO5[10]
	scu_pinmux(P3_8, (SCU_GPIO_FAST | SCU_CONF_FUNCTION4)); // P3_8 SPIFI SPIFI_CS => GPIO5[11]
	
	/* configure SSP pins */
	scu_pinmux(SCU_SSP0_MISO, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP0_MOSI, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP0_SCK,  (SCU_SSP_IO | SCU_CONF_FUNCTION2));

	/* configure GPIO pins */
	scu_pinmux(SCU_FLASH_HOLD, SCU_GPIO_FAST);
	scu_pinmux(SCU_FLASH_WP, SCU_GPIO_FAST);
	scu_pinmux(SCU_SSP0_SSEL, (SCU_GPIO_FAST | SCU_CONF_FUNCTION4));

	/* drive SSEL, HOLD, and WP pins high */
	gpio_set(PORT_FLASH, (PIN_FLASH_HOLD | PIN_FLASH_WP));
	gpio_set(PORT_SSP0_SSEL, PIN_SSP0_SSEL);

	/* Set GPIO pins as outputs. */
	GPIO1_DIR |= (PIN_FLASH_HOLD | PIN_FLASH_WP);
	GPIO5_DIR |= PIN_SSP0_SSEL;
}

void w25q80bv_spi_transfer_gather(
	spi_t* const spi,
	const spi_transfer_t* const transfers,
	const size_t transfer_count
) {
	(void)spi;

	gpio_clear(PORT_SSP0_SSEL, PIN_SSP0_SSEL);
	for(size_t i=0; i<transfer_count; i++) {
		uint8_t* const p = transfers[i].data;
		for(size_t j=0; j<transfers[i].count; j++) {
			p[j] = ssp_transfer(SSP0_NUM, p[j]);
		}
	}
	gpio_set(PORT_SSP0_SSEL, PIN_SSP0_SSEL);
}

void w25q80bv_spi_transfer(spi_t* const spi, void* const data, const size_t count) {
	const spi_transfer_t transfers[] = {
		{ data, count },
	};
	w25q80bv_spi_transfer_gather(spi, transfers, 1);
}
