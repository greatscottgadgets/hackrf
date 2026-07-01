/*
 * Copyright 2026 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include <stddef.h>
#include <stdint.h>

#include <fixed_point.h>
#include <radio.h>
#include <usb_request.h>
#include <usb_type.h>

#include "usb_queue.h"

usb_request_status_t usb_vendor_request_set_radio_mode(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		radio_config_mode_t mode = endpoint->setup.value;
		if (!radio_set_config_mode(&radio, mode)) {
			return USB_REQUEST_STATUS_STALL;
		}
		usb_transfer_schedule_ack(endpoint->in);
	}

	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_radio_frequency(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	static fp_40_24_t hz_param;

	if (stage == USB_TRANSFER_STAGE_SETUP) {
		usb_transfer_schedule_block(
			endpoint->out,
			&hz_param,
			sizeof(fp_40_24_t),
			NULL,
			NULL);
	} else if (stage == USB_TRANSFER_STAGE_DATA) {
		radio_reg_write(&radio, RADIO_BANK_ACTIVE, RADIO_FREQUENCY_RF, hz_param);
		radio_reg_write(
			&radio,
			RADIO_BANK_ACTIVE,
			RADIO_FREQUENCY_IF,
			RADIO_UNSET);
		radio_reg_write(
			&radio,
			RADIO_BANK_ACTIVE,
			RADIO_FREQUENCY_LO,
			RADIO_UNSET);
		radio_reg_write(
			&radio,
			RADIO_BANK_ACTIVE,
			RADIO_IMAGE_REJECT,
			RADIO_UNSET);
		usb_transfer_schedule_ack(endpoint->in);
	}

	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_radio_frequency_explicit(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	static struct {
		fp_40_24_t if_freq_hz;
		fp_40_24_t lo_freq_hz;
		uint8_t path;
	} params;

	if (stage == USB_TRANSFER_STAGE_SETUP) {
		usb_transfer_schedule_block(
			endpoint->out,
			&params,
			sizeof(params),
			NULL,
			NULL);
	} else if (stage == USB_TRANSFER_STAGE_DATA) {
		radio_reg_write(
			&radio,
			RADIO_BANK_ACTIVE,
			RADIO_FREQUENCY_IF,
			params.if_freq_hz);
		radio_reg_write(
			&radio,
			RADIO_BANK_ACTIVE,
			RADIO_FREQUENCY_LO,
			params.lo_freq_hz);
		radio_reg_write(
			&radio,
			RADIO_BANK_ACTIVE,
			RADIO_IMAGE_REJECT,
			params.path);
		usb_transfer_schedule_ack(endpoint->in);
	}

	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_radio_sample_rate(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	static fp_28_36_t sps_param;

	if (stage == USB_TRANSFER_STAGE_SETUP) {
		usb_transfer_schedule_block(
			endpoint->out,
			&sps_param,
			sizeof(fp_28_36_t),
			NULL,
			NULL);
	} else if (stage == USB_TRANSFER_STAGE_DATA) {
		radio_reg_write(&radio, RADIO_BANK_ACTIVE, RADIO_SAMPLE_RATE, sps_param);
		usb_transfer_schedule_ack(endpoint->in);
	}

	return USB_REQUEST_STATUS_OK;
}
