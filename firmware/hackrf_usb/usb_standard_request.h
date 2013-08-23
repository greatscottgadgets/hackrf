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

#ifndef __USB_STANDARD_REQUEST_H__
#define __USB_STANDARD_REQUEST_H__

#include "usb_type.h"
#include "usb_request.h"

void usb_set_configuration_changed_cb(
        void (*callback)(usb_device_t* const)
);

usb_request_status_t usb_standard_request(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
);

const uint8_t* usb_endpoint_descriptor(
	const usb_endpoint_t* const endpoint
);

uint_fast16_t usb_endpoint_descriptor_max_packet_size(
	const uint8_t* const endpoint_descriptor
);

usb_transfer_type_t usb_endpoint_descriptor_transfer_type(
	const uint8_t* const endpoint_descriptor
);

bool usb_set_configuration(
	usb_device_t* const device,
	const uint_fast8_t configuration_number
);

#endif//__USB_STANDARD_REQUEST_H__
