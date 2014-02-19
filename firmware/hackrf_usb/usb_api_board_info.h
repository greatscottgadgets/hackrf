/*
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

#ifndef __USB_API_BOARD_INFO_H__
#define __USB_API_BOARD_INFO_H__

#include <stdint.h>

#include <usb_type.h>
#include <usb_request.h>

typedef struct {
	uint32_t part_id[2];
	uint32_t serial_no[4];
} read_partid_serialno_t;

usb_request_status_t usb_vendor_request_read_board_id(
	usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage);
usb_request_status_t usb_vendor_request_read_version_string(
	usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage);
usb_request_status_t usb_vendor_request_read_partid_serialno(
	usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage);

#endif /* end of include guard: __USB_API_BOARD_INFO_H__ */
