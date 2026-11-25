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
#include <string.h>

#include <fixed_point.h>
#include <radio.h>
#include <transceiver_mode.h>
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
	static fp_40_24_t freq_hz;

	if (stage == USB_TRANSFER_STAGE_SETUP) {
		usb_transfer_schedule_block(
			endpoint->out,
			&freq_hz,
			sizeof(fp_40_24_t),
			NULL,
			NULL);
	} else if (stage == USB_TRANSFER_STAGE_DATA) {
		radio_reg_write(&radio, RADIO_BANK_REQUESTED, RADIO_FREQUENCY_RF, freq_hz);
		radio_reg_write(
			&radio,
			RADIO_BANK_REQUESTED,
			RADIO_FREQUENCY_IF,
			RADIO_UNSET);
		radio_reg_write(
			&radio,
			RADIO_BANK_REQUESTED,
			RADIO_FREQUENCY_LO,
			RADIO_UNSET);
		radio_reg_write(
			&radio,
			RADIO_BANK_REQUESTED,
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
	typedef struct {
		fp_40_24_t if_freq_hz;
		fp_40_24_t lo_freq_hz;
		uint8_t path;
	} params_t;

	static params_t params;

	if (stage == USB_TRANSFER_STAGE_SETUP) {
		usb_transfer_schedule_block(
			endpoint->out,
			&params,
			sizeof(params_t),
			NULL,
			NULL);
	} else if (stage == USB_TRANSFER_STAGE_DATA) {
		radio_reg_write(
			&radio,
			RADIO_BANK_REQUESTED,
			RADIO_FREQUENCY_IF,
			params.if_freq_hz);
		radio_reg_write(
			&radio,
			RADIO_BANK_REQUESTED,
			RADIO_FREQUENCY_LO,
			params.lo_freq_hz);
		radio_reg_write(
			&radio,
			RADIO_BANK_REQUESTED,
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
	typedef struct {
		fp_28_36_t sample_rate;
		uint64_t resample_rate;
	} params_t;

	static params_t params;

	if (stage == USB_TRANSFER_STAGE_SETUP) {
		usb_transfer_schedule_block(
			endpoint->out,
			&params,
			sizeof(params_t),
			NULL,
			NULL);
	} else if (stage == USB_TRANSFER_STAGE_DATA) {
		if (params.resample_rate != 0) {
			// resample_n = log2(resample_rate)
			uint64_t resample_n = 0;
			while (params.resample_rate >>= 1) {
				resample_n++;
			}
			// For now we just set both as we don't currently have a
			// public-facing API to define independent transmission
			// and receiption configurations for the radio.
			radio_reg_write(
				&radio,
				RADIO_BANK_REQUESTED,
				RADIO_RESAMPLE_RX,
				resample_n);
			radio_reg_write(
				&radio,
				RADIO_BANK_REQUESTED,
				RADIO_RESAMPLE_TX,
				resample_n);
		}
		radio_reg_write(
			&radio,
			RADIO_BANK_REQUESTED,
			RADIO_SAMPLE_RATE,
			params.sample_rate);
		usb_transfer_schedule_ack(endpoint->in);
	}

	// Force an update to avoid a race condition when the sample rate
	// is requested before the radio has updated. TODO discuss.
	radio_update(&radio);

	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_get_radio_sample_rate(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	typedef struct {
		fp_28_36_t sample_rate;
		uint64_t resample_rate;
	} params_t;

	radio_register_t resample_reg;
	uint64_t opmode = 0;
	uint64_t sample_rate = 0;
	uint64_t resample_n = 0;

	// Try to get the currently applied transceiver mode so we know
	// whether we need to return RADIO_RESAMPLE_RX or
	// RADIO_RESAMPLE_TX. If there is no transceiver mode applied, get
	// the most recently requested mode.
	opmode = radio_reg_read(&radio, RADIO_BANK_APPLIED, RADIO_OPMODE);
	if (opmode == RADIO_UNSET) {
		opmode = radio_reg_read(&radio, RADIO_BANK_REQUESTED, RADIO_OPMODE);
	}
	switch (opmode) {
	case TRANSCEIVER_MODE_TX:
	case TRANSCEIVER_MODE_SS:
		resample_reg = RADIO_RESAMPLE_TX;
		break;
	default:
		resample_reg = RADIO_RESAMPLE_RX;
		break;
	}

	// Return the currently applied sample and resample rates.
	sample_rate = radio_reg_read(&radio, RADIO_BANK_APPLIED, RADIO_SAMPLE_RATE);
	resample_n = radio_reg_read(&radio, RADIO_BANK_APPLIED, resample_reg);

	uint8_t length = sizeof(params_t);
	params_t params = {
		.sample_rate = (fp_28_36_t) sample_rate,
		.resample_rate = 1 << resample_n,
	};
	memcpy(&endpoint->buffer, &params, length);

	if (stage == USB_TRANSFER_STAGE_SETUP) {
		usb_transfer_schedule_block(
			endpoint->in,
			&endpoint->buffer,
			length,
			NULL,
			NULL);
		usb_transfer_schedule_ack(endpoint->out);
	}

	return USB_REQUEST_STATUS_OK;
}
