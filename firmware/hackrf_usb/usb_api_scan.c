/*
 * Copyright 2016 Mike Walters, Dominic Spill
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

#include "usb_api_scan.h"
#include "usb_queue.h"
#include <stddef.h>
#include <hackrf_core.h>
#include "usb_api_transceiver.h"
#include "usb_bulk_buffer.h"
#include "tuning.h"
#include "usb_endpoint.h"

#define MIN(x,y)       ((x)<(y)?(x):(y))
#define MAX(x,y)       ((x)>(y)?(x):(y))
#define MIN_FREQ 1000000
#define MAX_FREQ 6000000000

volatile bool start_scan_mode = false;
static uint64_t scan_freq;
static uint64_t scan_freq_min;
static uint64_t scan_freq_max;
static uint64_t scan_freq_step;

struct init_scan_params {
	uint64_t min_freq_hz;
	uint64_t max_freq_hz;
	uint64_t step_freq_hz;
};
struct init_scan_params scan_params;

usb_request_status_t usb_vendor_request_init_scan(
		usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage)
{
	if ((stage == USB_TRANSFER_STAGE_SETUP) &&
		(endpoint->setup.length == 24)) {
		// DGS set scan frequencies here
		//freq_min  = bytes_to_uint64();
		usb_transfer_schedule_block(endpoint->out, &scan_params, sizeof(struct init_scan_params),
									NULL, NULL);
		
		scan_freq_min = MAX(MIN_FREQ, scan_params.min_freq_hz);
		scan_freq_max = MIN(MAX_FREQ, scan_params.max_freq_hz);
		scan_freq_step = scan_params.step_freq_hz;
		
		scan_freq = scan_freq_min;
		set_freq(scan_freq);
		start_scan_mode = true;
		usb_transfer_schedule_ack(endpoint->in);
	}
	return USB_REQUEST_STATUS_OK;
}

void scan_mode(void) {
	unsigned int blocks_queued = 0;
	unsigned int phase = 0;

	while(transceiver_mode() != TRANSCEIVER_MODE_OFF) {
		// Set up IN transfer of buffer 0.
		if ( usb_bulk_buffer_offset >= 16384
		     && phase == 1
		     && transceiver_mode() != TRANSCEIVER_MODE_OFF) {
			if (blocks_queued == 2)
				usb_transfer_schedule_block(
					(transceiver_mode() == TRANSCEIVER_MODE_RX)
					? &usb_endpoint_bulk_in : &usb_endpoint_bulk_out,
					&usb_bulk_buffer[0x0000],
					0x4000,
					NULL, NULL
					);
			phase = 0;
			blocks_queued++;
		}

		// Set up IN transfer of buffer 1.
		if ( usb_bulk_buffer_offset < 16384
		     && phase == 0
		     && transceiver_mode() != TRANSCEIVER_MODE_OFF) {
			if (blocks_queued == 2)
				usb_transfer_schedule_block(
					(transceiver_mode() == TRANSCEIVER_MODE_RX)
					? &usb_endpoint_bulk_in : &usb_endpoint_bulk_out,
					&usb_bulk_buffer[0x4000],
					0x4000,
					NULL, NULL
				);
			phase = 1;
			blocks_queued++;
		}

		if (blocks_queued > 2) {
			scan_freq += scan_freq_step;
			if (scan_freq > scan_freq_max) {
				scan_freq = scan_freq_min;
				}
				set_freq(scan_freq);
			blocks_queued = 0;
		}
	}
}
