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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "ice40_spi.h"

/* Up to 7 registers, each containing up to 8 bits of data */
#define FPGA_NUM_REGS            7
#define FPGA_DATA_REGS_MAX_VALUE 255

typedef enum {
	FPGA_QUARTER_SHIFT_MODE_NONE = 0b00,
	FPGA_QUARTER_SHIFT_MODE_UP = 0b11,
	FPGA_QUARTER_SHIFT_MODE_DOWN = 0b01,
} fpga_quarter_shift_mode_t;

typedef struct {
	ice40_spi_driver_t* bus;
	uint8_t regs[FPGA_NUM_REGS];
	uint8_t regs_dirty;
} fpga_driver_t;

struct fpga_loader_t {
	/* Start address added as an offset to all read() calls. */
	uint32_t start_addr;
	/* Any one-off setup needed before calling read(). May be NULL. */
	void (*setup)(void);
	/* Read data from the specified address. */
	void (*read)(uint32_t addr, uint32_t len, uint8_t* const data);
	/* Buffer to use for compressed data (4096 bytes). */
	uint8_t* in_buffer;
	/* Buffer to use for decompressed data (4096 bytes). */
	uint8_t* out_buffer;
};

/* Initialize the loaded bitstream's registers to their default values. */
extern void fpga_init(fpga_driver_t* const drv);

/* Initialize fpga and gateware. */
extern void fpga_setup(fpga_driver_t* const drv);

/* Read a register via SPI. Save a copy to memory and return
 * value. Mark clean. */
extern uint8_t fpga_reg_read(fpga_driver_t* const drv, uint8_t r);

/* Write value to register via SPI and save a copy to memory. Mark
 * clean. */
extern void fpga_reg_write(fpga_driver_t* const drv, uint8_t r, uint8_t v);

/* Write all dirty registers via SPI from memory. Mark all clean. Some
 * operations require registers to be written in a certain order. Use
 * provided routines for those operations. */
extern void fpga_regs_commit(fpga_driver_t* const drv);

void fpga_set_trigger_enable(fpga_driver_t* const drv, const bool enable);
void fpga_set_rx_dc_block_enable(fpga_driver_t* const drv, const bool enable);
void fpga_set_rx_decimation_ratio(fpga_driver_t* const drv, const uint8_t value);
void fpga_set_rx_quarter_shift_mode(
	fpga_driver_t* const drv,
	const fpga_quarter_shift_mode_t mode);
void fpga_set_tx_interpolation_ratio(fpga_driver_t* const drv, const uint8_t value);

void fpga_set_prbs_enable(fpga_driver_t* const drv, const bool enable);
void fpga_set_tx_nco_enable(fpga_driver_t* const drv, const bool enable);
void fpga_set_tx_nco_pstep(fpga_driver_t* const drv, const uint8_t phase_increment);

bool fpga_image_load(struct fpga_loader_t* loader, unsigned int index);
bool fpga_spi_selftest(void);
bool fpga_sgpio_selftest(void);
bool fpga_if_xcvr_selftest(void);

//void narrowband_filter_set(const uint8_t value);
