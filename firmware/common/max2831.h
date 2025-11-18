/*
 * Copyright 2025 Great Scott Gadgets <info@greatscottgadgets.com>
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

#ifndef __MAX2831_H
#define __MAX2831_H

#include <stdint.h>
#include <stdbool.h>

#include "gpio.h"
#include "spi_bus.h"

/* 16 registers, each containing 14 bits of data. */
#define MAX2831_NUM_REGS            16
#define MAX2831_DATA_REGS_MAX_VALUE 16384

typedef enum {
	MAX2831_MODE_SHUTDOWN,
	MAX2831_MODE_STANDBY,
	MAX2831_MODE_TX,
	MAX2831_MODE_RX,
	MAX2831_MODE_TX_CALIBRATION,
	MAX2831_MODE_RX_CALIBRATION,
} max2831_mode_t;

typedef enum {
	MAX2831_RX_HPF_100_HZ = 0,
	MAX2831_RX_HPF_4_KHZ = 1,
	MAX2831_RX_HPF_30_KHZ = 2,
	MAX2831_RX_HPF_600_KHZ = 3,
} max2831_rx_hpf_freq_t;

struct max2831_driver_t;
typedef struct max2831_driver_t max2831_driver_t;

struct max2831_driver_t {
	spi_bus_t* bus;
	gpio_t gpio_enable;
	gpio_t gpio_rxtx;
	gpio_t gpio_rxhp;
	gpio_t gpio_ld;
	void (*target_init)(max2831_driver_t* const drv);
	void (*set_mode)(max2831_driver_t* const drv, const max2831_mode_t new_mode);
	max2831_mode_t mode;
	uint16_t regs[MAX2831_NUM_REGS];
	uint16_t regs_dirty;
	uint32_t desired_lpf_bw;
};

typedef struct {
	uint32_t bandwidth_hz;
	uint8_t ft;
} max2831_ft_t;

typedef struct {
	uint8_t percent;
	uint8_t ft_fine;
} max2831_ft_fine_t;

// dirty hack is dirty
extern const max2831_ft_t* max2831_rx_ft;
extern const max2831_ft_t* max2831_tx_ft;

/* Initialize chip. */
extern void max2831_setup(max2831_driver_t* const drv);

/* Read a register via SPI. Save a copy to memory and return
 * value. Mark clean. */
extern uint16_t max2831_reg_read(max2831_driver_t* const drv, uint8_t r);

/* Write value to register via SPI and save a copy to memory. Mark
 * clean. */
extern void max2831_reg_write(max2831_driver_t* const drv, uint8_t r, uint16_t v);

/* Write all dirty registers via SPI from memory. Mark all clean. Some
 * operations require registers to be written in a certain order. Use
 * provided routines for those operations. */
extern void max2831_regs_commit(max2831_driver_t* const drv);

max2831_mode_t max2831_mode(max2831_driver_t* const drv);
void max2831_set_mode(max2831_driver_t* const drv, const max2831_mode_t new_mode);

/* Turn on/off all chip functions. Does not control oscillator and CLKOUT */
extern void max2831_start(max2831_driver_t* const drv);
extern void max2831_stop(max2831_driver_t* const drv);

/* Set frequency in Hz. Frequency setting is a multi-step function
 * where order of register writes matters. */
extern void max2831_set_frequency(max2831_driver_t* const drv, uint32_t freq);
uint32_t max2831_set_lpf_bandwidth(
	max2831_driver_t* const drv,
	const uint32_t bandwidth_hz);
bool max2831_set_lna_gain(max2831_driver_t* const drv, const uint32_t gain_db);
bool max2831_set_vga_gain(max2831_driver_t* const drv, const uint32_t gain_db);
bool max2831_set_txvga_gain(max2831_driver_t* const drv, const uint32_t gain_db);

/* Set receiver high-pass filter corner frequency in Hz */
extern void max2831_set_rx_hpf_frequency(
	max2831_driver_t* const drv,
	const max2831_rx_hpf_freq_t freq);

extern void max2831_tx(max2831_driver_t* const drv);
extern void max2831_rx(max2831_driver_t* const drv);
extern void max2831_tx_calibration(max2831_driver_t* const drv);
extern void max2831_rx_calibration(max2831_driver_t* const drv);

#endif // __MAX2831_H
