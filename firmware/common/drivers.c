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

#include <stdbool.h>

#include <libopencm3/lpc43xx/ssp.h>

#include "i2c_bus.h"
#include "i2c_lpc.h"
#include "max283x.h"
#include "max5864_target.h"
#include "spi_bus.h"
#if defined(IS_PRALINE)
	#include "fpga.h"
	#include "ice40_spi.h"
#endif

// const i2c_lpc_config_t i2c_config_si5351c_slow_clock = {
// 	.duty_cycle_count = 15,
// };

const i2c_lpc_config_t i2c_config_si5351c_fast_clock = {
	.duty_cycle_count = 255,
};

si5351c_driver_t clock_gen = {
	.bus = &i2c0,
	.i2c_address = 0x60,
};

ssp_config_t ssp_config_max283x = {
	/* FIXME speed up once everything is working reliably */
	/*
	// Freq About 0.0498MHz / 49.8KHz => Freq = PCLK / (CPSDVSR * [SCR+1]) with PCLK=PLL1=204MHz
	const uint8_t serial_clock_rate = 32;
	const uint8_t clock_prescale_rate = 128;
	*/
	// Freq About 4.857MHz => Freq = PCLK / (CPSDVSR * [SCR+1]) with PCLK=PLL1=204MHz
	.serial_clock_rate = 21,
	.clock_prescale_rate = 2,
};

max283x_driver_t max283x = {};

ssp_config_t ssp_config_max5864 = {
	/* FIXME speed up once everything is working reliably */
	/*
	// Freq About 0.0498MHz / 49.8KHz => Freq = PCLK / (CPSDVSR * [SCR+1]) with PCLK=PLL1=204MHz
	const uint8_t serial_clock_rate = 32;
	const uint8_t clock_prescale_rate = 128;
	*/
	// Freq About 4.857MHz => Freq = PCLK / (CPSDVSR * [SCR+1]) with PCLK=PLL1=204MHz
	.data_bits = SSP_DATA_8BITS,
	.serial_clock_rate = 21,
	.clock_prescale_rate = 2,
};

max5864_driver_t max5864 = {
	.bus = &spi_bus_ssp1,
	.target_init = max5864_target_init,
};

sgpio_config_t sgpio_config = {
	.slice_mode_multislice = true,
};

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

rf_path_t rf_path = {
	.switchctrl = 0,
};

jtag_gpio_t jtag_gpio_cpld = {};

jtag_t jtag_cpld = {
	.gpio = &jtag_gpio_cpld,
};

void ssp1_set_mode_max283x(void)
{
	spi_bus_start(&spi_bus_ssp1, &ssp_config_max283x);
}

void ssp1_set_mode_max5864(void)
{
	spi_bus_start(max5864.bus, &ssp_config_max5864);
}

#ifdef IS_PRALINE
void ssp1_set_mode_ice40(void)
{
	spi_bus_start(&spi_bus_ssp1, &ssp_config_ice40_fpga);
}
#endif
