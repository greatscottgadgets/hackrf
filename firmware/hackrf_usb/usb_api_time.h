/*
 * Copyright 2025 Fabrizio Pollastri <mxgbot@gmail.com>
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

#ifndef __USB_API_TIME_H__
#define __USB_API_TIME_H__

#include <libopencm3/lpc43xx/cgu.h>
#include <usb_type.h>
#include <usb_request.h>


usb_request_status_t usb_vendor_request_time_set_divisor_next_pps(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage);

usb_request_status_t usb_vendor_request_time_set_divisor_one_pps(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage);

usb_request_status_t usb_vendor_request_time_set_trig_delay_next_pps(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage);

usb_request_status_t usb_vendor_request_time_get_seconds_now(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage);

usb_request_status_t usb_vendor_request_time_set_seconds_now(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage);

usb_request_status_t usb_vendor_request_time_set_seconds_next_pps(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage);

usb_request_status_t usb_vendor_request_time_get_ticks_now(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage);

usb_request_status_t usb_vendor_request_time_set_ticks_now(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage);




void time_timer_init(void);
void time_set_divisor_next_pps(unsigned long int divisor);
void time_set_divisor_one_pps(unsigned long int divisor);
void time_set_trig_delay_next_pps(unsigned long int trig_delay);
long long int time_get_seconds_now(void);
void time_set_seconds_now(long long int seconds);
void time_set_seconds_next_pps(long long int seconds);
unsigned long int time_get_ticks_now();
void time_set_ticks_now(uint32_t ticks);

#endif // __USB_API_TIME_H__
