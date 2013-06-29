/*
 * Copyright 2012 Jared Boone
 * Copyright 2013 Ben Gamari
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

#include <stdint.h>
#include <stdbool.h>

#include "usb.h"

usb_transfer_descriptor_t usb_td[12] ATTR_ALIGNED(64);

#define USB_TD_INDEX(endpoint_address) (((endpoint_address & 0xF) * 2) + ((endpoint_address >> 7) & 1))

usb_transfer_descriptor_t* usb_transfer_descriptor(
	const uint_fast8_t endpoint_address
) {
	return &usb_td[USB_TD_INDEX(endpoint_address)];
}

void usb_endpoint_schedule(
	const usb_endpoint_t* const endpoint,
	void* const data,
	const uint32_t maximum_length
) {
	usb_transfer_descriptor_t* const td = usb_transfer_descriptor(endpoint->address);
	
	// Ensure that endpoint is ready to be primed.
	// It may have been flushed due to an aborted transaction.
	// TODO: This should be preceded by a flush?
	while( usb_endpoint_is_ready(endpoint) );

	// Configure a transfer.
	td->next_dtd_pointer = USB_TD_NEXT_DTD_POINTER_TERMINATE;
	td->total_bytes =
		  USB_TD_DTD_TOKEN_TOTAL_BYTES(maximum_length)
		| USB_TD_DTD_TOKEN_IOC
		| USB_TD_DTD_TOKEN_MULTO(0)
		| USB_TD_DTD_TOKEN_STATUS_ACTIVE
		;
	td->buffer_pointer_page[0] =  (uint32_t)data;
	td->buffer_pointer_page[1] = ((uint32_t)data + 0x1000) & 0xfffff000;
	td->buffer_pointer_page[2] = ((uint32_t)data + 0x2000) & 0xfffff000;
	td->buffer_pointer_page[3] = ((uint32_t)data + 0x3000) & 0xfffff000;
	td->buffer_pointer_page[4] = ((uint32_t)data + 0x4000) & 0xfffff000;
	
	usb_endpoint_prime(endpoint, td);
}

void usb_endpoint_schedule_ack(
	const usb_endpoint_t* const endpoint
) {
	usb_endpoint_schedule(endpoint, 0, 0);
}
