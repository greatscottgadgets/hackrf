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
#include "usb_api_m0_state.h"
#include "tuning.h"
#include "usb_endpoint.h"
#include "streaming.h"

#include <libopencm3/lpc43xx/m4/nvic.h>

#define MIN(x, y)         ((x) < (y) ? (x) : (y))
#define MAX(x, y)         ((x) > (y) ? (x) : (y))
#define FREQ_GRANULARITY  1000000
#define MAX_RANGES        10
#define THROWAWAY_BUFFERS 2

static uint64_t sweep_freq;
static uint16_t frequencies[MAX_RANGES * 2];
static unsigned char data[9 + MAX_RANGES * 2 * sizeof(frequencies[0])];
static uint16_t num_ranges = 0;
static uint32_t dwell_blocks = 0;
static uint32_t step_width = 0;
static uint32_t offset = 0;
static enum sweep_style style = LINEAR;

/* Do this before starting sweep mode with request_transceiver_mode(). */
usb_request_status_t usb_vendor_request_init_sweep(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	uint32_t num_bytes;
	int i;
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		num_bytes = (endpoint->setup.index << 16) | endpoint->setup.value;
		dwell_blocks = num_bytes / 0x4000;
		if (1 > dwell_blocks) {
			return USB_REQUEST_STATUS_STALL;
		}
		num_ranges = (endpoint->setup.length - 9) / (2 * sizeof(frequencies[0]));
		if ((1 > num_ranges) || (MAX_RANGES < num_ranges)) {
			return USB_REQUEST_STATUS_STALL;
		}
		usb_transfer_schedule_block(
			endpoint->out,
			&data,
			endpoint->setup.length,
			NULL,
			NULL);
	} else if (stage == USB_TRANSFER_STAGE_DATA) {
		step_width = ((uint32_t) (data[3]) << 24) | ((uint32_t) (data[2]) << 16) |
			((uint32_t) (data[1]) << 8) | data[0];
		if (1 > step_width) {
			return USB_REQUEST_STATUS_STALL;
		}
		offset = ((uint32_t) (data[7]) << 24) | ((uint32_t) (data[6]) << 16) |
			((uint32_t) (data[5]) << 8) | data[4];
		style = data[8];
		if (INTERLEAVED < style) {
			return USB_REQUEST_STATUS_STALL;
		}
		for (i = 0; i < (num_ranges * 2); i++) {
			frequencies[i] =
				((uint16_t) (data[10 + i * 2]) << 8) + data[9 + i * 2];
		}
		sweep_freq = (uint64_t) frequencies[0] * FREQ_GRANULARITY;
		set_freq(sweep_freq + offset);
		usb_transfer_schedule_ack(endpoint->in);
	}
	return USB_REQUEST_STATUS_OK;
}

void sweep_bulk_transfer_complete(void* user_data, unsigned int bytes_transferred)
{
	(void) user_data;
	(void) bytes_transferred;

	// For each buffer transferred, we need to bump the count by three buffers
	// worth of data, to allow for the discarded buffers.
	m0_state.m4_count += 3 * 0x4000;
}

void sweep_mode(uint32_t seq)
{
	// Sweep mode is implemented using timed M0 operations, as follows:
	//
	// 0. M4 initially puts the M0 into RX mode, with an m0_count threshold
	//    of 16K and a next mode of WAIT.
	//
	// 1. M4 spins until the M0 switches to WAIT mode.
	//
	// 2. M0 captures one 16K block of samples, and switches to WAIT mode.
	//
	// 3. M4 sees the mode change, advances the m0_count target by 32K, and
	//    sets next mode to RX.
	//
	// 4. M4 adds the sweep metadata at the start of the block and
	//    schedules a bulk transfer for the block.
	//
	// 5. M4 retunes - this takes about 760us worst-case, so should be
	//    complete before the M0 goes back to RX.
	//
	// 6. M4 spins until the M0 mode changes to RX, then advances the
	//    m0_count limit by 16K and sets the next mode to WAIT.
	//
	// 7. Process repeats from step 1.

	unsigned int blocks_queued = 0;
	unsigned int phase = 0;
	bool odd = true;
	uint16_t range = 0;

	uint8_t* buffer;

	transceiver_startup(TRANSCEIVER_MODE_RX_SWEEP);

	// Set M0 to RX first buffer, then wait.
	m0_state.threshold = 0x4000;
	m0_state.next_mode = M0_MODE_WAIT;

	baseband_streaming_enable(&sgpio_config);

	while (transceiver_request.seq == seq) {
		// Wait for M0 to finish receiving a buffer.
		while (m0_state.active_mode != M0_MODE_WAIT)
			if (transceiver_request.seq != seq)
				goto end;

		// Set M0 to switch back to RX after two more buffers.
		m0_state.threshold += 0x8000;
		m0_state.next_mode = M0_MODE_RX;

		// Write metadata to buffer.
		buffer = &usb_bulk_buffer[phase * 0x4000];
		*buffer = 0x7f;
		*(buffer + 1) = 0x7f;
		*(buffer + 2) = sweep_freq & 0xff;
		*(buffer + 3) = (sweep_freq >> 8) & 0xff;
		*(buffer + 4) = (sweep_freq >> 16) & 0xff;
		*(buffer + 5) = (sweep_freq >> 24) & 0xff;
		*(buffer + 6) = (sweep_freq >> 32) & 0xff;
		*(buffer + 7) = (sweep_freq >> 40) & 0xff;
		*(buffer + 8) = (sweep_freq >> 48) & 0xff;
		*(buffer + 9) = (sweep_freq >> 56) & 0xff;

		// Set up IN transfer of buffer.
		usb_transfer_schedule_block(
			&usb_endpoint_bulk_in,
			buffer,
			0x4000,
			sweep_bulk_transfer_complete,
			NULL);

		// Use other buffer next time.
		phase = (phase + 1) % 2;

		if (++blocks_queued == dwell_blocks) {
			// Calculate next sweep frequency.
			if (INTERLEAVED == style) {
				if (!odd &&
				    ((sweep_freq + step_width) >=
				     ((uint64_t) frequencies[1 + range * 2] *
				      FREQ_GRANULARITY))) {
					range = (range + 1) % num_ranges;
					sweep_freq = (uint64_t) frequencies[range * 2] *
						FREQ_GRANULARITY;
				} else {
					if (odd) {
						sweep_freq += step_width / 4;
					} else {
						sweep_freq += 3 * step_width / 4;
					}
				}
				odd = !odd;
			} else {
				if ((sweep_freq + step_width) >=
				    ((uint64_t) frequencies[1 + range * 2] *
				     FREQ_GRANULARITY)) {
					range = (range + 1) % num_ranges;
					sweep_freq = (uint64_t) frequencies[range * 2] *
						FREQ_GRANULARITY;
				} else {
					sweep_freq += step_width;
				}
			}
			// Retune to new frequency.
			nvic_disable_irq(NVIC_USB0_IRQ);
			set_freq(sweep_freq + offset);
			nvic_enable_irq(NVIC_USB0_IRQ);
			blocks_queued = 0;
		}

		// Wait for M0 to resume RX.
		while (m0_state.active_mode != M0_MODE_RX)
			if (transceiver_request.seq != seq)
				goto end;

		// Set M0 to switch back to WAIT after filling next buffer.
		m0_state.threshold += 0x4000;
		m0_state.next_mode = M0_MODE_WAIT;
	}
end:
	transceiver_shutdown();
}
