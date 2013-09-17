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
#include <stddef.h>

#include "usb_standard_request.h"

#include "usb.h"
#include "usb_type.h"
#include "usb_queue.h"

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

void (*usb_configuration_changed_cb)(usb_device_t* const) = NULL;

void usb_set_configuration_changed_cb(
	void (*callback)(usb_device_t* const)
) {
	usb_configuration_changed_cb = callback;
}

bool usb_set_configuration(
	usb_device_t* const device,
	const uint_fast8_t configuration_number
) {

	const usb_configuration_t* new_configuration = 0;
	if( configuration_number != 0 ) {
		
		// Locate requested configuration.
		if( device->configurations ) {
			usb_configuration_t** configurations = *(device->configurations);
			uint32_t i = 0;
			const usb_speed_t usb_speed_current = usb_speed(device);
			while( configurations[i] ) {
				if( (configurations[i]->speed == usb_speed_current) &&
				    (configurations[i]->number == configuration_number) ) {
					new_configuration = configurations[i];
					break;
				}
				i++;
			}
		}

		// Requested configuration not found: request error.
		if( new_configuration == 0 ) {
			return false;
		}
	}
	
	if( new_configuration != device->configuration ) {
		// Configuration changed.
		device->configuration = new_configuration;
	}

	if (usb_configuration_changed_cb)
		usb_configuration_changed_cb(device);

	return true;
}
	
static usb_request_status_t usb_send_descriptor(
	usb_endpoint_t* const endpoint,
	const uint8_t* const descriptor_data
) {
	const uint32_t setup_length = endpoint->setup.length;
	uint32_t descriptor_length = descriptor_data[0];
	if( descriptor_data[1] == USB_DESCRIPTOR_TYPE_CONFIGURATION ) {
		descriptor_length = (descriptor_data[3] << 8) | descriptor_data[2];
	}
	// We cast the const away but this shouldn't be a problem as this is a write transfer
	usb_transfer_schedule_block(
		endpoint->in,
		(uint8_t* const) descriptor_data,
	 	(setup_length > descriptor_length) ? descriptor_length : setup_length,
		NULL, NULL
	);
	usb_transfer_schedule_ack(endpoint->out);
	return USB_REQUEST_STATUS_OK;
}

static usb_request_status_t usb_send_descriptor_string(
	usb_endpoint_t* const endpoint
) {
	uint_fast8_t index = endpoint->setup.value_l;
	for( uint_fast8_t i=0; endpoint->device->descriptor_strings[i] != 0; i++ ) {
		if( i == index ) {
			return usb_send_descriptor(endpoint, endpoint->device->descriptor_strings[i]);
		}
	}

	return USB_REQUEST_STATUS_STALL;
}

static usb_request_status_t usb_send_descriptor_config(
	usb_endpoint_t* const endpoint,
	usb_speed_t speed,
	const uint8_t config_num
) {
	usb_configuration_t** config = *(endpoint->device->configurations);
	unsigned int i = 0;
	for( ; *config != NULL; config++ ) {
		if( (*config)->speed == speed) {
			if (i == config_num) {
				return usb_send_descriptor(endpoint, (*config)->descriptor);
			} else {
				i++;
			}
		}
	}
	return USB_REQUEST_STATUS_STALL;
}

static usb_request_status_t usb_standard_request_get_descriptor_setup(
	usb_endpoint_t* const endpoint
) {
	switch( endpoint->setup.value_h ) {
	case USB_DESCRIPTOR_TYPE_DEVICE:
		return usb_send_descriptor(endpoint, endpoint->device->descriptor);
		
	case USB_DESCRIPTOR_TYPE_CONFIGURATION:
		// TODO: Duplicated code. Refactor.
		if( usb_speed(endpoint->device) == USB_SPEED_HIGH ) {
			return usb_send_descriptor_config(endpoint, USB_SPEED_HIGH, endpoint->setup.value_l);
		} else {
			return usb_send_descriptor_config(endpoint, USB_SPEED_FULL, endpoint->setup.value_l);
		}
	
	case USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER:
		return usb_send_descriptor(endpoint, endpoint->device->qualifier_descriptor);

	case USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIGURATION:
		// TODO: Duplicated code. Refactor.
		if( usb_speed(endpoint->device) == USB_SPEED_HIGH ) {
			return usb_send_descriptor_config(endpoint, USB_SPEED_FULL, endpoint->setup.value_l);
		} else {
			return usb_send_descriptor_config(endpoint, USB_SPEED_HIGH, endpoint->setup.value_l);
		}
	
	case USB_DESCRIPTOR_TYPE_STRING:
		return usb_send_descriptor_string(endpoint);
		
	case USB_DESCRIPTOR_TYPE_INTERFACE:
	case USB_DESCRIPTOR_TYPE_ENDPOINT:
	default:
		return USB_REQUEST_STATUS_STALL;
	}
}

