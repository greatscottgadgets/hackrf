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

#include "max5864_target.h"

#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/gpio.h>
#include "hackrf_core.h"

void max5864_target_init(max5864_driver_t* const drv) {
	(void)drv;
	
	/*
	 * Configure CS_AD pin to keep the MAX5864 SPI disabled while we use the
	 * SPI bus for the MAX2837. FIXME: this should probably be somewhere else.
	 */
	scu_pinmux(SCU_AD_CS, SCU_GPIO_FAST);
	GPIO_SET(PORT_AD_CS) = PIN_AD_CS;
	GPIO_DIR(PORT_AD_CS) |= PIN_AD_CS;
}

void max5864_target_spi_select(spi_t* const spi) {
	(void)spi;
	gpio_clear(PORT_AD_CS, PIN_AD_CS);
}

void max5864_target_spi_unselect(spi_t* const spi) {
	(void)spi;
	gpio_set(PORT_AD_CS, PIN_AD_CS);
}
