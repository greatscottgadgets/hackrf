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

#include "max5864.h"

static void max5864_write(max5864_driver_t* const drv, uint8_t value) {
	spi_bus_transfer(drv->bus, &value, 1);
}

static void max5864_init(max5864_driver_t* const drv) {
	drv->target_init(drv);
}

void max5864_setup(max5864_driver_t* const drv) {
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
