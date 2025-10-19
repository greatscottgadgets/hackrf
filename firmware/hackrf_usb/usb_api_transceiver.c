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

#include "usb_api_transceiver.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <clock_gen.h>
#include <drivers.h>
#include <fixed_point.h>
#include <leds.h>
#include <m0_state.h>
#include <operacake_sctimer.h>
#include <platform_detect.h>
#include <radio.h>
#include <streaming.h>
#include <transceiver_mode.h>
#include <usb.h>
#include <usb_queue.h>
#include <usb_request.h>
#include <usb_type.h>

#include "usb_buffer.h"
#include "usb_endpoint.h"

#define USB_TRANSFER_SIZE 0x4000
#define DMA_TRANSFER_SIZE 0x2000

#define BUF_HALF_MASK (USB_SAMP_BUFFER_SIZE >> 1)

volatile uint32_t dma_started, dma_pending, usb_started, usb_completed;

typedef struct {
	uint32_t freq_mhz;
	uint32_t freq_hz;
} set_freq_params_t;

set_freq_params_t set_freq_params;

struct set_freq_explicit_params {
	uint64_t if_freq_hz; /* intermediate frequency */
	uint64_t lo_freq_hz; /* front-end local oscillator frequency */
	uint8_t path;        /* image rejection filter path */
};

struct set_freq_explicit_params explicit_params;

typedef struct {
	uint32_t freq_hz;
	uint32_t divider;
} set_sample_r_params_t;

set_sample_r_params_t set_sample_r_params;

void dma_isr(void);

