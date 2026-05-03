/*
 * Copyright 2012-2026 Great Scott Gadgets <info@greatscottgadgets.com>
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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "i2c_bus.h"

#define SI_INTDIV(x) (x * 128 - 512)

#define SI5351C_CACHED_REGS 188

typedef enum {
	SI5351C_MODE_FRAC = 0,
	SI5351C_MODE_INT = 1,
} si5351c_mode_t;

typedef enum {
	SI5351C_SRC_XTAL = 0,
	SI5351C_SRC_CLKIN = 1,
	SI5351C_SRC_MULTISYNTH_0_4 = 2,
	SI5351C_SRC_MULTISYNTH_SELF = 3,
} si5351c_src_t;

typedef enum {
	SI5351C_DRIVE_2MA = 0,
	SI5351C_DRIVE_4MA = 1,
	SI5351C_DRIVE_6MA = 2,
	SI5351C_DRIVE_8MA = 3,
} si5351c_drive_t;

typedef enum {
	SI5351C_PLL_A = 0,
	SI5351C_PLL_B = 1,
} si5351c_pll_t;

typedef enum {
	SI5351C_PLL_MASK_A = 1,
	SI5351C_PLL_MASK_B = 2,
	SI5351C_PLL_MASK_BOTH = 3,
} si5351c_pll_mask_t;

typedef enum {
	SI5351C_XTAL_6PF = 1,
	SI5351C_XTAL_8PF = 2,
	SI5351C_XTAL_10PF = 3,
} si5351c_xtal_t;

typedef enum {
	SI5351C_OUTPUT_ENABLE = 0,
	SI5351C_OUTPUT_DISABLE = 1,
} si5351c_output_t;

typedef enum {
	SI5351C_DIV_1 = 0,
	SI5351C_DIV_2 = 1,
	SI5351C_DIV_4 = 2,
	SI5351C_DIV_8 = 3,
} si5351c_div_t;

enum pll_sources {
	PLL_SOURCE_UNINITIALIZED = -1,
	PLL_SOURCE_XTAL = 0,
	PLL_SOURCE_CLKIN = 1,
};

typedef struct {
	i2c_bus_t* const bus;
	uint8_t i2c_address;
	uint8_t regs[SI5351C_CACHED_REGS];
	uint32_t regs_dirty[(SI5351C_CACHED_REGS + 31) / 32];
} si5351c_driver_t;

typedef struct {
	bool power_down;
	si5351c_mode_t mode;
	si5351c_pll_t pll;
	si5351c_src_t source;
	si5351c_drive_t drive;
	bool invert;
} si5351c_clk_ctrl_t;

void si5351c_disable_all_outputs(si5351c_driver_t* const drv);
void si5351c_disable_oeb_pin_control(si5351c_driver_t* const drv);
void si5351c_power_down_all_clocks(si5351c_driver_t* const drv);
void si5351c_set_crystal_configuration(si5351c_driver_t* const drv);
void si5351c_enable_xo_and_ms_fanout(si5351c_driver_t* const drv);
void si5351c_configure_pll_sources(
	si5351c_driver_t* const drv,
	const enum pll_sources source);
void si5351c_configure_pll_multisynth(
	si5351c_driver_t* const drv,
	const enum pll_sources source);
void si5351c_reset_plls(si5351c_driver_t* const drv, si5351c_pll_mask_t mask);
void si5351c_configure_multisynth(
	si5351c_driver_t* const drv,
	const uint_fast8_t ms_number,
	const uint32_t p1,
	const uint32_t p2,
	const uint32_t p3,
	const uint_fast8_t r_div);
void si5351c_configure_clock_control(si5351c_driver_t* const drv);
void si5351c_enable_clock_outputs(si5351c_driver_t* const drv);
void si5351c_set_int_mode(
	si5351c_driver_t* const drv,
	const uint_fast8_t ms_number,
	const uint_fast8_t on);
void si5351c_set_clock_source(si5351c_driver_t* const drv, const enum pll_sources source);
bool si5351c_clkin_signal_valid(si5351c_driver_t* const drv);

void si5351c_write_single(si5351c_driver_t* const drv, uint8_t reg, uint8_t val);
uint8_t si5351c_read_single(si5351c_driver_t* const drv, uint8_t reg);
void si5351c_clkout_enable(si5351c_driver_t* const drv, uint8_t enable);
void si5351c_init(si5351c_driver_t* const drv);
void si5351c_set_phase(
	si5351c_driver_t* const drv,
	const uint8_t ms_number,
	const uint8_t offset);

#ifdef __cplusplus
}
#endif
