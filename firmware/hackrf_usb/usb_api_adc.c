/*
 * Copyright 2025 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include <stddef.h>
#include <usb_queue.h>
#include "adc.h"
#include "usb_api_adc.h"

usb_request_status_t usb_vendor_request_adc_read(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		if ((endpoint->setup.index & ~0x80) > 7) {
			return USB_REQUEST_STATUS_STALL;
		}
		uint16_t value = adc_read(endpoint->setup.index);
		adc_off();
		endpoint->buffer[0] = value & 0xff;
		endpoint->buffer[1] = value >> 8;
		usb_transfer_schedule_block(
			endpoint->in,
			&endpoint->buffer,
			2,
			NULL,
			NULL);
		usb_transfer_schedule_ack(endpoint->out);
		return USB_REQUEST_STATUS_OK;
	}
	return USB_REQUEST_STATUS_OK;
}
