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

#include "usb_api_sweep.h"
#include "usb_queue.h"
#include <stddef.h>
#include <hackrf_core.h>
#include "usb_api_transceiver.h"
#include "usb_bulk_buffer.h"
#include "tuning.h"
#include "usb_endpoint.h"

#define MIN(x,y)       ((x)<(y)?(x):(y))
#define MAX(x,y)       ((x)>(y)?(x):(y))
#define FREQ_GRANULARITY 1000000
#define MIN_FREQ 1
#define MAX_FREQ 6000
#define MAX_FREQ_COUNT 500

volatile bool start_sweep_mode = false;
static uint64_t sweep_freq;
bool odd = true;
static uint16_t frequencies[MAX_FREQ_COUNT];
static uint16_t frequency_count = 0;

usb_request_status_t usb_vendor_request_init_sweep(
		usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		frequency_count = endpoint->setup.length;
		usb_transfer_schedule_block(endpoint->out, &frequencies,
									endpoint->setup.length, NULL, NULL);
	} else if (stage == USB_TRANSFER_STAGE_DATA) {
		sweep_freq = frequencies[0];
		set_freq(sweep_freq*FREQ_GRANULARITY);
		start_sweep_mode = true;
		usb_transfer_schedule_ack(endpoint->in);
	}
	return USB_REQUEST_STATUS_OK;
}

void sweep_mode(void) {
	unsigned int blocks_queued = 0;
	unsigned int phase = 0;
	unsigned int ifreq = 0;

	uint8_t *buffer;
	bool transfer = false;

	while(transceiver_mode() != TRANSCEIVER_MODE_OFF) {
		// Set up IN transfer of buffer 0.
		if ( usb_bulk_buffer_offset >= 16384 && phase == 1) {
			transfer = true;
			buffer = &usb_bulk_buffer[0x0000];
			phase = 0;
			blocks_queued++;
		}

		// Set up IN transfer of buffer 1.
		if ( usb_bulk_buffer_offset < 16384 && phase == 0) {
			transfer = true;
			buffer = &usb_bulk_buffer[0x4000];
			phase = 1;
			blocks_queued++;
		}

		if (transfer) {
			*(uint16_t*)buffer = 0x7F7F;
			*(uint16_t*)(buffer+2) = sweep_freq;
			if (blocks_queued > 1)
				usb_transfer_schedule_block(
					&usb_endpoint_bulk_in,
					buffer,
					0x4000,
					NULL, NULL
				);
			transfer = false;
		}

		if (blocks_queued > 2) {
			if(++ifreq >= frequency_count)
				ifreq = 0;
			sweep_freq = frequencies[ifreq];
			set_freq(sweep_freq*FREQ_GRANULARITY);
			blocks_queued = 0;
		}
	}
}
