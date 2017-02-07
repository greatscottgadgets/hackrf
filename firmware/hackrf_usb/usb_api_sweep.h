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

#ifndef __USB_API_SWEEP_H__
#define __USB_API_SWEEP_H__

#include <stdbool.h>
#include <usb_type.h>
#include <usb_request.h>

extern volatile bool start_sweep_mode;

enum sweep_style {
	LINEAR = 0,
	INTERLEAVED = 1,
};

usb_request_status_t usb_vendor_request_init_sweep(
	usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage);

void sweep_mode(void);

#endif /* __USB_API_SWEEP_H__ */
