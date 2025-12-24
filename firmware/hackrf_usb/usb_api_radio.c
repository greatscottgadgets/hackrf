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

#include "usb_api_radio.h"
#include <hackrf_core.h>
#include <usb_queue.h>
#include <radio.h>

#include <max2831.h>

typedef uint64_t set_center_frequency_params_t;

set_center_frequency_params_t set_center_frequency_params;

typedef struct {
	uint32_t rate_num;
	uint32_t rate_denom;
} set_sample_rate_params_t;

set_sample_rate_params_t set_sample_rate_params;

usb_request_status_t usb_vendor_request_set_mode_frequency(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		usb_transfer_schedule_block(
			endpoint->out,
			&set_center_frequency_params,
			sizeof(set_center_frequency_params_t),
			NULL,
			NULL);
		return USB_REQUEST_STATUS_OK;
	} else if (stage == USB_TRANSFER_STAGE_DATA) {
		const radio_mode_t mode = endpoint->setup.index;
		const uint64_t frequency_hz = set_center_frequency_params;
		radio_error_t result;
		result = radio_set_mode_frequency(
			&radio,
			RADIO_CHANNEL0,
			mode,
			frequency_hz);
		if (result != RADIO_OK) {
			return USB_REQUEST_STATUS_STALL;
		}
		usb_transfer_schedule_ack(endpoint->in);
	}

	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_mode_sample_rate(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		usb_transfer_schedule_block(
			endpoint->out,
			&set_sample_rate_params,
			sizeof(set_sample_rate_params_t),
			NULL,
			NULL);
		return USB_REQUEST_STATUS_OK;

	} else if (stage == USB_TRANSFER_STAGE_DATA) {
		const radio_mode_t mode = endpoint->setup.index;
		const uint32_t rate_num = set_sample_rate_params.rate_num;
		const uint32_t rate_denom = set_sample_rate_params.rate_denom;
		radio_error_t result;
		result = radio_set_mode_sample_rate(
			&radio,
			RADIO_CHANNEL0,
			mode,
			rate_num,
			rate_denom);
		if (result != RADIO_OK) {
			return USB_REQUEST_STATUS_STALL;
		}
		usb_transfer_schedule_ack(endpoint->in);
	}

	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_supported_sample_rate(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		const radio_mode_t mode = endpoint->setup.index;
		radio_range_t range;
		radio_error_t result;
		result =
			radio_supported_sample_rate(&radio, RADIO_CHANNEL0, mode, &range);
		if (result != RADIO_OK) {
			return USB_REQUEST_STATUS_STALL;
		}

		uint8_t size = sizeof(radio_range_t);
		memcpy(endpoint->buffer, &range, size);

		usb_transfer_schedule_block(
			endpoint->in,
			endpoint->buffer,
			size,
			NULL,
			NULL);
		usb_transfer_schedule_ack(endpoint->out);
	}

	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_get_sample_rate_element(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		radio_mode_t mode = endpoint->setup.index;
		radio_sample_rate_id sample_rate_id = endpoint->setup.value;
		radio_sample_rate_t sample_rate = radio_get_sample_rate_element(
			&radio,
			RADIO_CHANNEL0,
			mode,
			sample_rate_id);

		uint8_t size = sizeof(radio_sample_rate_t);
		memcpy(endpoint->buffer, &sample_rate, size);

		usb_transfer_schedule_block(
			endpoint->in,
			endpoint->buffer,
			size,
			NULL,
			NULL);
		usb_transfer_schedule_ack(endpoint->out);
	}

	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_get_filter_element(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		radio_mode_t mode = endpoint->setup.index;
		radio_filter_id filter_id = endpoint->setup.value;
		radio_filter_t filter =
			radio_get_filter_element(&radio, RADIO_CHANNEL0, mode, filter_id);

		uint8_t size = sizeof(radio_filter_t);
		memcpy(endpoint->buffer, &filter, size);

		usb_transfer_schedule_block(
			endpoint->in,
			endpoint->buffer,
			size,
			NULL,
			NULL);
		usb_transfer_schedule_ack(endpoint->out);
	}

	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_supported_filter_element_bandwidths(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		radio_mode_t mode = endpoint->setup.index;
		radio_filter_id filter_id = endpoint->setup.value;
		size_t length = 0;
		radio_error_t result;
		result = radio_supported_filter_element_bandwidths(
			&radio,
			RADIO_CHANNEL0,
			mode,
			filter_id,
			(uint32_t*) &endpoint->buffer,
			&length);
		if (result != RADIO_OK) {
			return USB_REQUEST_STATUS_STALL;
		}

		usb_transfer_schedule_block(
			endpoint->in,
			endpoint->buffer,
			length * sizeof(uint32_t),
			NULL,
			NULL);
		usb_transfer_schedule_ack(endpoint->out);
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_get_frequency_element(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		radio_mode_t mode = endpoint->setup.index;
		radio_frequency_id frequency_id = endpoint->setup.value;
		radio_frequency_t frequency = radio_get_frequency_element(
			&radio,
			RADIO_CHANNEL0,
			mode,
			frequency_id);

		uint8_t size = sizeof(radio_frequency_t);
		memcpy(endpoint->buffer, &frequency, size);

		usb_transfer_schedule_block(
			endpoint->in,
			endpoint->buffer,
			size,
			NULL,
			NULL);
		usb_transfer_schedule_ack(endpoint->out);
	}

	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_get_gain_element(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		radio_mode_t mode = endpoint->setup.index;
		radio_gain_id gain_id = endpoint->setup.value;
		radio_gain_t gain =
			radio_get_gain_element(&radio, RADIO_CHANNEL0, mode, gain_id);

		uint8_t size = sizeof(radio_gain_t);
		memcpy(endpoint->buffer, &gain, size);

		usb_transfer_schedule_block(
			endpoint->in,
			endpoint->buffer,
			size,
			NULL,
			NULL);
		usb_transfer_schedule_ack(endpoint->out);
	}

	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_get_antenna_element(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		radio_mode_t mode = endpoint->setup.index;
		radio_antenna_id antenna_id = endpoint->setup.value;
		radio_antenna_t antenna = radio_get_antenna_element(
			&radio,
			RADIO_CHANNEL0,
			mode,
			antenna_id);

		uint8_t size = sizeof(radio_antenna_t);
		memcpy(endpoint->buffer, &antenna, size);

		usb_transfer_schedule_block(
			endpoint->in,
			endpoint->buffer,
			size,
			NULL,
			NULL);
		usb_transfer_schedule_ack(endpoint->out);
	}

	return USB_REQUEST_STATUS_OK;
}
