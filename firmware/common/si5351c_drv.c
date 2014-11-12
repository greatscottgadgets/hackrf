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

static void i2c0_transfer(si5351c_driver_t* const drv,
	const uint_fast8_t address,
	const uint8_t* const data_tx, const size_t count_tx,
	uint8_t* const data_rx, const size_t count_rx
) {
	(void)drv;
	
	i2c0_tx_start();
	i2c0_tx_byte((address << 1) | I2C_WRITE);
	for(size_t i=0; i<count_tx; i++) {
		i2c0_tx_byte(data_tx[i]);
	}

	if( data_rx ) {
		i2c0_tx_start();
		i2c0_tx_byte((address << 1) | I2C_READ);
		for(size_t i=0; i<count_rx; i++) {
			data_rx[i] = i2c0_rx_byte();
		}
	}

	i2c0_stop();
}

/* write to single register */
void si5351c_write_single(si5351c_driver_t* const drv, uint8_t reg, uint8_t val)
{
	const uint8_t data_tx[] = { reg, val };
	si5351c_write(drv, data_tx, 2);
}

/* read single register */
uint8_t si5351c_read_single(si5351c_driver_t* const drv, uint8_t reg)
{
	const uint8_t data_tx[] = { reg };
	uint8_t data_rx[] = { 0x00 };
	i2c0_transfer(drv, drv->i2c_address, data_tx, 1, data_rx, 1);
	return data_rx[0];
}

/*
 * Write to one or more contiguous registers. data[0] should be the first
 * register number, one or more values follow.
 */
void si5351c_write(si5351c_driver_t* const drv, const uint8_t* const data, const size_t data_count)
{
	i2c0_transfer(drv, drv->i2c_address, data, data_count, NULL, 0);
}
