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

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

#include <libopencm3/cm3/cortex.h>

#include "usb.h"
#include "usb_queue.h"

struct _usb_transfer_t {
        struct _usb_transfer_t* next;
        usb_transfer_descriptor_t td ATTR_ALIGNED(64);
        unsigned int maximum_length;
        usb_endpoint_t* endpoint;
        transfer_completion_cb completion_cb;
};

usb_transfer_t transfer_pool[8];
const unsigned int transfer_pool_size = sizeof(transfer_pool) / sizeof(usb_transfer_t);

// Available transfer list
usb_transfer_t* volatile free_transfers;

#define USB_ENDPOINT_INDEX(endpoint_address) (((endpoint_address & 0xF) * 2) + ((endpoint_address >> 7) & 1))

// Pending transfer heads
usb_transfer_t* volatile endpoint_transfers[12] = {};

void usb_queue_init() {
        usb_transfer_t* t = &transfer_pool[0];
        free_transfers = t;
        for (unsigned int i=0; i < transfer_pool_size - 1; i++, t++) {
                t->next = t+1;
        }
        t->next = NULL;
}

/* Allocate a transfer */
static usb_transfer_t* allocate_transfer()
{
        while (free_transfers == NULL);
        usb_transfer_t* const transfer = free_transfers;
        free_transfers = transfer->next;
        return transfer;
}

/* Place a transfer in the free list */
static void free_transfer(usb_transfer_t* const transfer)
{
        transfer->next = free_transfers;
        free_transfers = transfer;
}

/* Add a transfer to the end of an endpoint's queue */
static void endpoint_add_transfer(
        const usb_endpoint_t* const endpoint,
        usb_transfer_t* const transfer
) {
        uint_fast8_t index = USB_ENDPOINT_INDEX(endpoint->address);
        transfer->next = NULL;
        if (endpoint_transfers[index] != NULL) {
            usb_transfer_t* t = endpoint_transfers[index];
            for (; t->next != NULL; t = t->next);
            t->next = transfer;
        } else {
            endpoint_transfers[index] = transfer;
        }
}
                
void usb_queue_flush_endpoint(const usb_endpoint_t* const endpoint)
{
        uint_fast8_t index = USB_ENDPOINT_INDEX(endpoint->address);
        cc_disable_interrupts();
        while (endpoint_transfers[index]) {
                usb_transfer_t * transfer = endpoint_transfers[index];
                endpoint_transfers[index] = transfer->next;
                free_transfer(transfer);
        }
        cc_enable_interrupts();
}

void usb_transfer_schedule(
	const usb_endpoint_t* const endpoint,
	void* const data,
	const uint32_t maximum_length,
        const transfer_completion_cb completion_cb
) {
        usb_transfer_t* const transfer = allocate_transfer();
        assert(transfer != NULL);
        usb_transfer_descriptor_t* const td = &transfer->td;
        uint_fast8_t index = USB_ENDPOINT_INDEX(endpoint->address);

	// Configure the transfer descriptor
        td->next_dtd_pointer = USB_TD_NEXT_DTD_POINTER_TERMINATE;
	td->total_bytes =
		  USB_TD_DTD_TOKEN_TOTAL_BYTES(maximum_length)
		| USB_TD_DTD_TOKEN_IOC
		| USB_TD_DTD_TOKEN_MULTO(0)
		| USB_TD_DTD_TOKEN_STATUS_ACTIVE
		;
	td->buffer_pointer_page[0] =  (uint32_t)data;
	td->buffer_pointer_page[1] = ((uint32_t)data + 0x1000) & 0xfffff000;
	td->buffer_pointer_page[2] = ((uint32_t)data + 0x2000) & 0xfffff000;
	td->buffer_pointer_page[3] = ((uint32_t)data + 0x3000) & 0xfffff000;
	td->buffer_pointer_page[4] = ((uint32_t)data + 0x4000) & 0xfffff000;

        // Fill in transfer fields
        transfer->maximum_length = maximum_length;
        transfer->completion_cb = completion_cb;
        transfer->endpoint = (usb_endpoint_t*) endpoint;

        cc_disable_interrupts();
        usb_transfer_t* tail = endpoint_transfers[index];
        endpoint_add_transfer(endpoint, transfer);
        if (tail == NULL) {
                // The queue is currently empty, we need to re-prime
                usb_endpoint_schedule_wait(endpoint, &transfer->td);
        } else {
                // The queue is currently running, try to append
                for (; tail->next != NULL; tail = tail->next);
                usb_endpoint_schedule_append(endpoint, &tail->td, &transfer->td);
        }
        cc_enable_interrupts();
}
	
void usb_transfer_schedule_ack(
	const usb_endpoint_t* const endpoint
) {
        usb_transfer_schedule(endpoint, 0, 0, NULL);
}

/* Called when an endpoint might have completed a transfer */
void usb_queue_transfer_complete(usb_endpoint_t* const endpoint)
{
        uint_fast8_t index = USB_ENDPOINT_INDEX(endpoint->address);
        usb_transfer_t* transfer = endpoint_transfers[index];

        while (transfer != NULL) {
                uint8_t status = transfer->td.total_bytes;

                // Check for failures
                if (   status & USB_TD_DTD_TOKEN_STATUS_HALTED
                    || status & USB_TD_DTD_TOKEN_STATUS_BUFFER_ERROR
                    || status & USB_TD_DTD_TOKEN_STATUS_TRANSACTION_ERROR) {
                        // TODO: Uh oh, do something useful here
                        while (1);
                }

                // Still not finished
                if (status & USB_TD_DTD_TOKEN_STATUS_ACTIVE) 
                        break;

                // Advance the head. We need to do this before invoking the completion
                // callback as it might attempt to schedule a new transfer
                endpoint_transfers[index] = transfer->next;
                usb_transfer_t* next = transfer->next;

                // Invoke completion callback
                unsigned int total_bytes = (transfer->td.total_bytes & USB_TD_DTD_TOKEN_TOTAL_BYTES_MASK) >> USB_TD_DTD_TOKEN_TOTAL_BYTES_SHIFT;
                unsigned int transferred = transfer->maximum_length - total_bytes;
                if (transfer->completion_cb)
                        transfer->completion_cb(transfer, transferred);

                // Advance head and free transfer
                free_transfer(transfer);
                transfer = next;
        }
}
