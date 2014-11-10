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

#include "w25q80bv_target.h"

#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/gpio.h>
#include "hackrf_core.h"

/* TODO: Why is SSEL being controlled manually when SSP0 could do it
 * automatically?
 */

void w25q80bv_target_init(w25q80bv_driver_t* const drv) {
	(void)drv;

	scu_pinmux(SCU_SSP0_SSEL, (SCU_GPIO_FAST | SCU_CONF_FUNCTION4));
	gpio_set(PORT_SSP0_SSEL, PIN_SSP0_SSEL);
	GPIO5_DIR |= PIN_SSP0_SSEL;
}

void w25q80bv_target_spi_select(spi_t* const spi) {
	(void)spi;
	gpio_clear(PORT_SSP0_SSEL, PIN_SSP0_SSEL);
}

void w25q80bv_target_spi_unselect(spi_t* const spi) {
	(void)spi;
	gpio_set(PORT_SSP0_SSEL, PIN_SSP0_SSEL);
}
