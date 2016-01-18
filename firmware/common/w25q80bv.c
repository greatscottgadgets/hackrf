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
#include <stddef.h>

#include "w25q80bv.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define W25Q80BV_READ_DATA    0x03
#define W25Q80BV_FAST_READ    0x0b
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

	drv->page_len = 256U;
	drv->num_pages = 4096U;
	drv->num_bytes = 1048576U;

	drv->target_init(drv);

	device_id = 0;
	while(device_id != W25Q80BV_DEVICE_ID_RES)
	{
		device_id = w25q80bv_get_device_id(drv);
	}
}

uint8_t w25q80bv_get_status(w25q80bv_driver_t* const drv)
{
	uint8_t data[] = { W25Q80BV_READ_STATUS1, 0xFF };
	spi_bus_transfer(drv->bus, data, ARRAY_SIZE(data));
	return data[1];
}

/* Release power down / Device ID */  
uint8_t w25q80bv_get_device_id(w25q80bv_driver_t* const drv)
{
	uint8_t data[] = {
		W25Q80BV_DEVICE_ID,
		0xFF, 0xFF, 0xFF, 0xFF
	};
	spi_bus_transfer(drv->bus, data, ARRAY_SIZE(data));
	return data[4];
}

void w25q80bv_get_unique_id(w25q80bv_driver_t* const drv, w25q80bv_unique_id_t* unique_id)
{
	uint8_t data[] = {
		W25Q80BV_UNIQUE_ID,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	};
	spi_bus_transfer(drv->bus, data, ARRAY_SIZE(data));

	for(size_t i=0; i<8; i++) {
		unique_id->id_8b[i]  = data[5+i];
	}
}

void w25q80bv_wait_while_busy(w25q80bv_driver_t* const drv)
{
	while (w25q80bv_get_status(drv) & W25Q80BV_STATUS_BUSY);
}

void w25q80bv_write_enable(w25q80bv_driver_t* const drv)
{
	w25q80bv_wait_while_busy(drv);

	uint8_t data[] = { W25Q80BV_WRITE_ENABLE };
	spi_bus_transfer(drv->bus, data, ARRAY_SIZE(data));
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

	uint8_t data[] = { W25Q80BV_CHIP_ERASE };
	spi_bus_transfer(drv->bus, data, ARRAY_SIZE(data));
}

/* write up a 256 byte page or partial page */
static void w25q80bv_page_program(w25q80bv_driver_t* const drv, const uint32_t addr, const uint16_t len, uint8_t* data)
{
	/* do nothing if asked to write beyond a page boundary */
	if (((addr & 0xFF) + len) > drv->page_len)
		return;

	/* do nothing if we would overflow the flash */
	if (addr > (drv->num_bytes - len))
		return;

	w25q80bv_write_enable(drv);
	w25q80bv_wait_while_busy(drv);

	uint8_t header[] = {
		W25Q80BV_PAGE_PROGRAM,
		(addr & 0xFF0000) >> 16,
		(addr & 0xFF00) >> 8,
		addr & 0xFF
	};

	const spi_transfer_t transfers[] = {
		{ header, ARRAY_SIZE(header) },
		{ data, len }
	};

	spi_bus_transfer_gather(drv->bus, transfers, ARRAY_SIZE(transfers));
}

/* write an arbitrary number of bytes */
void w25q80bv_program(w25q80bv_driver_t* const drv, uint32_t addr, uint32_t len, uint8_t* data)
{
	uint16_t first_block_len;
	uint8_t device_id;

	device_id = 0;
	while(device_id != W25Q80BV_DEVICE_ID_RES)
	{
		device_id = w25q80bv_get_device_id(drv);
	}	
	
	/* do nothing if we would overflow the flash */
	if ((len > drv->num_bytes) || (addr > drv->num_bytes)
			|| ((addr + len) > drv->num_bytes))
		return;

	/* handle start not at page boundary */
	first_block_len = drv->page_len - (addr % drv->page_len);
	if (len < first_block_len)
		first_block_len = len;
	if (first_block_len) {
		w25q80bv_page_program(drv, addr, first_block_len, data);
		addr += first_block_len;
		data += first_block_len;
		len -= first_block_len;
	}

	/* one page at a time on boundaries */
	while (len >= drv->page_len) {
		w25q80bv_page_program(drv, addr, drv->page_len, data);
		addr += drv->page_len;
		data += drv->page_len;
		len -= drv->page_len;
	}

	/* handle end not at page boundary */
	if (len) {
		w25q80bv_page_program(drv, addr, len, data);
	}
}

/* write an arbitrary number of bytes */
void w25q80bv_read(w25q80bv_driver_t* const drv, uint32_t addr, uint32_t len, uint8_t* const data)
{
	/* do nothing if we would overflow the flash */
	if ((len > drv->num_bytes) || (addr > drv->num_bytes)
			|| ((addr + len) > drv->num_bytes))
		return;

	w25q80bv_wait_while_busy(drv);

	uint8_t header[] = {
		W25Q80BV_FAST_READ,
		(addr & 0xFF0000) >> 16,
		(addr & 0xFF00) >> 8,
		addr & 0xFF,
		0x00
	};

	const spi_transfer_t transfers[] = {
		{ header, ARRAY_SIZE(header) },
		{ data, len }
	};

	spi_bus_transfer_gather(drv->bus, transfers, ARRAY_SIZE(transfers));
}
