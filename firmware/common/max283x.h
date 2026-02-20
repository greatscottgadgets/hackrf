/*
 * Copyright 2012-2022 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2012 Will Code <willcode4@gmail.com>
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

#ifndef __MAX283x_H
#define __MAX283x_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "gpio.h"
#include "gpio_lpc.h"
#include "max2831.h"
#include "max2831_target.h"
#include "max2837.h"
#include "max2837_target.h"
#include "max2839.h"
#include "max2839_target.h"
#include "spi_bus.h"

typedef enum {
	MAX283x_MODE_SHUTDOWN,
	MAX283x_MODE_STANDBY,
	MAX283x_MODE_TX,
	MAX283x_MODE_RX,
	MAX283x_MODE_RX_CAL,
	MAX283x_MODE_TX_CAL,
	MAX283x_MODE_CLKOUT,
} max283x_mode_t;

typedef enum {
	MAX2831_VARIANT,
	MAX2837_VARIANT,
	MAX2839_VARIANT,
} max283x_variant_t;

typedef struct {
	max283x_variant_t type;

	union {
#if defined(PRALINE) || defined(HACKRF_ALL)
		max2831_driver_t max2831;
#endif
		max2837_driver_t max2837;
		max2839_driver_t max2839;
	} drv;
} max283x_driver_t;

/* Initialize chip. */
void max283x_setup(max283x_driver_t* const drv, max283x_variant_t type);

/* Read a register via SPI. Save a copy to memory and return
 * value. Mark clean. */
uint16_t max283x_reg_read(max283x_driver_t* const drv, uint8_t r);

/* Write value to register via SPI and save a copy to memory. Mark
 * clean. */
void max283x_reg_write(max283x_driver_t* const drv, uint8_t r, uint16_t v);

/* Write all dirty registers via SPI from memory. Mark all clean. Some
 * operations require registers to be written in a certain order. Use
 * provided routines for those operations. */
void max283x_regs_commit(max283x_driver_t* const drv);

max283x_mode_t max283x_mode(max283x_driver_t* const drv);
void max283x_set_mode(max283x_driver_t* const drv, const max283x_mode_t new_mode);

/* Turn on/off all chip functions. Does not control oscillator and CLKOUT */
void max283x_start(max283x_driver_t* const drv);
void max283x_stop(max283x_driver_t* const drv);

/* Set frequency in Hz. Frequency setting is a multi-step function
 * where order of register writes matters. */
void max283x_set_frequency(max283x_driver_t* const drv, uint32_t freq);
uint32_t max283x_set_lpf_bandwidth(
	max283x_driver_t* const drv,
	const uint32_t bandwidth_hz);

bool max283x_set_lna_gain(max283x_driver_t* const drv, const uint32_t gain_db);

bool max283x_set_vga_gain(max283x_driver_t* const drv, const uint32_t gain_db);
bool max283x_set_txvga_gain(max283x_driver_t* const drv, const uint32_t gain_db);

void max283x_tx(max283x_driver_t* const drv);
void max283x_rx(max283x_driver_t* const drv);
void max283x_tx_calibration(max283x_driver_t* const drv);
void max283x_rx_calibration(max283x_driver_t* const drv);

#endif // __MAX283x_H
