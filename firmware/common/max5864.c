/*
 * Copyright 2012-2022 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2012 Jared Boone <jared@sharebrained.com>
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

#include "max5864.h"

#include <stdint.h>

#include <libopencm3/lpc43xx/ssp.h>

#include "max5864_target.h"
#include "spi_bus.h"

/* Driver instance. */
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

void ssp1_set_mode_max5864(void)
{
	spi_bus_start(max5864.bus, &ssp_config_max5864);
}

static void max5864_write(max5864_driver_t* const drv, uint8_t value)
{
	spi_bus_transfer(drv->bus, &value, 1);
}

static void max5864_init(max5864_driver_t* const drv)
{
	drv->target_init(drv);
}

void max5864_setup(max5864_driver_t* const drv)
{
	max5864_init(drv);
}

/* Set MAX5864 operation mode to "Shutdown":
 *   REF: off
 *   CLK: off
 *   ADCs: off (bus is tri-stated)
 *   DACs: off (set input bus to zero or OVdd)
 */
void max5864_shutdown(max5864_driver_t* const drv)
{
	max5864_write(drv, 0x00);
}

/* Set MAX5864 operation mode to "Standby":
 *   REF: on
 *   CLK: off?
 *   ADCs: off (bus is tri-stated)
 *   DACs: off (set input bus to zero or OVdd)
 */
void max5864_standby(max5864_driver_t* const drv)
{
	max5864_write(drv, 0x05);
}

/* Set MAX5864 operation mode to "Idle":
 *   REF: on
 *   CLK: on
 *   ADCs: off (bus is tri-stated)
 *   DACs: off (set input bus to zero or OVdd)
 */
void max5864_idle(max5864_driver_t* const drv)
{
	max5864_write(drv, 0x01);
}

/* Set MAX5864 operation mode to "Rx":
 *   REF: on
 *   CLK: on
 *   ADCs: on
 *   DACs: off (set input bus to zero or OVdd)
 */
void max5864_rx(max5864_driver_t* const drv)
{
	max5864_write(drv, 0x02);
}

/* Set MAX5864 operation mode to "Tx":
 *   REF: on
 *   CLK: on
 *   ADCs: off (bus is tri-stated)
 *   DACs: on
 */
void max5864_tx(max5864_driver_t* const drv)
{
	max5864_write(drv, 0x03);
}

/* Set MAX5864 operation mode to "Xcvr":
 *   REF: on
 *   CLK: on
 *   ADCs: on
 *   DACs: on
 */
void max5864_xcvr(max5864_driver_t* const drv)
{
	max5864_write(drv, 0x04);
}
