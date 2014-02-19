/*
 * Copyright 2012 Jared Boone
 * Copyright 2013 Benjamin Vernoux
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

#include "usb_api_spiflash.h"

#include "usb_queue.h"

#include <stddef.h>

#include <w25q80bv.h>

uint8_t spiflash_buffer[W25Q80BV_PAGE_LEN];

usb_request_status_t usb_vendor_request_erase_spiflash(
		usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage)
{
	//FIXME This should refuse to run if executing from SPI flash.

	if (stage == USB_TRANSFER_STAGE_SETUP) {
		w25q80bv_setup();
		/* only chip erase is implemented */
		w25q80bv_chip_erase();
		usb_transfer_schedule_ack(endpoint->in);
		//FIXME probably should undo w25q80bv_setup()
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_write_spiflash(
	usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage)
{
	uint32_t addr = 0;
	uint16_t len = 0;

	//FIXME This should refuse to run if executing from SPI flash.

	if (stage == USB_TRANSFER_STAGE_SETUP) {
		addr = (endpoint->setup.value << 16) | endpoint->setup.index;
		len = endpoint->setup.length;
		if ((len > W25Q80BV_PAGE_LEN) || (addr > W25Q80BV_NUM_BYTES)
				|| ((addr + len) > W25Q80BV_NUM_BYTES)) {
			return USB_REQUEST_STATUS_STALL;
		} else {
			usb_transfer_schedule_block(endpoint->out, &spiflash_buffer[0], len,
						    NULL, NULL);
			w25q80bv_setup();
			return USB_REQUEST_STATUS_OK;
		}
	} else if (stage == USB_TRANSFER_STAGE_DATA) {
		addr = (endpoint->setup.value << 16) | endpoint->setup.index;
		len = endpoint->setup.length;
		/* This check is redundant but makes me feel better. */
		if ((len > W25Q80BV_PAGE_LEN) || (addr > W25Q80BV_NUM_BYTES)
				|| ((addr + len) > W25Q80BV_NUM_BYTES)) {
			return USB_REQUEST_STATUS_STALL;
		} else {
			w25q80bv_program(addr, len, &spiflash_buffer[0]);
			usb_transfer_schedule_ack(endpoint->in);
			//FIXME probably should undo w25q80bv_setup()
			return USB_REQUEST_STATUS_OK;
		}
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_read_spiflash(
	usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage)
{
	uint32_t i;
	uint32_t addr;
	uint16_t len;
	uint8_t* u8_addr_pt;

	if (stage == USB_TRANSFER_STAGE_SETUP) 
	{
		addr = (endpoint->setup.value << 16) | endpoint->setup.index;
		len = endpoint->setup.length;
		if ((len > W25Q80BV_PAGE_LEN) || (addr > W25Q80BV_NUM_BYTES)
			    || ((addr + len) > W25Q80BV_NUM_BYTES)) {
			return USB_REQUEST_STATUS_STALL;
		} else {
			/* TODO flush SPIFI "cache" before to read the SPIFI memory */
			u8_addr_pt = (uint8_t*)(addr + SPIFI_DATA_UNCACHED_BASE);
			for(i=0; i<len; i++)
			{
				spiflash_buffer[i] = u8_addr_pt[i];
			}
			usb_transfer_schedule_block(endpoint->in, &spiflash_buffer[0], len,
						    NULL, NULL);
			return USB_REQUEST_STATUS_OK;
		}
	} else if (stage == USB_TRANSFER_STAGE_DATA) 
	{
			addr = (endpoint->setup.value << 16) | endpoint->setup.index;
			len = endpoint->setup.length;
			/* This check is redundant but makes me feel better. */
			if ((len > W25Q80BV_PAGE_LEN) || (addr > W25Q80BV_NUM_BYTES)
					|| ((addr + len) > W25Q80BV_NUM_BYTES)) 
			{
				return USB_REQUEST_STATUS_STALL;
			} else
			{
				usb_transfer_schedule_ack(endpoint->out);
				return USB_REQUEST_STATUS_OK;
			}
	} else 
	{
		return USB_REQUEST_STATUS_OK;
	}
}

