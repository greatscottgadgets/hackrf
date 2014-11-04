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

#ifndef __MAX2837_H
#define __MAX2837_H

#include <stdint.h>
#include <stdbool.h>

#include "max2837_drv.h"

/* TODO - make this a private header for max2837.c only, make new max2837.h */

/* 32 registers, each containing 10 bits of data. */
#define MAX2837_NUM_REGS 32
#define MAX2837_DATA_REGS_MAX_VALUE 1024

/* TODO - these externs will be local to max2837.c ... don't define here? */
extern uint16_t max2837_regs[MAX2837_NUM_REGS];
extern uint32_t max2837_regs_dirty;

#define MAX2837_REG_SET_CLEAN(r) max2837_regs_dirty &= ~(1UL<<r)
#define MAX2837_REG_SET_DIRTY(r) max2837_regs_dirty |= (1UL<<r)

/* Initialize chip. */
extern void max2837_init(void);
extern void max2837_setup(void);

/* Read a register via SPI. Save a copy to memory and return
 * value. Mark clean. */
extern uint16_t max2837_reg_read(uint8_t r);

/* Write value to register via SPI and save a copy to memory. Mark
 * clean. */
extern void max2837_reg_write(uint8_t r, uint16_t v);

/* Write all dirty registers via SPI from memory. Mark all clean. Some
 * operations require registers to be written in a certain order. Use
 * provided routines for those operations. */
extern void max2837_regs_commit(void);

void max2837_mode_shutdown(void);
void max2837_mode_standby(void);
void max2837_mode_tx(void);
void max2837_mode_rx(void);

max2837_mode_t max2837_mode(void);
void max2837_set_mode(const max2837_mode_t new_mode);

/* Turn on/off all chip functions. Does not control oscillator and CLKOUT */
extern void max2837_start(void);
extern void max2837_stop(void);

/* Set frequency in Hz. Frequency setting is a multi-step function
 * where order of register writes matters. */
extern void max2837_set_frequency(uint32_t freq);
bool max2837_set_lpf_bandwidth(const uint32_t bandwidth_hz);
bool max2837_set_lna_gain(const uint32_t gain_db);
bool max2837_set_vga_gain(const uint32_t gain_db);
bool max2837_set_txvga_gain(const uint32_t gain_db);
	
extern void max2837_tx(void);
extern void max2837_rx(void);

#endif // __MAX2837_H
