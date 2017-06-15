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

#ifndef __USB_API_MODE_H__
#define __USB_API_MODE_H__

#include <usb_type.h>
#include <usb_request.h>

typedef enum {
	HACKRF_MODE_IDLE = 0,
	HACKRF_MODE_RX = 1,
	HACKRF_MODE_TX = 2,
	HACKRF_MODE_SWEEP = 3,
	HACKRF_MODE_CPLD = 4
} hackrf_mode_t;

extern volatile hackrf_mode_t hackrf_mode;

int set_hackrf_mode(hackrf_mode_t _hackrf_mode);

usb_request_status_t usb_vendor_request_set_mode(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage);

#endif /* __USB_API_MODE_H__ */