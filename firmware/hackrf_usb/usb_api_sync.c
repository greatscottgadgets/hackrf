/*
 * Copyright 2012-2026 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include "usb_api_sync.h"

#include <platform_detect.h>
#include <radio.h>
#include <usb.h>
#include <usb_queue.h>
#include <usb_request.h>
#include <usb_type.h>
#include "usb_api_transceiver.h"

usb_request_status_t usb_vendor_request_sync_start(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		switch (detected_platform()) {
		case BOARD_ID_HACKRF1_OG:
		case BOARD_ID_HACKRF1_R9:
		case BOARD_ID_PRALINE:
			// supported
			break;
		default:
			return USB_REQUEST_STATUS_STALL;
		}

		const uint8_t mode = endpoint->setup.value;

		switch (mode) {
		case TRANSCEIVER_MODE_OFF:
		case TRANSCEIVER_MODE_RX:
		case TRANSCEIVER_MODE_TX:
			break;
		default:
			return USB_REQUEST_STATUS_STALL;
		}

		radio_reg_write(&radio, RADIO_BANK_ACTIVE, RADIO_TRIGGER, 1);
		request_transceiver_mode(mode);

		usb_transfer_schedule_ack(endpoint->in);
	}
	return USB_REQUEST_STATUS_OK;
}
