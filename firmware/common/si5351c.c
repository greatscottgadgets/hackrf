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
#include <libopencm3/lpc43xx/i2c.h>

/* FIXME return i2c0 status from each function */

/* write to single register */
void si5351c_write_single(uint8_t reg, uint8_t val)
{
	i2c0_tx_start();
	i2c0_tx_byte(SI5351C_I2C_ADDR | I2C_WRITE);
	i2c0_tx_byte(reg);
	i2c0_tx_byte(val);
	i2c0_stop();
}

/* read single register */
uint8_t si5351c_read_single(uint8_t reg)
{
	uint8_t val;

	/* set register address with write */
	i2c0_tx_start();
	i2c0_tx_byte(SI5351C_I2C_ADDR | I2C_WRITE);
	i2c0_tx_byte(reg);

	/* read the value */
	i2c0_tx_start();
	i2c0_tx_byte(SI5351C_I2C_ADDR | I2C_READ);
	val = i2c0_rx_byte();
	i2c0_stop();

	return val;
}

/*
 * Write to one or more contiguous registers. data[0] should be the first
 * register number, one or more values follow.
 */
void si5351c_write(uint8_t* const data, const uint_fast8_t data_count)
{
	uint_fast8_t i;

	i2c0_tx_start();
	i2c0_tx_byte(SI5351C_I2C_ADDR | I2C_WRITE);
	
	for (i = 0; i < data_count; i++)
		i2c0_tx_byte(data[i]);
	i2c0_stop();
}

/* Disable all CLKx outputs. */
void si5351c_disable_all_outputs()
{
	uint8_t data[] = { 3, 0xFF };
	si5351c_write(data, sizeof(data));
}

/* Turn off OEB pin control for all CLKx */
void si5351c_disable_oeb_pin_control()
{
	uint8_t data[] = { 9, 0xFF };
	si5351c_write(data, sizeof(data));
}

/* Power down all CLKx */
void si5351c_power_down_all_clocks()
{
	uint8_t data[] = { 16, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xC0, 0xC0 };
	si5351c_write(data, sizeof(data));
}

/*
 * Register 183: Crystal Internal Load Capacitance
 * Reads as 0xE4 on power-up
 * Set to 10pF (until I find out what loading the crystal/PCB likes best)
 */
void si5351c_set_crystal_configuration()
{
	uint8_t data[] = { 183, 0xE4 };
	si5351c_write(data, sizeof(data));
}

/*
 * Register 187: Fanout Enable
 * Turn on XO and MultiSynth fanout only.
 */
void si5351c_enable_xo_and_ms_fanout()
{
	uint8_t data[] = { 187, 0x50 };
	si5351c_write(data, sizeof(data));
}

/*
 * Register 15: PLL Input Source
 * CLKIN_DIV=0 (Divide by 1)
 * PLLB_SRC=0 (XTAL input)
 * PLLA_SRC=0 (XTAL input)
 */
void si5351c_configure_pll_sources_for_xtal()
{
	uint8_t data[] = { 15, 0x00 };
	si5351c_write(data, sizeof(data));
}

/* MultiSynth NA (PLL1) */
void si5351c_configure_pll1_multisynth()
{
	uint8_t data[] = { 26, 0x00, 0x01, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00 };
	si5351c_write(data, sizeof(data));
}

void si5351c_configure_multisynth(const uint_fast8_t ms_number,
		const uint32_t p1, const uint32_t p2, const uint32_t p3,
    	const uint_fast8_t r_div)
{
	/*
	 * TODO: Check for p3 > 0? 0 has no meaning in fractional mode?
	 * And it makes for more jitter in integer mode.
	 */
	/*
	 * r is the r divider value encoded:
	 *   0 means divide by 1
	 *   1 means divide by 2
	 *   2 means divide by 4
	 *   ...
	 *   7 means divide by 128
	 */
	const uint_fast8_t register_number = 42 + (ms_number * 8);
	uint8_t data[] = {
			register_number,
			(p3 >> 8) & 0xFF,
			(p3 >> 0) & 0xFF,
			(r_div << 4) | (0 << 2) | ((p1 >> 16) & 0x3),
			(p1 >> 8) & 0xFF,
			(p1 >> 0) & 0xFF,
			(((p3 >> 16) & 0xF) << 4) | (((p2 >> 16) & 0xF) << 0),
			(p2 >> 8) & 0xFF,
			(p2 >> 0) & 0xFF };
	si5351c_write(data, sizeof(data));
}

