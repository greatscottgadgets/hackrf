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
typedef struct _usb_queue_t usb_queue_t;
typedef void (*transfer_completion_cb)(void*, unsigned int);

// This is an opaque datatype. Thou shall not touch these members.
struct _usb_transfer_t {
        struct _usb_transfer_t* next;
        usb_transfer_descriptor_t td ATTR_ALIGNED(64);
        unsigned int maximum_length;
        struct _usb_queue_t* queue;
        transfer_completion_cb completion_cb;
        void* user_data;
};

// This is an opaque datatype. Thou shall not touch these members.
struct _usb_queue_t {
        struct usb_endpoint_t* endpoint;
        const unsigned int pool_size;
        usb_transfer_t* volatile free_transfers;
        usb_transfer_t* volatile active;
};

#define USB_DECLARE_QUEUE(endpoint_name)                                \
        struct _usb_queue_t endpoint_name##_queue;
#define USB_DEFINE_QUEUE(endpoint_name, _pool_size)                     \
        struct _usb_transfer_t endpoint_name##_transfers[_pool_size];   \
        struct _usb_queue_t endpoint_name##_queue = {                   \
                .endpoint = &endpoint_name,                             \
                .free_transfers = endpoint_name##_transfers,            \
                .pool_size = _pool_size                                 \
        };

void usb_queue_flush_endpoint(const usb_endpoint_t* const endpoint);

int usb_transfer_schedule(
	const usb_endpoint_t* const endpoint,
	void* const data,
	const uint32_t maximum_length,
        const transfer_completion_cb completion_cb,
        void* const user_data
);

int usb_transfer_schedule_block(
	const usb_endpoint_t* const endpoint,
	void* const data,
	const uint32_t maximum_length,
        const transfer_completion_cb completion_cb,
        void* const user_data
);

int usb_transfer_schedule_ack(
	const usb_endpoint_t* const endpoint
);

void usb_queue_init(
        usb_queue_t* const queue
);

void usb_queue_transfer_complete(
        usb_endpoint_t* const endpoint
);

#endif//__USB_QUEUE_H__
