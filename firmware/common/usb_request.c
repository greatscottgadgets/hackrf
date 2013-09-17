/*
 * Copyright 2012 Jared Boone
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

#include "usb.h"
#include "usb_request.h"
#include "usb_queue.h"

#include <stdbool.h>

static void usb_request(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	usb_request_status_t status = USB_REQUEST_STATUS_STALL;
	usb_request_handler_fn handler = 0;
	
	switch( endpoint->setup.request_type & USB_SETUP_REQUEST_TYPE_mask ) {
	case USB_SETUP_REQUEST_TYPE_STANDARD:
		handler = usb_request_handlers.standard;
		break;
	
	case USB_SETUP_REQUEST_TYPE_CLASS:
		handler = usb_request_handlers.class;
		break;
	
	case USB_SETUP_REQUEST_TYPE_VENDOR:
		handler = usb_request_handlers.vendor;
		break;
		
	case USB_SETUP_REQUEST_TYPE_RESERVED:
		handler = usb_request_handlers.reserved;
		break;
	}
	
	if( handler ) {
		status = handler(endpoint, stage);
	}

	if( status != USB_REQUEST_STATUS_OK ) {
		// USB 2.0 section 9.2.7 "Request Error"
		usb_endpoint_stall(endpoint);
	}
}

void usb_setup_complete(
	usb_endpoint_t* const endpoint
) {
	usb_request(endpoint, USB_TRANSFER_STAGE_SETUP);
}

void usb_control_out_complete(
	usb_endpoint_t* const endpoint
) {
	const bool device_to_host =
		endpoint->setup.request_type >> USB_SETUP_REQUEST_TYPE_DATA_TRANSFER_DIRECTION_shift;
	if( device_to_host ) {
		usb_request(endpoint, USB_TRANSFER_STAGE_STATUS);
	} else {
		usb_request(endpoint, USB_TRANSFER_STAGE_DATA);
	}
        usb_queue_transfer_complete(endpoint);
}

void usb_control_in_complete(
	usb_endpoint_t* const endpoint
) {
	const bool device_to_host =
		endpoint->setup.request_type >> USB_SETUP_REQUEST_TYPE_DATA_TRANSFER_DIRECTION_shift;
	if( device_to_host ) {
		usb_request(endpoint, USB_TRANSFER_STAGE_DATA);
	} else {
		usb_request(endpoint, USB_TRANSFER_STAGE_STATUS);
	}
        usb_queue_transfer_complete(endpoint);
}

