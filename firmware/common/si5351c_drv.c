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

#include "si5351c_drv.h"

#include <libopencm3/lpc43xx/i2c.h>

#define SI5351C_I2C_ADDR (0x60 << 1)

/* FIXME return i2c0 status from each function */

/* write to single register */
void si5351c_write_single(si5351c_driver_t* const drv, uint8_t reg, uint8_t val)
{
	i2c0_tx_start();
	i2c0_tx_byte((drv->i2c_address << 1) | I2C_WRITE);
	i2c0_tx_byte(reg);
	i2c0_tx_byte(val);
	i2c0_stop();
}

/* read single register */
uint8_t si5351c_read_single(si5351c_driver_t* const drv, uint8_t reg)
{
	uint8_t val;

	/* set register address with write */
	i2c0_tx_start();
	i2c0_tx_byte((drv->i2c_address << 1) | I2C_WRITE);
	i2c0_tx_byte(reg);

	/* read the value */
	i2c0_tx_start();
	i2c0_tx_byte((drv->i2c_address << 1) | I2C_READ);
	val = i2c0_rx_byte();
	i2c0_stop();

	return val;
}

/*
 * Write to one or more contiguous registers. data[0] should be the first
 * register number, one or more values follow.
 */
void si5351c_write(si5351c_driver_t* const drv, uint8_t* const data, const uint_fast8_t data_count)
{
	uint_fast8_t i;

	i2c0_tx_start();
	i2c0_tx_byte((drv->i2c_address << 1) | I2C_WRITE);
	
	for (i = 0; i < data_count; i++)
		i2c0_tx_byte(data[i]);
	i2c0_stop();
}
