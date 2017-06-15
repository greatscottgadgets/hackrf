/*
 * Copyright 2017 Dominic Spill <dominicgs@gmail.com>
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

#include "usb_api_mode.h"
#include "usb_api_transceiver.h"
#include "usb.h"
#include "usb_queue.h"
#include "usb_endpoint.h"

typedef enum {
	USB_MODE_NONE = 0,
	USB_MODE_IN = 1,
	USB_MODE_OUT = 2,
	USB_MODE_BOTH = 3
} usb_mode_t;

void set_usb_mode(const usb_mode_t new_usb_mode) {
	usb_endpoint_disable(&usb_endpoint_bulk_in);
	usb_endpoint_disable(&usb_endpoint_bulk_out);
    switch(new_usb_mode) {
        case USB_MODE_IN:
	        usb_endpoint_init(&usb_endpoint_bulk_in);
            break;
        case USB_MODE_OUT:
	        usb_endpoint_init(&usb_endpoint_bulk_out);
            break;
        case USB_MODE_BOTH:
	        usb_endpoint_init(&usb_endpoint_bulk_in);
	        usb_endpoint_init(&usb_endpoint_bulk_out);
            break;
        default: /* USB_MODE_NONE */
            break;
    }
}

volatile hackrf_mode_t hackrf_mode;

int set_hackrf_mode(hackrf_mode_t _hackrf_mode) {
	hackrf_mode = _hackrf_mode;
	switch(hackrf_mode) {
		case HACKRF_MODE_RX:
		case HACKRF_MODE_SWEEP:
			set_transceiver_mode(TRANSCEIVER_MODE_RX);
			set_usb_mode(USB_MODE_IN);
			break;
		case HACKRF_MODE_TX:
			set_transceiver_mode(TRANSCEIVER_MODE_TX);
			set_usb_mode(USB_MODE_OUT);
			break;
		case HACKRF_MODE_CPLD:
			set_transceiver_mode(TRANSCEIVER_MODE_OFF);
			set_usb_mode(USB_MODE_OUT);
			break;
		case HACKRF_MODE_IDLE:
			set_transceiver_mode(TRANSCEIVER_MODE_OFF);
			set_usb_mode(USB_MODE_NONE);
			break;
		default:
			hackrf_mode = HACKRF_MODE_IDLE;
			set_transceiver_mode(TRANSCEIVER_MODE_OFF);
			set_usb_mode(USB_MODE_NONE);
			return 1;
	}
	return 0;
}

 usb_request_status_t usb_vendor_request_set_mode(
        usb_endpoint_t* const endpoint,const usb_transfer_stage_t stage) {
	int result;
	if(stage == USB_TRANSFER_STAGE_SETUP) {
		result = set_hackrf_mode(endpoint->setup.value);
		if(result == 0) {
			usb_transfer_schedule_ack(endpoint->in);
			return USB_REQUEST_STATUS_OK;
		}
	}
	return USB_REQUEST_STATUS_STALL;
}
