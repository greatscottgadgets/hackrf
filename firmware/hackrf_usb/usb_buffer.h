/*
 * Copyright 2012-2022 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2012 Jared Boone
 * Copyright 2013 Benjamin Vernoux
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

#ifndef __USB_BUFFER_H__
#define __USB_BUFFER_H__

#include <stdbool.h>
#include <stdint.h>

#define USB_SAMP_BUFFER_SIZE 0x8000
#define USB_SAMP_BUFFER_MASK 0x7FFF

#define USB_BULK_BUFFER_SIZE 0x8000
#define USB_BULK_BUFFER_MASK 0x7FFF

/* Addresses of usb_samp_buffer and usb_bulk_buffer are set in ldscripts. If
 * you change the name of these variables, they won't be where they need to
 * be in the processor's address space, unless you also adjust the ldscripts.
 */
extern uint8_t usb_samp_buffer[USB_SAMP_BUFFER_SIZE];
extern uint8_t usb_bulk_buffer[USB_BULK_BUFFER_SIZE];

#endif /*__USB_BUFFER_H__*/
