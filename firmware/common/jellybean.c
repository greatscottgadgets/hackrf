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

#include "si5351c.h"

/* initial configuration for Jellybean with Lemondrop attached */ 
void jellybean_init(void)
{
	//FIXME I2C setup

	si5351c_disable_all_outputs();
	si5351c_disable_oeb_pin_control();
	si5351c_power_down_all_clocks();
	si5351c_set_crystal_configuration();
	si5351c_enable_xo_and_ms_fanout();
	si5351c_configure_pll_sources_for_xtal();
	si5351c_configure_pll1_multisynth();

	/* MS0/CLK0 is the source for the MAX2837 clock input. */
	si5351c_configure_multisynth(0, 2048, 0, 1);   /* 40MHz */

	/* MS1/CLK1 is the source for the MAX5864 codec. */
	si5351c_configure_multisynth(1, 4608, 0, 1);   /* 20MHz */

	/* MS4/CLK4 is the source for the LPC43xx microcontroller. */
	si5351c_configure_multisynth(4, 8021, 1, 3);   /* 12MHz */

	si5351c_configure_clock_control();
	si5351c_enable_clock_outputs();

	//FIXME disable I2C

	/*
	 * 12MHz clock is entering LPC XTAL1/OSC input now.
	 * Set up PLL1 to run from XTAL1 input.
	 */

	//FIXME CGU setup
}
