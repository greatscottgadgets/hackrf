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

#include "usb_api_firmware.h"
#include "usb_api_mode.h"
#include <stddef.h>
#include <hackrf_core.h>
#include "usb_queue.h"
#include "usb_endpoint.h"
#include <usb.h>
#include <w25q80bv.h>
#include <cpld_jtag.h>

/* Buffer size == spi_flash.page_len */
uint8_t spiflash_buffer[256U];

usb_request_status_t usb_vendor_request_erase_spiflash(
		usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		spi_bus_start(spi_flash.bus, &ssp_config_w25q80bv);
		w25q80bv_setup(&spi_flash);
		/* only chip erase is implemented */
		w25q80bv_chip_erase(&spi_flash);
		usb_transfer_schedule_ack(endpoint->in);
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_write_spiflash(
	usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage)
{
	uint32_t addr = 0;
	uint16_t len = 0;

	if (stage == USB_TRANSFER_STAGE_SETUP) {
		addr = (endpoint->setup.value << 16) | endpoint->setup.index;
		len = endpoint->setup.length;
		if ((len > spi_flash.page_len) || (addr > spi_flash.num_bytes)
				|| ((addr + len) > spi_flash.num_bytes)) {
			return USB_REQUEST_STATUS_STALL;
		} else {
			usb_transfer_schedule_block(endpoint->out, &spiflash_buffer[0], len,
						    NULL, NULL);
			spi_bus_start(spi_flash.bus, &ssp_config_w25q80bv);
			w25q80bv_setup(&spi_flash);
			return USB_REQUEST_STATUS_OK;
		}
	} else if (stage == USB_TRANSFER_STAGE_DATA) {
		addr = (endpoint->setup.value << 16) | endpoint->setup.index;
		len = endpoint->setup.length;
		/* This check is redundant but makes me feel better. */
		if ((len > spi_flash.page_len) || (addr > spi_flash.num_bytes)
				|| ((addr + len) > spi_flash.num_bytes)) {
			return USB_REQUEST_STATUS_STALL;
		} else {
			w25q80bv_program(&spi_flash, addr, len, &spiflash_buffer[0]);
			usb_transfer_schedule_ack(endpoint->in);
			return USB_REQUEST_STATUS_OK;
		}
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_read_spiflash(
	usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage)
{
	uint32_t addr;
	uint16_t len;

	if (stage == USB_TRANSFER_STAGE_SETUP) 
	{
		addr = (endpoint->setup.value << 16) | endpoint->setup.index;
		len = endpoint->setup.length;
		if ((len > spi_flash.page_len) || (addr > spi_flash.num_bytes)
			    || ((addr + len) > spi_flash.num_bytes)) {
			return USB_REQUEST_STATUS_STALL;
		} else {
			w25q80bv_read(&spi_flash, addr, len, &spiflash_buffer[0]);
			usb_transfer_schedule_block(endpoint->in, &spiflash_buffer[0], len,
						    NULL, NULL);
			return USB_REQUEST_STATUS_OK;
		}
	} else if (stage == USB_TRANSFER_STAGE_DATA) 
	{
			addr = (endpoint->setup.value << 16) | endpoint->setup.index;
			len = endpoint->setup.length;
			/* This check is redundant but makes me feel better. */
			if ((len > spi_flash.page_len) || (addr > spi_flash.num_bytes)
					|| ((addr + len) > spi_flash.num_bytes)) 
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

uint8_t cpld_xsvf_buffer[512];
volatile bool cpld_wait = false;

static void cpld_buffer_refilled(void* user_data, unsigned int length)
{
	(void)user_data;
	(void)length;
	cpld_wait = false;
}

static void refill_cpld_buffer(void)
{
	cpld_wait = true;
	usb_transfer_schedule(
		&usb_endpoint_bulk_out,
		cpld_xsvf_buffer,
		sizeof(cpld_xsvf_buffer),
		cpld_buffer_refilled,
		NULL
		);

	// Wait until transfer finishes
	while (cpld_wait);
}

usb_request_status_t usb_vendor_request_cpld_update(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
		usb_endpoint_init(&usb_endpoint_bulk_out);
		set_hackrf_mode(HACKRF_MODE_CPLD);
		usb_transfer_schedule_ack(endpoint->in);
		return USB_REQUEST_STATUS_OK;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

void cpld_update(void)
{
	#define WAIT_LOOP_DELAY (6000000)
	int i;
	int error;

	usb_queue_flush_endpoint(&usb_endpoint_bulk_in);
	usb_queue_flush_endpoint(&usb_endpoint_bulk_out);

	refill_cpld_buffer();

	error = cpld_jtag_program(&jtag_cpld, sizeof(cpld_xsvf_buffer),
				  cpld_xsvf_buffer,
				  refill_cpld_buffer);
	if(error == 0)
	{
		/* blink LED1, LED2, and LED3 on success */
		while (1)
		{
			led_on(LED1);
			led_on(LED2);
			led_on(LED3);
			for (i = 0; i < WAIT_LOOP_DELAY; i++)  /* Wait a bit. */
				__asm__("nop");
			led_off(LED1);
			led_off(LED2);
			led_off(LED3);
			for (i = 0; i < WAIT_LOOP_DELAY; i++)  /* Wait a bit. */
				__asm__("nop");
		}
	}else
	{
		/* LED3 (Red) steady on error */
		led_on(LED3);
		while (1);
	}
}
