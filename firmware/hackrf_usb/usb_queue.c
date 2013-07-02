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
        usb_endpoint_schedule_wait(endpoint,
                                   usb_transfer_descriptor(endpoint->address),
                                   data,
                                   maximum_length);
}
	
void usb_endpoint_schedule_ack(
	const usb_endpoint_t* const endpoint
) {
	usb_endpoint_schedule(endpoint, 0, 0);
}
