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

#include <stdint.h>

#include "usb_standard_request.h"

#include "usb.h"
#include "usb_type.h"
#include "usb_descriptor.h"

const uint8_t* usb_endpoint_descriptor(
	const usb_endpoint_t* const endpoint
) {
	const usb_configuration_t* const configuration = endpoint->device->configuration;
	if( configuration ) {
		const uint8_t* descriptor = configuration->descriptor;
		while( descriptor[0] != 0 ) {
			if( descriptor[1] == USB_DESCRIPTOR_TYPE_ENDPOINT ) {
				if( descriptor[2] == endpoint->address ) {
					return descriptor;
				}
			}
			descriptor += descriptor[0];
		}
	}
	
	return 0;
}
	
uint_fast16_t usb_endpoint_descriptor_max_packet_size(
	const uint8_t* const endpoint_descriptor
) {
	return (endpoint_descriptor[5] << 8) | endpoint_descriptor[4];
}

usb_transfer_type_t usb_endpoint_descriptor_transfer_type(
	const uint8_t* const endpoint_descriptor
) {
	return (endpoint_descriptor[3] & 0x3);
}

extern bool usb_set_configuration(
	usb_device_t* const device,
	const uint_fast8_t configuration_number
);
	
static void usb_send_descriptor(
	usb_endpoint_t* const endpoint,
	uint8_t* const descriptor_data
) {
	const uint32_t setup_length = endpoint->setup.length;
	uint32_t descriptor_length = descriptor_data[0];
	if( descriptor_data[1] == USB_DESCRIPTOR_TYPE_CONFIGURATION ) {
		descriptor_length = (descriptor_data[3] << 8) | descriptor_data[2];
	}
	usb_endpoint_schedule(
		endpoint->in,
		descriptor_data,
	 	(setup_length > descriptor_length) ? descriptor_length : setup_length
	);
}

static void usb_send_descriptor_string(
	usb_endpoint_t* const endpoint
) {
	uint_fast8_t index = endpoint->setup.value_l;
	for( uint_fast8_t i=0; usb_descriptor_strings[i] != 0; i++ ) {
		if( i == index ) {
			usb_send_descriptor(endpoint, usb_descriptor_strings[i]);
			return;
		}
	}

	usb_endpoint_stall(endpoint);
}

static void usb_standard_request_get_descriptor_setup(
	usb_endpoint_t* const endpoint
) {
	switch( endpoint->setup.value_h ) {
	case USB_DESCRIPTOR_TYPE_DEVICE:
		usb_send_descriptor(endpoint, usb_descriptor_device);
		break;
		
	case USB_DESCRIPTOR_TYPE_CONFIGURATION:
		// TODO: Duplicated code. Refactor.
		if( usb_speed(endpoint->device) == USB_SPEED_HIGH ) {
			usb_send_descriptor(endpoint, usb_descriptor_configuration_high_speed);
		} else {
			usb_send_descriptor(endpoint, usb_descriptor_configuration_full_speed);
		}
		break;
	
	case USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER:
		usb_send_descriptor(endpoint, usb_descriptor_device_qualifier);
		break;

	case USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIGURATION:
		// TODO: Duplicated code. Refactor.
		if( usb_speed(endpoint->device) == USB_SPEED_HIGH ) {
			usb_send_descriptor(endpoint, usb_descriptor_configuration_full_speed);
		} else {
			usb_send_descriptor(endpoint, usb_descriptor_configuration_high_speed);
		}
		break;
	
	case USB_DESCRIPTOR_TYPE_STRING:
		usb_send_descriptor_string(endpoint);
		break;
		
	case USB_DESCRIPTOR_TYPE_INTERFACE:
	case USB_DESCRIPTOR_TYPE_ENDPOINT:
	default:
		usb_endpoint_stall(endpoint);
		break;
	}
}

static void usb_standard_request_get_descriptor(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	switch( stage ) {
	case USB_TRANSFER_STAGE_SETUP:
		usb_standard_request_get_descriptor_setup(endpoint);
		usb_endpoint_schedule_ack(endpoint->out);
		break;
		
	case USB_TRANSFER_STAGE_DATA:
		break;
		
	case USB_TRANSFER_STAGE_STATUS:
		break;

	}
}

/*********************************************************************/

static void usb_standard_request_set_address(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	switch( stage ) {
	case USB_TRANSFER_STAGE_SETUP:
		usb_set_address_deferred(endpoint->device, endpoint->setup.value_l);
		usb_endpoint_schedule_ack(endpoint->in);
		break;
		
	case USB_TRANSFER_STAGE_DATA:
		break;
		
	case USB_TRANSFER_STAGE_STATUS:
		/* NOTE: Not necessary to set address here, as DEVICEADR.USBADRA bit
		 * will cause controller to automatically perform set address
		 * operation on IN ACK.
		 */
		break;
		
	default:
		break;
	}
}

/*********************************************************************/

static void usb_standard_request_set_configuration_setup(
	usb_endpoint_t* const endpoint
) {
	const uint8_t usb_configuration = endpoint->setup.value_l;
	if( usb_set_configuration(endpoint->device, usb_configuration) ) {
		if( usb_configuration == 0 ) {
			// TODO: Should this be done immediately?
			usb_set_address_immediate(endpoint->device, 0);
		}
		usb_endpoint_schedule_ack(endpoint->in);
	} else {
		usb_endpoint_stall(endpoint);
	}
}

static void usb_standard_request_set_configuration(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	switch( stage ) {
	case USB_TRANSFER_STAGE_SETUP:
		usb_standard_request_set_configuration_setup(endpoint);
		break;
		
	case USB_TRANSFER_STAGE_DATA:
		break;
		
	case USB_TRANSFER_STAGE_STATUS:
		break;
		
	}
}

/*********************************************************************/

static void usb_standard_request_get_configuration_setup(
	usb_endpoint_t* const endpoint
) {
	if( endpoint->setup.length == 1 ) {
		endpoint->buffer[0] = 0;
		if( endpoint->device->configuration ) {
			endpoint->buffer[0] = endpoint->device->configuration->number;
		}
		usb_endpoint_schedule(endpoint->in, &endpoint->buffer, 1);
	} else {
		usb_endpoint_stall(endpoint);
	}
}

static void usb_standard_request_get_configuration(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	switch( stage ) {
	case USB_TRANSFER_STAGE_SETUP:
		usb_standard_request_get_configuration_setup(endpoint);
		usb_endpoint_schedule_ack(endpoint->out);
		break;
		
	case USB_TRANSFER_STAGE_DATA:
		break;
		
	case USB_TRANSFER_STAGE_STATUS:
		break;
	
	}
}

/*********************************************************************/

void usb_standard_request(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	switch( endpoint->setup.request ) {
	case USB_STANDARD_REQUEST_GET_DESCRIPTOR:
		usb_standard_request_get_descriptor(endpoint, stage);
		break;
	
	case USB_STANDARD_REQUEST_SET_ADDRESS:
		usb_standard_request_set_address(endpoint, stage);
		break;
		
	case USB_STANDARD_REQUEST_SET_CONFIGURATION:
		usb_standard_request_set_configuration(endpoint, stage);
		break;
		
	case USB_STANDARD_REQUEST_GET_CONFIGURATION:
		usb_standard_request_get_configuration(endpoint, stage);
		break;
		
	}
}
