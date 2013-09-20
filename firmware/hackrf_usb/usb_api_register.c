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

#include "usb_api_register.h"

#include <usb_queue.h>
#include <max2837.h>
#include <si5351c.h>
#include <rffc5071.h>

#include <stddef.h>
#include <stdint.h>

usb_request_status_t usb_vendor_request_write_max2837(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
		if( endpoint->setup.index < MAX2837_NUM_REGS ) {
			if( endpoint->setup.value < MAX2837_DATA_REGS_MAX_VALUE ) {
				max2837_reg_write(endpoint->setup.index, endpoint->setup.value);
				usb_transfer_schedule_ack(endpoint->in);
				return USB_REQUEST_STATUS_OK;
			}
		}
		return USB_REQUEST_STATUS_STALL;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_read_max2837(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
		if( endpoint->setup.index < MAX2837_NUM_REGS ) {
			const uint16_t value = max2837_reg_read(endpoint->setup.index);
			endpoint->buffer[0] = value & 0xff;
			endpoint->buffer[1] = value >> 8;
			usb_transfer_schedule_block(endpoint->in, &endpoint->buffer, 2,
						    NULL, NULL);
			usb_transfer_schedule_ack(endpoint->out);
			return USB_REQUEST_STATUS_OK;
		}
		return USB_REQUEST_STATUS_STALL;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_write_si5351c(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
		if( endpoint->setup.index < 256 ) {
			if( endpoint->setup.value < 256 ) {
				si5351c_write_single(endpoint->setup.index, endpoint->setup.value);
				usb_transfer_schedule_ack(endpoint->in);
				return USB_REQUEST_STATUS_OK;
			}
		}
		return USB_REQUEST_STATUS_STALL;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_read_si5351c(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
		if( endpoint->setup.index < 256 ) {
			const uint8_t value = si5351c_read_single(endpoint->setup.index);
			endpoint->buffer[0] = value;
			usb_transfer_schedule_block(endpoint->in, &endpoint->buffer, 1,
						    NULL, NULL);
			usb_transfer_schedule_ack(endpoint->out);
			return USB_REQUEST_STATUS_OK;
		}
		return USB_REQUEST_STATUS_STALL;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_write_rffc5071(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	if( stage == USB_TRANSFER_STAGE_SETUP ) 
	{
		if( endpoint->setup.index < RFFC5071_NUM_REGS ) 
		{
			rffc5071_reg_write(endpoint->setup.index, endpoint->setup.value);
			usb_transfer_schedule_ack(endpoint->in);
			return USB_REQUEST_STATUS_OK;
		}
		return USB_REQUEST_STATUS_STALL;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_read_rffc5071(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	uint16_t value;
	if( stage == USB_TRANSFER_STAGE_SETUP ) 
	{
		if( endpoint->setup.index < RFFC5071_NUM_REGS ) 
		{
			value = rffc5071_reg_read(endpoint->setup.index);
			endpoint->buffer[0] = value & 0xff;
			endpoint->buffer[1] = value >> 8;
			usb_transfer_schedule_block(endpoint->in, &endpoint->buffer, 2,
						    NULL, NULL);
			usb_transfer_schedule_ack(endpoint->out);
			return USB_REQUEST_STATUS_OK;
		}
		return USB_REQUEST_STATUS_STALL;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}
