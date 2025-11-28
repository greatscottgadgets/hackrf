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

#ifndef __FPGA_H
#define __FPGA_H

#include <stdbool.h>

/* Up to 5 registers, each containing up to 8 bits of data */
#define FPGA_NUM_REGS            5
#define FPGA_DATA_REGS_MAX_VALUE 255

struct fpga_driver_t;
typedef struct fpga_driver_t fpga_driver_t;

struct fpga_driver_t {
	ice40_spi_driver_t* bus;
	uint8_t regs[FPGA_NUM_REGS];
	uint8_t regs_dirty;
};

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

void fpga_hw_sync_enable(const hw_sync_mode_t hw_sync_mode);

bool fpga_image_load(unsigned int index);
bool fpga_spi_selftest();
bool fpga_sgpio_selftest();
bool fpga_if_xcvr_selftest();

#endif // __FPGA_H
