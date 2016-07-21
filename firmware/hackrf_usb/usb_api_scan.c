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
//#include <hackrf_core.h>
#include "tuning.h"

volatile bool scan_mode = false;
static uint64_t scan_freq;
static uint64_t scan_freq_min;
static uint64_t scan_freq_max;
static uint64_t scan_freq_step;

usb_request_status_t usb_vendor_request_init_scan(
		usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		// DGS set scan frequencies here
		scan_freq = scan_freq_min;
		set_freq(scan_freq);
		scan_mode = true;
		usb_transfer_schedule_ack(endpoint->in);
	}
	return USB_REQUEST_STATUS_OK;
}

void scan_callback(void) {
	scan_freq += scan_freq_step;
	if (scan_freq > scan_freq_max) {
		scan_freq = scan_freq_min;
		}
		set_freq(scan_freq);
}
