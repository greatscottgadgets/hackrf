/*
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

#include <stdint.h>

#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/ssp.h>

#include "hackrf_core.h"
#include "max5864.h"

void max5864_spi_write(uint_fast8_t value) {
	gpio_clear(PORT_AD_CS, PIN_AD_CS);
	ssp_transfer(SSP1_NUM, value);
	gpio_set(PORT_AD_CS, PIN_AD_CS);
}

/* Set MAX5864 operation mode to "Shutdown":
 *   REF: off
 *   CLK: off
 *   ADCs: off (bus is tri-stated)
 *   DACs: off (set input bus to zero or OVdd)
 */
void max5864_shutdown()
{
	max5864_spi_write(0x00);
}

/* Set MAX5864 operation mode to "Standby":
 *   REF: on
 *   CLK: off?
 *   ADCs: off (bus is tri-stated)
 *   DACs: off (set input bus to zero or OVdd)
 */
void max5864_standby()
{
	max5864_spi_write(0x05);
}

/* Set MAX5864 operation mode to "Idle":
 *   REF: on
 *   CLK: on
 *   ADCs: off (bus is tri-stated)
 *   DACs: off (set input bus to zero or OVdd)
 */
void max5864_idle()
{
	max5864_spi_write(0x01);
}

/* Set MAX5864 operation mode to "Rx":
 *   REF: on
 *   CLK: on
 *   ADCs: on
 *   DACs: off (set input bus to zero or OVdd)
 */
void max5864_rx()
{
	max5864_spi_write(0x02);
}

/* Set MAX5864 operation mode to "Tx":
 *   REF: on
 *   CLK: on
 *   ADCs: off (bus is tri-stated)
 *   DACs: on
 */
void max5864_tx()
{
	max5864_spi_write(0x03);
}

/* Set MAX5864 operation mode to "Xcvr":
 *   REF: on
 *   CLK: on
 *   ADCs: on
 *   DACs: on
 */
void max5864_xcvr()
{
	max5864_spi_write(0x04);
}
