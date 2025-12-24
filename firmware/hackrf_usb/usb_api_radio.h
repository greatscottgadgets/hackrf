/*
 * Copyright 2012-2025 Great Scott Gadgets <info@greatscottgadgets.com>
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

#ifndef __USB_API_RADIO_H__
#define __USB_API_RADIO_H__

#include <usb_type.h>
#include <usb_request.h>

usb_request_status_t usb_vendor_request_set_mode_frequency(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage);

usb_request_status_t usb_vendor_request_set_mode_sample_rate(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage);

usb_request_status_t usb_vendor_request_supported_sample_rate(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage);

usb_request_status_t usb_vendor_request_get_sample_rate_element(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage);

usb_request_status_t usb_vendor_request_get_filter_element(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage);

usb_request_status_t usb_vendor_request_supported_filter_element_bandwidths(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage);

usb_request_status_t usb_vendor_request_get_frequency_element(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage);

usb_request_status_t usb_vendor_request_get_gain_element(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage);

usb_request_status_t usb_vendor_request_get_antenna_element(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage);

#endif /* end of include guard: __USB_API_RADIO_H__ */
