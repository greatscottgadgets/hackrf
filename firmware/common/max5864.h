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

#ifndef __MAX5864_H
#define __MAX5864_H

#include "spi_bus.h"

struct max5864_driver_t;
typedef struct max5864_driver_t max5864_driver_t;

struct max5864_driver_t {
	spi_bus_t* const bus;
	void (*target_init)(max5864_driver_t* const drv);
};

void max5864_setup(max5864_driver_t* const drv);

void max5864_shutdown(max5864_driver_t* const drv);
void max5864_standby(max5864_driver_t* const drv);
void max5864_idle(max5864_driver_t* const drv);
void max5864_rx(max5864_driver_t* const drv);
void max5864_tx(max5864_driver_t* const drv);
void max5864_xcvr(max5864_driver_t* const drv);

#endif // __MAX5864_H
