/*
 * Copyright 2013 Michael Ossmann
 * Copyright 2013 Benjamin Vernoux
 * Copyright 2014 Jared Boone, ShareBrained Technology
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

/*
 * This is a rudimentary driver for the W25Q80BV SPI Flash IC using the
 * LPC43xx's SSP0 peripheral (not quad SPIFI).  The only goal here is to allow
 * programming the flash.
 */

#include <stdint.h>
#include "w25q80bv.h"
#include "w25q80bv_drv.h"

#define W25Q80BV_WRITE_ENABLE 0x06
#define W25Q80BV_CHIP_ERASE   0xC7
#define W25Q80BV_READ_STATUS1 0x05
#define W25Q80BV_PAGE_PROGRAM 0x02
#define W25Q80BV_DEVICE_ID    0xAB
#define W25Q80BV_UNIQUE_ID    0x4B

#define W25Q80BV_STATUS_BUSY  0x01

#define W25Q80BV_DEVICE_ID_RES  0x13 /* Expected device_id for W25Q80BV */

/*
 * Set up pins for GPIO and SPI control, configure SSP0 peripheral for SPI.
 * SSP0_SSEL is controlled by GPIO in order to handle various transfer lengths.
 */

void w25q80bv_setup(w25q80bv_driver_t* const drv)
{
	uint8_t device_id;

	w25q80bv_spi_init(drv);

	device_id = 0;
	while(device_id != W25Q80BV_DEVICE_ID_RES)
	{
		device_id = w25q80bv_get_device_id(drv);
	}
}

uint8_t w25q80bv_get_status(w25q80bv_driver_t* const drv)
{
	uint8_t value;

	w25q80bv_spi_select(drv);
	w25q80bv_spi_transfer(drv, W25Q80BV_READ_STATUS1);
	value = w25q80bv_spi_transfer(drv, 0xFF);
	w25q80bv_spi_unselect(drv);

	return value;
}

/* Release power down / Device ID */  
uint8_t w25q80bv_get_device_id(w25q80bv_driver_t* const drv)
{
	uint8_t value;

	w25q80bv_spi_select(drv);
	w25q80bv_spi_transfer(drv, W25Q80BV_DEVICE_ID);

	/* Read 3 dummy bytes */
	value = w25q80bv_spi_transfer(drv, 0xFF);
	value = w25q80bv_spi_transfer(drv, 0xFF);
	value = w25q80bv_spi_transfer(drv, 0xFF);
	/* Read Device ID shall return 0x13 for W25Q80BV */
	value = w25q80bv_spi_transfer(drv, 0xFF);
	
	w25q80bv_spi_unselect(drv);

	return value;
}

void w25q80bv_get_unique_id(w25q80bv_driver_t* const drv, w25q80bv_unique_id_t* unique_id)
{
	int i;
	uint8_t value;

	w25q80bv_spi_select(drv);
	w25q80bv_spi_transfer(drv, W25Q80BV_UNIQUE_ID);

	/* Read 4 dummy bytes */
	for(i=0; i<4; i++)
		value = w25q80bv_spi_transfer(drv, 0xFF);

	/* Read Unique ID 64bits (8*8) */
	for(i=0; i<8; i++)
	{
		value = w25q80bv_spi_transfer(drv, 0xFF);
		unique_id->id_8b[i]  = value;
	}

	w25q80bv_spi_unselect(drv);
}

void w25q80bv_wait_while_busy(w25q80bv_driver_t* const drv)
{
	while (w25q80bv_get_status(drv) & W25Q80BV_STATUS_BUSY);
}

void w25q80bv_write_enable(w25q80bv_driver_t* const drv)
{
	w25q80bv_wait_while_busy(drv);
	w25q80bv_spi_select(drv);
	w25q80bv_spi_transfer(drv, W25Q80BV_WRITE_ENABLE);
	w25q80bv_spi_unselect(drv);
}

void w25q80bv_chip_erase(w25q80bv_driver_t* const drv)
{
	uint8_t device_id;

	device_id = 0;
	while(device_id != W25Q80BV_DEVICE_ID_RES)
	{
		device_id = w25q80bv_get_device_id(drv);
	}

	w25q80bv_write_enable(drv);
	w25q80bv_wait_while_busy(drv);
	w25q80bv_spi_select(drv);
	w25q80bv_spi_transfer(drv, W25Q80BV_CHIP_ERASE);
	w25q80bv_spi_unselect(drv);
}

/* write up a 256 byte page or partial page */
void w25q80bv_page_program(w25q80bv_driver_t* const drv, const uint32_t addr, const uint16_t len, const uint8_t* data)
{
	int i;

	/* do nothing if asked to write beyond a page boundary */
	if (((addr & 0xFF) + len) > W25Q80BV_PAGE_LEN)
		return;

	/* do nothing if we would overflow the flash */
	if (addr > (W25Q80BV_NUM_BYTES - len))
		return;

	w25q80bv_write_enable(drv);
	w25q80bv_wait_while_busy(drv);

	w25q80bv_spi_select(drv);
	w25q80bv_spi_transfer(drv, W25Q80BV_PAGE_PROGRAM);
	w25q80bv_spi_transfer(drv, (addr & 0xFF0000) >> 16);
	w25q80bv_spi_transfer(drv, (addr & 0xFF00) >> 8);
	w25q80bv_spi_transfer(drv, addr & 0xFF);
	for (i = 0; i < len; i++)
		w25q80bv_spi_transfer(drv, data[i]);
	w25q80bv_spi_unselect(drv);
}

/* write an arbitrary number of bytes */
void w25q80bv_program(w25q80bv_driver_t* const drv, uint32_t addr, uint32_t len, const uint8_t* data)
{
	uint16_t first_block_len;
	uint8_t device_id;

	device_id = 0;
	while(device_id != W25Q80BV_DEVICE_ID_RES)
	{
		device_id = w25q80bv_get_device_id(drv);
	}	
	
	/* do nothing if we would overflow the flash */
	if ((len > W25Q80BV_NUM_BYTES) || (addr > W25Q80BV_NUM_BYTES)
			|| ((addr + len) > W25Q80BV_NUM_BYTES))
		return;

	/* handle start not at page boundary */
	first_block_len = W25Q80BV_PAGE_LEN - (addr % W25Q80BV_PAGE_LEN);
	if (len < first_block_len)
		first_block_len = len;
	if (first_block_len) {
		w25q80bv_page_program(drv, addr, first_block_len, data);
		addr += first_block_len;
		data += first_block_len;
		len -= first_block_len;
	}

	/* one page at a time on boundaries */
	while (len >= W25Q80BV_PAGE_LEN) {
		w25q80bv_page_program(drv, addr, W25Q80BV_PAGE_LEN, data);
		addr += W25Q80BV_PAGE_LEN;
		data += W25Q80BV_PAGE_LEN;
		len -= W25Q80BV_PAGE_LEN;
	}

	/* handle end not at page boundary */
	if (len) {
		w25q80bv_page_program(drv, addr, len, data);
	}
}
