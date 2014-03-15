/*
 * Copyright 2012 Michael Ossmann <mike@ossmann.com>
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

#ifndef __SI5351C_H
#define __SI5351C_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define SI_INTDIV(x)  (x*128-512)
#define SI5351C_I2C_ADDR (0x60 << 1)

#define SI5351C_CLK_POWERDOWN	(1<<7)
#define SI5351C_CLK_INT_MODE	(1<<6)
#define SI5351C_CLK_FRAC_MODE	(0<<6)

#define SI5351C_CLK_PLL_SRC(x)	(x<<5)
#define SI5351C_CLK_PLL_SRC_A	0
#define SI5351C_CLK_PLL_SRC_B	1

#define SI5351C_CLK_INV			(1<<4)

#define SI5351C_CLK_SRC(x)		(x<<2)
#define SI5351C_CLK_SRC_XTAL			0
#define SI5351C_CLK_SRC_CLKIN			1
#define SI5351C_CLK_SRC_MULTISYNTH_0_4	2
#define SI5351C_CLK_SRC_MULTISYNTH_SELF	3

#define SI5351C_CLK_IDRV(x) (x<<0)
#define SI5351C_CLK_IDRV_2MA 0
#define SI5351C_CLK_IDRV_4MA 1
#define SI5351C_CLK_IDRV_6MA 2
#define SI5351C_CLK_IDRV_8MA 3

#define SI5351C_LOS (1<<4)

enum pll_sources {
	PLL_SOURCE_XTAL = 0,
	PLL_SOURCE_CLKIN = 1,
};

void si5351c_disable_all_outputs();
void si5351c_disable_oeb_pin_control();
void si5351c_power_down_all_clocks();
void si5351c_set_crystal_configuration();
void si5351c_enable_xo_and_ms_fanout();
void si5351c_configure_pll_sources(void);
void si5351c_configure_pll_multisynth(void);
void si5351c_reset_pll(void);
void si5351c_configure_multisynth(const uint_fast8_t ms_number,
    	const uint32_t p1, const uint32_t p2, const uint32_t p3,
    	const uint_fast8_t r_div);
void si5351c_configure_clock_control(const enum pll_sources source);
void si5351c_enable_clock_outputs();
void si5351c_set_int_mode(const uint_fast8_t ms_number, const uint_fast8_t on);

void si5351c_write_single(uint8_t reg, uint8_t val);
uint8_t si5351c_read_single(uint8_t reg);
void si5351c_write(uint8_t* const data, const uint_fast8_t data_count);
void si5351c_set_clock_source(const enum pll_sources source);
void si5351c_activate_best_clock_source(void);

#ifdef __cplusplus
}
#endif

#endif /* __SI5351C_H */
