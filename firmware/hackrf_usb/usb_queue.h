/*
 * Copyright 2012 Jared Boone
 * Copyright 2013 Ben Gamari
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

#ifndef __USB_QUEUE_H__
#define __USB_QUEUE_H__

#include <libopencm3/lpc43xx/usb.h>

#include "usb_type.h"

typedef struct _usb_transfer_t usb_transfer_t;

typedef void (*transfer_completion_cb)(struct _usb_transfer_t*, unsigned int);

void usb_endpoint_schedule(
	const usb_endpoint_t* const endpoint,
	void* const data,
	const uint32_t maximum_length
);

void usb_transfer_schedule_wait(
	const usb_endpoint_t* const endpoint,
	void* const data,
	const uint32_t maximum_length
);

void usb_transfer_schedule_append(
	const usb_endpoint_t* const endpoint,
	void* const data,
	const uint32_t maximum_length
);

void usb_transfer_schedule_ack(
	const usb_endpoint_t* const endpoint
);

void usb_transfer_schedule(
	const usb_endpoint_t* const endpoint,
	void* const data,
	const uint32_t maximum_length
);

void init_transfers(void);

#endif//__USB_QUEUE_H__
