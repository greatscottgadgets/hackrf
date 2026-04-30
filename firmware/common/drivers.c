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

#include "drivers.h"

#if defined(IS_PRALINE)
	#include <libopencm3/lpc43xx/ssp.h>
	#include "fpga.h"
	#include "ice40_spi.h"
	#include "spi_bus.h"
#endif

#ifdef IS_PRALINE
ssp_config_t ssp_config_ice40_fpga = {
	.data_bits = SSP_DATA_8BITS,
	.spi_mode = SSP_CPOL_1_CPHA_1,
	.serial_clock_rate = 21,
	.clock_prescale_rate = 2,
};

ice40_spi_driver_t ice40 = {
	.bus = &spi_bus_ssp1,
};

fpga_driver_t fpga = {
	.bus = &ice40,
};
#endif

jtag_gpio_t jtag_gpio_cpld = {};

jtag_t jtag_cpld = {
	.gpio = &jtag_gpio_cpld,
};

#ifdef IS_PRALINE
void ssp1_set_mode_ice40(void)
{
	spi_bus_start(&spi_bus_ssp1, &ssp_config_ice40_fpga);
}
#endif
