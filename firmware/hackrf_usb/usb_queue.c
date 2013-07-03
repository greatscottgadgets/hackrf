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

#include "usb.h"
#include "usb_queue.h"

struct _usb_transfer_t {
        struct _usb_transfer_t* next;
        usb_transfer_descriptor_t td ATTR_ALIGNED(64);
        unsigned int actual_length;
        usb_endpoint_t* endpoint;
        bool finished;
        transfer_completion_cb completion_cb;
};

usb_transfer_t transfer_pool[16];

// Available transfer list
usb_transfer_t* free_transfers;

#define USB_ENDPOINT_INDEX(endpoint_address) (((endpoint_address & 0xF) * 2) + ((endpoint_address >> 7) & 1))

// Pending transfer heads
usb_transfer_t* endpoint_transfers[12] = {};

void init_transfers() {
        usb_transfer_t* t = &transfer_pool[0];
        free_transfers = t;
        for (unsigned int i=0; i < sizeof(transfer_pool) / sizeof(usb_transfer_t); i++, t++) {
                t->next = t+1;
        }
        t->next = NULL;
}

static bool usb_endpoint_is_in(const uint_fast8_t endpoint_address) {
	return (endpoint_address & 0x80) ? true : false;
}

#if 0
static usb_transfer_t* usb_transfer(
	const uint_fast8_t endpoint_address
) {
	return endpoint_transfers[USB_ENDPOINT_INDEX(endpoint_address)];
}
#endif

static void fill_in_transfer(usb_transfer_t* transfer,
                             void* const data,
                             const uint32_t maximum_length
) {
        usb_transfer_descriptor_t* const td = &transfer->td;

	// Configure a transfer.
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
}
                             
static usb_transfer_t* allocate_transfer()
{
        while (free_transfers == NULL);
        //disable_irqs(); // FIXME
        usb_transfer_t* const transfer = free_transfers;
        free_transfers = transfer->next;
        //enable_irqs();
        transfer->finished = false;
        return transfer;
}

static void endpoint_add_transfer(
        const usb_endpoint_t* const endpoint,
        usb_transfer_t* const transfer
) {
        uint_fast8_t index = USB_ENDPOINT_INDEX(endpoint->address);
        //FIXME disable_irqs();
        usb_transfer_t* t = endpoint_transfers[index];
        transfer->next = NULL;
        for (; t->next != NULL; t = t->next);
        t->next = transfer;
        //enable_irqs();
}

static usb_transfer_t* endpoint_pop_transfer(
        const usb_endpoint_t* const endpoint
) {
        uint_fast8_t index = USB_ENDPOINT_INDEX(endpoint->address);
        //FIXME disable_irqs();
        usb_transfer_t* transfer = endpoint_transfers[index];
        endpoint_transfers[index] = transfer->next;
        //enable_irqs();
        return transfer;
}

void usb_transfer_schedule_wait(
	const usb_endpoint_t* const endpoint,
	void* const data,
	const uint32_t maximum_length
) {
        usb_transfer_t* const transfer = allocate_transfer();
        fill_in_transfer(transfer, data, maximum_length);
        endpoint_add_transfer(endpoint, transfer);
        usb_endpoint_schedule_wait(endpoint, &transfer->td);
}

void usb_transfer_schedule_append(
	const usb_endpoint_t* const endpoint,
	void* const data,
	const uint32_t maximum_length
) {
        usb_transfer_t* const transfer = allocate_transfer();
        uint_fast8_t index = USB_ENDPOINT_INDEX(endpoint->address);
        fill_in_transfer(transfer, data, maximum_length);
        // TODO: disable_interrupts();
        usb_transfer_t* tail = endpoint_transfers[index];
        for (; tail->next != NULL; tail = tail->next);
        endpoint_add_transfer(endpoint, transfer);
        usb_endpoint_schedule_append(endpoint, &tail->td, &transfer->td);
        //enable_interrupts();
}
	
void usb_transfer_schedule_ack(
	const usb_endpoint_t* const endpoint
) {
	usb_transfer_schedule_wait(endpoint, 0, 0);
}

void transfer_complete(const usb_endpoint_t* const endpoint)
{
        usb_transfer_t* transfer = endpoint_pop_transfer(endpoint);
        unsigned int transferred = transfer->actual_length - transfer->td.total_bytes;
        if (transfer->completion_cb)
                transfer->completion_cb(transfer, transferred);
        else if (usb_endpoint_is_in(transfer->endpoint->address))
                transfer->finished = true;
}
