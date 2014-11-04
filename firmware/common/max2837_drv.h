/*
 * Copyright 2012 Will Code? (TODO: Proper attribution)
 * Copyright 2014 Jared Boone <jared@sharebrained.com>
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

#ifndef __MAX2837_DRV_H
#define __MAX2837_DRV_H

#include <stdint.h>

typedef enum {
	MAX2837_MODE_SHUTDOWN,
	MAX2837_MODE_STANDBY,
	MAX2837_MODE_TX,
	MAX2837_MODE_RX
} max2837_mode_t;

void max2837_pin_config(void);
void max2837_mode_shutdown(void);
void max2837_mode_standby(void);
void max2837_mode_tx(void);
void max2837_mode_rx(void);
max2837_mode_t max2837_mode(void);

uint16_t max2837_spi_read(uint8_t r);
void max2837_spi_write(uint8_t r, uint16_t v);

#endif // __MAX2837_DRV_H