usb_request_status_t usb_vendor_request_set_baseband_filter_bandwidth(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		const uint32_t bandwidth =
			(endpoint->setup.index << 16) | endpoint->setup.value;
		radio_reg_write(
			&radio,
			RADIO_BANK_ACTIVE,
			RADIO_BB_BANDWIDTH_TX,
			bandwidth);
		radio_reg_write(
			&radio,
			RADIO_BANK_ACTIVE,
			RADIO_BB_BANDWIDTH_RX,
			bandwidth);
		usb_transfer_schedule_ack(endpoint->in);
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_freq(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		usb_transfer_schedule_block(
			endpoint->out,
			&set_freq_params,
			sizeof(set_freq_params_t),
			NULL,
			NULL);
	} else if (stage == USB_TRANSFER_STAGE_DATA) {
		const uint64_t freq =
			set_freq_params.freq_mhz * 1000000ULL + set_freq_params.freq_hz;
		radio_reg_write(
			&radio,
			RADIO_BANK_ACTIVE,
			RADIO_FREQUENCY_RF,
			freq * FP_ONE_HZ);
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

usb_request_status_t usb_vendor_request_set_freq_explicit(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		usb_transfer_schedule_block(
			endpoint->out,
			&explicit_params,
			sizeof(struct set_freq_explicit_params),
			NULL,
			NULL);
	} else if (stage == USB_TRANSFER_STAGE_DATA) {
		radio_reg_write(
			&radio,
			RADIO_BANK_ACTIVE,
			RADIO_FREQUENCY_IF,
			explicit_params.if_freq_hz * FP_ONE_HZ);
		radio_reg_write(
			&radio,
			RADIO_BANK_ACTIVE,
			RADIO_FREQUENCY_LO,
			explicit_params.lo_freq_hz * FP_ONE_HZ);
		radio_reg_write(
			&radio,
			RADIO_BANK_ACTIVE,
			RADIO_IMAGE_REJECT,
			explicit_params.path);
		usb_transfer_schedule_ack(endpoint->in);
	}
	return USB_REQUEST_STATUS_OK;
}

/*
 * Convert fractional sample rate to units of 1/(2**36) Hz.
 */
static inline fp_28_36_t round_sample_rate(uint64_t num, uint32_t denom)
{
	uint64_t q1, r1, q2, r2, q3;

	if (denom == 0) {
		denom = 1;
	}

	q1 = num / denom;
	r1 = num % denom;

	q2 = (r1 << 32) / denom;
	r2 = (r1 << 32) % denom;

	q3 = ((r2 << 4) + (denom >> 1)) / denom;

	return (q1 << 36) + (q2 << 4) + q3;
}

usb_request_status_t usb_vendor_request_set_sample_rate_frac(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		usb_transfer_schedule_block(
			endpoint->out,
			&set_sample_r_params,
			sizeof(set_sample_r_params_t),
			NULL,
			NULL);
	} else if (stage == USB_TRANSFER_STAGE_DATA) {
		uint32_t numerator = set_sample_r_params.freq_hz;
		uint32_t denominator = set_sample_r_params.divider;
		uint64_t value = round_sample_rate(numerator, denominator);
		radio_reg_write(&radio, RADIO_BANK_ACTIVE, RADIO_SAMPLE_RATE, value);
		usb_transfer_schedule_ack(endpoint->in);
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_amp_enable(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		radio_reg_write(
			&radio,
			RADIO_BANK_ACTIVE,
			RADIO_GAIN_TX_RF,
			endpoint->setup.value);
		radio_reg_write(
			&radio,
			RADIO_BANK_ACTIVE,
			RADIO_GAIN_RX_RF,
			endpoint->setup.value);
		usb_transfer_schedule_ack(endpoint->in);
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_lna_gain(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		uint8_t gain = endpoint->setup.index;
		radio_reg_write(&radio, RADIO_BANK_ACTIVE, RADIO_GAIN_RX_IF, gain);
		endpoint->buffer[0] = RADIO_OK;
		usb_transfer_schedule_block(
			endpoint->in,
			&endpoint->buffer,
			1,
			NULL,
			NULL);
		usb_transfer_schedule_ack(endpoint->out);
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_vga_gain(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		uint8_t gain = endpoint->setup.index;
		radio_reg_write(&radio, RADIO_BANK_ACTIVE, RADIO_GAIN_RX_BB, gain);
		endpoint->buffer[0] = RADIO_OK;
		usb_transfer_schedule_block(
			endpoint->in,
			&endpoint->buffer,
			1,
			NULL,
			NULL);
		usb_transfer_schedule_ack(endpoint->out);
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_txvga_gain(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		uint8_t gain = endpoint->setup.index;
		radio_reg_write(&radio, RADIO_BANK_ACTIVE, RADIO_GAIN_TX_IF, gain);
		endpoint->buffer[0] = RADIO_OK;
		usb_transfer_schedule_block(
			endpoint->in,
			&endpoint->buffer,
			1,
			NULL,
			NULL);
		usb_transfer_schedule_ack(endpoint->out);
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_antenna_enable(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	switch (detected_platform()) {
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
	case BOARD_ID_PRALINE:
		// supported
		break;
	default:
		return USB_REQUEST_STATUS_STALL;
	}

	if (stage == USB_TRANSFER_STAGE_SETUP) {
		radio_reg_write(
			&radio,
			RADIO_BANK_ACTIVE,
			RADIO_BIAS_TEE,
			endpoint->setup.value);
		usb_transfer_schedule_ack(endpoint->in);
	}
	return USB_REQUEST_STATUS_OK;
}

static volatile uint32_t _tx_underrun_limit;
static volatile uint32_t _rx_overrun_limit;

volatile transceiver_request_t transceiver_request = {
	.mode = TRANSCEIVER_MODE_OFF,
	.seq = 0,
};

// Must be called from an atomic context (normally USB ISR)
void request_transceiver_mode(transceiver_mode_t mode)
{
	usb_endpoint_flush(&usb_endpoint_bulk_in);
	usb_endpoint_flush(&usb_endpoint_bulk_out);

	transceiver_request.mode = mode;
	transceiver_request.seq++;
}

void transceiver_shutdown(void)
{
	baseband_streaming_disable(&sgpio_config);
	operacake_sctimer_reset_state();

	usb_endpoint_flush(&usb_endpoint_bulk_in);
	usb_endpoint_flush(&usb_endpoint_bulk_out);

	led_off(LED2);
	led_off(LED3);
	radio_switch_opmode(&radio, TRANSCEIVER_MODE_OFF);
	m0_set_mode(M0_MODE_IDLE);
}

void transceiver_startup(const transceiver_mode_t mode)
{
	dma_started = 0;
	dma_pending = 0;
	usb_started = 0;
	usb_completed = 0;

	radio_switch_opmode(&radio, mode);

	switch (mode) {
	case TRANSCEIVER_MODE_RX_SWEEP:
	case TRANSCEIVER_MODE_RX:
		led_off(LED3);
		led_on(LED2);
		m0_set_mode(M0_MODE_RX);
		m0_state.shortfall_limit = _rx_overrun_limit;
		break;
	case TRANSCEIVER_MODE_TX:
		led_off(LED2);
		led_on(LED3);
		m0_set_mode(M0_MODE_TX_START);
		m0_state.shortfall_limit = _tx_underrun_limit;
		break;
	default:
		break;
	}

	activate_best_clock_source();
}

usb_request_status_t usb_vendor_request_set_transceiver_mode(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		switch (endpoint->setup.value) {
		case TRANSCEIVER_MODE_OFF:
		case TRANSCEIVER_MODE_RX:
		case TRANSCEIVER_MODE_TX:
		case TRANSCEIVER_MODE_RX_SWEEP:
		case TRANSCEIVER_MODE_CPLD_UPDATE:
			request_transceiver_mode(endpoint->setup.value);
			usb_transfer_schedule_ack(endpoint->in);
			return USB_REQUEST_STATUS_OK;
		default:
			return USB_REQUEST_STATUS_STALL;
		}
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_set_hw_sync_mode(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		radio_reg_write(
			&radio,
			RADIO_BANK_ACTIVE,
			RADIO_TRIGGER,
			endpoint->setup.value);
		usb_transfer_schedule_ack(endpoint->in);
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_tx_underrun_limit(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		uint32_t value = (endpoint->setup.index << 16) + endpoint->setup.value;
		_tx_underrun_limit = value;
		usb_transfer_schedule_ack(endpoint->in);
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_rx_overrun_limit(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		uint32_t value = (endpoint->setup.index << 16) + endpoint->setup.value;
		_rx_overrun_limit = value;
		usb_transfer_schedule_ack(endpoint->in);
	}
	return USB_REQUEST_STATUS_OK;
}

void transceiver_start_dma(void* src, void* dest, size_t size)
{
	dma_pending = size;
	memcpy(dest, src, size);
	dma_isr();
}

void dma_isr(void)
{
	m0_state.m4_count += dma_pending;
	dma_pending = 0;
}

void transceiver_bulk_transfer_complete(void* user_data, unsigned int bytes_transferred)
{
	(void) user_data;
	usb_completed += bytes_transferred;
}

typedef enum {
	DIRECTION_RX,
	DIRECTION_TX,
} direction_t;

void start_dma_if_possible(direction_t direction, size_t size)
{
	if (dma_pending) {
		return;
	}

	uint32_t sampling_completed = m0_state.m0_count;
	uint32_t dma_completed = m0_state.m4_count;
	uint32_t samp_offset = dma_started & USB_SAMP_BUFFER_MASK;
	uint32_t bulk_offset = dma_started & USB_BULK_BUFFER_MASK;
	uint32_t data_available, space_in_use, space_available, samp_buf_margin;
	uint8_t *dest, *src;

	if (direction == DIRECTION_RX) {
		data_available = sampling_completed - dma_started;
		space_in_use = usb_completed - dma_completed;
		space_available = USB_BULK_BUFFER_SIZE - space_in_use;
		samp_buf_margin = USB_SAMP_BUFFER_SIZE - data_available;
		src = &usb_samp_buffer[samp_offset];
		dest = &usb_bulk_buffer[bulk_offset];
	} else {
		data_available = usb_completed - dma_started;
		space_in_use = dma_completed - sampling_completed;
		space_available = USB_SAMP_BUFFER_SIZE - space_in_use;
		samp_buf_margin = space_in_use;
		src = &usb_bulk_buffer[bulk_offset];
		dest = &usb_samp_buffer[samp_offset];
	}

	if (data_available < size || size > space_available) {
		return;
	}

	uint32_t m0_buf_half = sampling_completed & BUF_HALF_MASK;
	uint32_t dma_buf_half = dma_started & BUF_HALF_MASK;
	bool same_buf_half = m0_buf_half == dma_buf_half;

	if (same_buf_half && samp_buf_margin >= (USB_SAMP_BUFFER_SIZE / 2)) {
		return;
	}

	transceiver_start_dma(src, dest, size);

	dma_started += size;
}

void start_usb_if_possible(direction_t direction)
{
	uint32_t bulk_offset = usb_started & USB_BULK_BUFFER_MASK;
	uint32_t dma_completed = m0_state.m4_count;
	uint32_t bytes_available;
	usb_endpoint_t* usb_endpoint;

	if (direction == DIRECTION_RX) {
		bytes_available = dma_completed - usb_started;
		usb_endpoint = &usb_endpoint_bulk_in;
	} else {
		uint32_t space_used = usb_started - dma_completed;
		bytes_available = USB_BULK_BUFFER_SIZE - space_used;
		usb_endpoint = &usb_endpoint_bulk_out;
	}

	if (bytes_available < USB_TRANSFER_SIZE) {
		return;
	}

	usb_transfer_schedule_block(
		usb_endpoint,
		&usb_bulk_buffer[bulk_offset],
		USB_TRANSFER_SIZE,
		transceiver_bulk_transfer_complete,
		NULL);

	usb_started += USB_TRANSFER_SIZE;
}

void rx_mode(uint32_t seq)
{
	transceiver_startup(TRANSCEIVER_MODE_RX);

	baseband_streaming_enable(&sgpio_config);

	while (transceiver_request.seq == seq) {
		start_dma_if_possible(DIRECTION_RX, DMA_TRANSFER_SIZE);
		start_usb_if_possible(DIRECTION_RX);
		radio_update(&radio);
	}

	transceiver_shutdown();
}

void tx_mode(uint32_t seq)
{
	transceiver_startup(TRANSCEIVER_MODE_TX);

	// First, make transfers directly into the sample buffer to fill it.
	for (int i = 0; i < (USB_SAMP_BUFFER_SIZE / USB_TRANSFER_SIZE); i++) {
		// Set up transfer.
		usb_transfer_schedule_block(
			&usb_endpoint_bulk_out,
			&usb_samp_buffer[usb_started],
			USB_TRANSFER_SIZE,
			transceiver_bulk_transfer_complete,
			NULL);
		usb_started += USB_TRANSFER_SIZE;

		// Wait for the transfer to complete.
		while (usb_completed < usb_started) {
			// Handle the host switching modes before filling the buffer.
			if (transceiver_request.seq != seq) {
				transceiver_shutdown();
				return;
			}

			radio_update(&radio);
		}
	}

	// Sample buffer is now full. Update DMA counters accordingly.
	dma_started = USB_SAMP_BUFFER_SIZE;
	m0_state.m4_count = USB_SAMP_BUFFER_SIZE;

	// Start transmitting samples.
	baseband_streaming_enable(&sgpio_config);

	// Continue feeding samples to the sample buffer.
	while (transceiver_request.seq == seq) {
		start_dma_if_possible(DIRECTION_TX, DMA_TRANSFER_SIZE);
		start_usb_if_possible(DIRECTION_TX);
		radio_update(&radio);
	}

	transceiver_shutdown();
}

void off_mode(uint32_t seq)
{
	while (transceiver_request.seq == seq) {
		radio_update(&radio);
	}
}