static usb_request_status_t usb_standard_request_get_descriptor(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	switch( stage ) {
	case USB_TRANSFER_STAGE_SETUP:
		return usb_standard_request_get_descriptor_setup(endpoint);
		
	case USB_TRANSFER_STAGE_DATA:
	case USB_TRANSFER_STAGE_STATUS:
		return USB_REQUEST_STATUS_OK;

	default:
		return USB_REQUEST_STATUS_STALL;
	}
}

/*********************************************************************/

static usb_request_status_t usb_standard_request_set_address_setup(
	usb_endpoint_t* const endpoint
) {
	usb_set_address_deferred(endpoint->device, endpoint->setup.value_l);
	usb_transfer_schedule_ack(endpoint->in);
	return USB_REQUEST_STATUS_OK;
}

static usb_request_status_t usb_standard_request_set_address(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	switch( stage ) {
	case USB_TRANSFER_STAGE_SETUP:
		return usb_standard_request_set_address_setup(endpoint);
		
	case USB_TRANSFER_STAGE_DATA:
	case USB_TRANSFER_STAGE_STATUS:
		/* NOTE: Not necessary to set address here, as DEVICEADR.USBADRA bit
		 * will cause controller to automatically perform set address
		 * operation on IN ACK.
		 */
		return USB_REQUEST_STATUS_OK;
		
	default:
		return USB_REQUEST_STATUS_STALL;
	}
}

/*********************************************************************/

static usb_request_status_t usb_standard_request_set_configuration_setup(
	usb_endpoint_t* const endpoint
) {
	const uint8_t usb_configuration = endpoint->setup.value_l;
	if( usb_set_configuration(endpoint->device, usb_configuration) ) {
		if( usb_configuration == 0 ) {
			// TODO: Should this be done immediately?
			usb_set_address_immediate(endpoint->device, 0);
		}
		usb_transfer_schedule_ack(endpoint->in);
		return USB_REQUEST_STATUS_OK;
	} else {
		return USB_REQUEST_STATUS_STALL;
	}
}

static usb_request_status_t usb_standard_request_set_configuration(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	switch( stage ) {
	case USB_TRANSFER_STAGE_SETUP:
		return usb_standard_request_set_configuration_setup(endpoint);
		
	case USB_TRANSFER_STAGE_DATA:
	case USB_TRANSFER_STAGE_STATUS:
		return USB_REQUEST_STATUS_OK;
		
	default:
		return USB_REQUEST_STATUS_STALL;
	}
}

/*********************************************************************/

static usb_request_status_t usb_standard_request_get_configuration_setup(
	usb_endpoint_t* const endpoint
) {
	if( endpoint->setup.length == 1 ) {
		endpoint->buffer[0] = 0;
		if( endpoint->device->configuration ) {
			endpoint->buffer[0] = endpoint->device->configuration->number;
		}
		usb_transfer_schedule_block(endpoint->in, &endpoint->buffer, 1, NULL, NULL);
		usb_transfer_schedule_ack(endpoint->out);
		return USB_REQUEST_STATUS_OK;
	} else {
		return USB_REQUEST_STATUS_STALL;
	}
}

static usb_request_status_t usb_standard_request_get_configuration(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	switch( stage ) {
	case USB_TRANSFER_STAGE_SETUP:
		return usb_standard_request_get_configuration_setup(endpoint);
		
	case USB_TRANSFER_STAGE_DATA:
	case USB_TRANSFER_STAGE_STATUS:
		return USB_REQUEST_STATUS_OK;

	default:
		return USB_REQUEST_STATUS_STALL;
	}
}

/*********************************************************************/

usb_request_status_t usb_standard_request(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	switch( endpoint->setup.request ) {
	case USB_STANDARD_REQUEST_GET_DESCRIPTOR:
		return usb_standard_request_get_descriptor(endpoint, stage);
	
	case USB_STANDARD_REQUEST_SET_ADDRESS:
		return usb_standard_request_set_address(endpoint, stage);
		
	case USB_STANDARD_REQUEST_SET_CONFIGURATION:
		return usb_standard_request_set_configuration(endpoint, stage);
		
	case USB_STANDARD_REQUEST_GET_CONFIGURATION:
		return usb_standard_request_get_configuration(endpoint, stage);

	default:
		return USB_REQUEST_STATUS_STALL;
	}
}
