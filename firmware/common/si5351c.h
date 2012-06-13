/*
 * Copyright 2012 Michael Ossmann
 * Copyright 2012 Jared Boone
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

#define SI5351C_I2C_ADDR (0x60 << 1)

void si5351c_disable_all_outputs();
void si5351c_disable_oeb_pin_control();
void si5351c_power_down_all_clocks();
void si5351c_set_crystal_configuration();
void si5351c_enable_xo_and_ms_fanout();
void si5351c_configure_pll_sources_for_xtal();
void si5351c_configure_pll1_multisynth();
void si5351c_configure_multisynth(const uint_fast8_t ms_number,
    	const uint32_t p1, const uint32_t p2, const uint32_t p3,
		const uint_fast8_t r);
void si5351c_configure_clock_control();
void si5351c_enable_clock_outputs();

#ifdef __cplusplus
}
#endif

#endif /* __SI5351C_H */
