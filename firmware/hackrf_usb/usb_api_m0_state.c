/*
 * Copyright 2022 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include "usb_api_m0_state.h"

#include <libopencm3/lpc43xx/sgpio.h>
#include <stddef.h>
#include <usb_request.h>
#include <usb_queue.h>

void m0_set_mode(enum m0_mode mode)
{
	// Set requested mode and flag bit.
	m0_state.requested_mode = M0_REQUEST_FLAG | mode;

	// The M0 may be blocked waiting for the next SGPIO interrupt.
	// In order to ensure that it sees our request, we need to set
	// the interrupt flag here. The M0 will clear the flag again
	// before acknowledging our request.
	SGPIO_SET_STATUS_1 = (1 << SGPIO_SLICE_A);

	// Wait for M0 to acknowledge by clearing the flag.
	while (m0_state.requested_mode & M0_REQUEST_FLAG) {}
}

usb_request_status_t usb_vendor_request_get_m0_state(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		usb_transfer_schedule_block(
			endpoint->in,
			(void*) &m0_state,
			sizeof(m0_state),
			NULL,
			NULL);
		usb_transfer_schedule_ack(endpoint->out);
		return USB_REQUEST_STATUS_OK;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}