#ifdef JELLYBEAN
/*
 * Registers 16 through 23: CLKx Control
 * CLK0:
 *   CLK0_PDN=0 (powered up)
 *   MS0_INT=1 (integer mode)
 *   MS0_SRC=0 (PLLA as source for MultiSynth 0)
 *   CLK0_INV=0 (not inverted)
 *   CLK0_SRC=3 (MS0 as input source)
 *   CLK0_IDRV=3 (8mA)
 * CLK1:
 *   CLK1_PDN=0 (powered up)
 *   MS1_INT=1 (integer mode)
 *   MS1_SRC=0 (PLLA as source for MultiSynth 1)
 *   CLK1_INV=0 (not inverted)
 *   CLK1_SRC=2 (MS0 as input source)
 *   CLK1_IDRV=3 (8mA)
 * CLK2:
 *   CLK2_PDN=0 (powered up)
 *   MS2_INT=1 (integer mode)
 *   MS2_SRC=0 (PLLA as source for MultiSynth 2)
 *   CLK2_INV=0 (not inverted)
 *   CLK2_SRC=2 (MS0 as input source)
 *   CLK2_IDRV=3 (8mA)
 * CLK3:
 *   CLK3_PDN=0 (powered up)
 *   MS3_INT=1 (integer mode)
 *   MS3_SRC=0 (PLLA as source for MultiSynth 3)
 *   CLK3_INV=0 (inverted)
 *   CLK3_SRC=2 (MS0 as input source)
 *   CLK3_IDRV=3 (8mA)
 * CLK4:
 *   CLK4_PDN=0 (powered up)
 *   MS4_INT=0 (fractional mode -- to support 12MHz to LPC for USB DFU)
 *   MS4_SRC=0 (PLLA as source for MultiSynth 4)
 *   CLK4_INV=0 (not inverted)
 *   CLK4_SRC=3 (MS4 as input source)
 *   CLK4_IDRV=3 (8mA)
 * CLK5:
 *   CLK5_PDN=0 (powered up)
 *   MS5_INT=1 (integer mode)
 *   MS5_SRC=0 (PLLA as source for MultiSynth 5)
 *   CLK5_INV=0 (not inverted)
 *   CLK5_SRC=3 (MS5 as input source)
 *   CLK5_IDRV=3 (8mA)
 */
void si5351c_configure_clock_control()
{
	uint8_t data[] = { 16, 0x4F, 0x4B, 0x4B, 0x4B, 0x0F, 0x4F, 0xC0, 0xC0 };
	si5351c_write(data, sizeof(data));
}
#endif

#ifdef JAWBREAKER
/*
 * Registers 16 through 23: CLKx Control
 * CLK0:
 *   CLK0_PDN=0 (powered up)
 *   MS0_INT=1 (integer mode)
 *   MS0_SRC=0 (PLLA as source for MultiSynth 0)
 *   CLK0_INV=0 (not inverted)
 *   CLK0_SRC=3 (MS0 as input source)
 *   CLK0_IDRV=3 (8mA)
 * CLK1:
 *   CLK1_PDN=0 (powered up)
 *   MS1_INT=1 (integer mode)
 *   MS1_SRC=0 (PLLA as source for MultiSynth 1)
 *   CLK1_INV=0 (not inverted)
 *   CLK1_SRC=2 (MS0 as input source)
 *   CLK1_IDRV=3 (8mA)
 * CLK2:
 *   CLK2_PDN=0 (powered up)
 *   MS2_INT=1 (integer mode)
 *   MS2_SRC=0 (PLLA as source for MultiSynth 2)
 *   CLK2_INV=0 (not inverted)
 *   CLK2_SRC=2 (MS0 as input source)
 *   CLK2_IDRV=3 (8mA)
 * CLK3:
 *   CLK3_PDN=0 (powered up)
 *   MS3_INT=1 (integer mode)
 *   MS3_SRC=0 (PLLA as source for MultiSynth 3)
 *   CLK3_INV=0 (inverted)
 *   CLK3_SRC=2 (MS0 as input source)
 *   CLK3_IDRV=3 (8mA)
 * CLK4:
 *   CLK4_PDN=0 (powered up)
 *   MS4_INT=1 (integer mode)
 *   MS4_SRC=0 (PLLA as source for MultiSynth 4)
 *   CLK4_INV=0 (not inverted)
 *   CLK4_SRC=3 (MS4 as input source)
 *   CLK4_IDRV=3 (8mA)
 * CLK5:
 *   CLK5_PDN=0 (powered up)
 *   MS5_INT=1 (integer mode)
 *   MS5_SRC=0 (PLLA as source for MultiSynth 5)
 *   CLK5_INV=0 (not inverted)
 *   CLK5_SRC=3 (MS5 as input source)
 *   CLK5_IDRV=3 (8mA)
 * CLK6: (not connected)
 *   CLK5_PDN=1 (powered down)
 *   MS5_INT=1 (integer mode)
 * CLK7: (not connected)
 *   CLK7_PDN=1 (powered down)
 *   MS7_INT=0 (fractional mode -- to support 12MHz to LPC)
 */
void si5351c_configure_clock_control()
{
	uint8_t data[] = { 16, 0x4F, 0x4B, 0x4B, 0x4B, 0x4F, 0x4F, 0xC0, 0x80 };
	si5351c_write(data, sizeof(data));
}
#endif

/* Enable CLK outputs 0, 1, 2, 3, 4, 5 only. */
void si5351c_enable_clock_outputs()
{
	uint8_t data[] = { 3, 0xC0 };
	si5351c_write(data, sizeof(data));
}
