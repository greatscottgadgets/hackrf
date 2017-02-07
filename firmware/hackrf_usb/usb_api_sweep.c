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
#define MAX_RANGES 10
#define THROWAWAY_BUFFERS 2

volatile bool start_sweep_mode = false;
static uint64_t sweep_freq;
static bool odd = true;
static uint16_t frequencies[MAX_RANGES * 2];
static unsigned char data[9 + MAX_RANGES * 2 * sizeof(frequencies[0])];
static uint16_t num_ranges = 0;
static uint32_t dwell_blocks = 0;
static uint32_t step_width = 0;
static uint32_t offset = 0;
static enum sweep_style style = LINEAR;
static uint16_t range = 0;

usb_request_status_t usb_vendor_request_init_sweep(
		usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage)
{
	uint32_t num_samples;
	int i;
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		num_samples = (endpoint->setup.index << 16) | endpoint->setup.value;
		dwell_blocks = num_samples / 0x4000;
		if(1 > dwell_blocks) {
			return USB_REQUEST_STATUS_STALL;
		}
		num_ranges = (endpoint->setup.length - 9) / (2 * sizeof(frequencies[0]));
		if((1 > num_ranges) || (MAX_RANGES < num_ranges)) {
			return USB_REQUEST_STATUS_STALL;
		}
		usb_transfer_schedule_block(endpoint->out, &data,
				endpoint->setup.length, NULL, NULL);
	} else if (stage == USB_TRANSFER_STAGE_DATA) {
		step_width = ((uint32_t)(data[3]) << 24) | ((uint32_t)(data[2]) << 16)
				| ((uint32_t)(data[1]) << 8) | data[0];
		if(1 > step_width) {
			return USB_REQUEST_STATUS_STALL;
		}
		offset = ((uint32_t)(data[7]) << 24) | ((uint32_t)(data[6]) << 16)
				| ((uint32_t)(data[5]) << 8) | data[4];
		style = data[8];
		if(INTERLEAVED < style) {
			return USB_REQUEST_STATUS_STALL;
		}
		for(i=0; i<(num_ranges*2); i++) {
			frequencies[i] = ((uint16_t)(data[10+i*2]) << 8) + data[9+i*2];
		}
		sweep_freq = frequencies[0] * FREQ_GRANULARITY;
		set_freq(sweep_freq + offset);
		start_sweep_mode = true;
		usb_transfer_schedule_ack(endpoint->in);
	}
	return USB_REQUEST_STATUS_OK;
}

void sweep_mode(void) {
	unsigned int blocks_queued = 0;
	unsigned int phase = 0;

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
			*buffer = 0x7f;
			*(buffer+1) = 0x7f;
			*(buffer+2) = sweep_freq & 0xff;
			*(buffer+3) = (sweep_freq >> 8) & 0xff;
			*(buffer+4) = (sweep_freq >> 16) & 0xff;
			*(buffer+5) = (sweep_freq >> 24) & 0xff;
			*(buffer+6) = (sweep_freq >> 32) & 0xff;
			*(buffer+7) = (sweep_freq >> 40) & 0xff;
			*(buffer+8) = (sweep_freq >> 48) & 0xff;
			*(buffer+9) = (sweep_freq >> 56) & 0xff;
			if (blocks_queued > THROWAWAY_BUFFERS) {
				usb_transfer_schedule_block(
					&usb_endpoint_bulk_in,
					buffer,
					0x4000,
					NULL, NULL
				);
			}
			transfer = false;
		}

		if ((dwell_blocks + THROWAWAY_BUFFERS) <= blocks_queued) {
			if(INTERLEAVED == style) {
				if(!odd && ((sweep_freq + step_width) >= ((uint64_t)frequencies[1+range*2] * FREQ_GRANULARITY))) {
					range = (range + 1) % num_ranges;
					sweep_freq = frequencies[range*2] * FREQ_GRANULARITY;
				} else {
					if(odd) {
						sweep_freq += step_width/4;
					} else {
						sweep_freq += 3*step_width/4;
					}
				}
				odd = !odd;
			} else {
				if((sweep_freq + step_width) >= ((uint64_t)frequencies[1+range*2] * FREQ_GRANULARITY)) {
					range = (range + 1) % num_ranges;
					sweep_freq = frequencies[range*2] * FREQ_GRANULARITY;
				} else {
					sweep_freq += step_width;
				}
			}
			set_freq(sweep_freq + offset);
			blocks_queued = 0;
		}
	}
}
