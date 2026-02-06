/*
 * Copyright 2012-2026 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include "usb_api_register.h"
#include <user_config.h>
#include <hackrf_core.h>
#include <usb_queue.h>
#include <max2831.h>
#include <max283x.h>
#include <rffc5071.h>
#include <ice40_spi.h>

#include <stddef.h>
#include <stdint.h>

#include <hackrf_core.h>

usb_request_status_t usb_vendor_request_write_max283x(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
#ifndef PRALINE
		if (endpoint->setup.index < MAX2837_NUM_REGS) {
			if (endpoint->setup.value < MAX2837_DATA_REGS_MAX_VALUE) {
				max283x_reg_write(
					&max283x,
					endpoint->setup.index,
					endpoint->setup.value);
				usb_transfer_schedule_ack(endpoint->in);
				return USB_REQUEST_STATUS_OK;
			}
		}
#else
		if (endpoint->setup.index < MAX2831_NUM_REGS) {
			if (endpoint->setup.value < MAX2831_DATA_REGS_MAX_VALUE) {
				max2831_reg_write(
					&max283x,
					endpoint->setup.index,
					endpoint->setup.value);
				usb_transfer_schedule_ack(endpoint->in);
				return USB_REQUEST_STATUS_OK;
			}
		}
#endif
		return USB_REQUEST_STATUS_STALL;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_read_max283x(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
#ifndef PRALINE
		if (endpoint->setup.index < MAX2837_NUM_REGS) {
			const uint16_t value =
				max283x_reg_read(&max283x, endpoint->setup.index);
			endpoint->buffer[0] = value & 0xff;
			endpoint->buffer[1] = value >> 8;
			usb_transfer_schedule_block(
				endpoint->in,
				&endpoint->buffer,
				2,
				NULL,
				NULL);
			usb_transfer_schedule_ack(endpoint->out);
			return USB_REQUEST_STATUS_OK;
		}
#else
		if (endpoint->setup.index < MAX2831_NUM_REGS) {
			const uint16_t value =
				max2831_reg_read(&max283x, endpoint->setup.index);
			endpoint->buffer[0] = value & 0xff;
			endpoint->buffer[1] = value >> 8;
			usb_transfer_schedule_block(
				endpoint->in,
				&endpoint->buffer,
				2,
				NULL,
				NULL);
			usb_transfer_schedule_ack(endpoint->out);
			return USB_REQUEST_STATUS_OK;
		}
#endif
		return USB_REQUEST_STATUS_STALL;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_write_si5351c(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		if (endpoint->setup.index < 256) {
			if (endpoint->setup.value < 256) {
				si5351c_write_single(
					&clock_gen,
					endpoint->setup.index,
					endpoint->setup.value);
				usb_transfer_schedule_ack(endpoint->in);
				return USB_REQUEST_STATUS_OK;
			}
		}
		return USB_REQUEST_STATUS_STALL;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_read_si5351c(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		if (endpoint->setup.index < 256) {
			const uint8_t value =
				si5351c_read_single(&clock_gen, endpoint->setup.index);
			endpoint->buffer[0] = value;
			usb_transfer_schedule_block(
				endpoint->in,
				&endpoint->buffer,
				1,
				NULL,
				NULL);
			usb_transfer_schedule_ack(endpoint->out);
			return USB_REQUEST_STATUS_OK;
		}
		return USB_REQUEST_STATUS_STALL;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

#ifndef RAD1O
usb_request_status_t usb_vendor_request_write_rffc5071(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		if (endpoint->setup.index < RFFC5071_NUM_REGS) {
			rffc5071_reg_write(
				&mixer,
				endpoint->setup.index,
				endpoint->setup.value);
			usb_transfer_schedule_ack(endpoint->in);
			return USB_REQUEST_STATUS_OK;
		}
		return USB_REQUEST_STATUS_STALL;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_read_rffc5071(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	uint16_t value;
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		if (endpoint->setup.index < RFFC5071_NUM_REGS) {
			value = rffc5071_reg_read(&mixer, endpoint->setup.index);
			endpoint->buffer[0] = value & 0xff;
			endpoint->buffer[1] = value >> 8;
			usb_transfer_schedule_block(
				endpoint->in,
				&endpoint->buffer,
				2,
				NULL,
				NULL);
			usb_transfer_schedule_ack(endpoint->out);
			return USB_REQUEST_STATUS_OK;
		}
		return USB_REQUEST_STATUS_STALL;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}
#endif

usb_request_status_t usb_vendor_request_set_clkout_enable(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		si5351c_clkout_enable(&clock_gen, endpoint->setup.value);
		usb_transfer_schedule_ack(endpoint->in);
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_get_clkin_status(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		endpoint->buffer[0] = si5351c_clkin_signal_valid(&clock_gen);
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

usb_request_status_t usb_vendor_request_set_leds(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		set_leds(endpoint->setup.value);
		usb_transfer_schedule_ack(endpoint->in);
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_user_config_set_bias_t_opts(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		user_config_set_bias_t_opts(endpoint->setup.value);
		usb_transfer_schedule_ack(endpoint->in);
	}
	return USB_REQUEST_STATUS_OK;
}

#ifdef PRALINE
usb_request_status_t usb_vendor_request_write_fpga_reg(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		fpga_reg_write(&fpga, endpoint->setup.index, endpoint->setup.value);
		usb_transfer_schedule_ack(endpoint->in);
		return USB_REQUEST_STATUS_OK;
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_read_fpga_reg(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		const uint8_t value = fpga_reg_read(&fpga, endpoint->setup.index);
		endpoint->buffer[0] = value;
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
#endif

/*
 * Each register is transferred as a uint8_t register number followed by a
 * little-endian uint64_t value for a total of 9 bytes.
 */
static uint8_t radio_reg_buf[RADIO_NUM_REGS * 9];

usb_request_status_t usb_vendor_request_write_radio_reg(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	uint8_t bank;
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		bank = endpoint->setup.index;
		if (bank >= RADIO_NUM_BANKS) {
			return USB_REQUEST_STATUS_STALL;
		}
		uint8_t num_regs = endpoint->setup.length / 9;
		if ((num_regs == 0) || (num_regs > RADIO_NUM_REGS)) {
			return USB_REQUEST_STATUS_STALL;
		}
		usb_transfer_schedule_block(
			endpoint->out,
			&radio_reg_buf,
			endpoint->setup.length,
			NULL,
			NULL);
	} else if (stage == USB_TRANSFER_STAGE_DATA) {
		uint8_t address, i;
		uint64_t value;
		bank = endpoint->setup.index;
		for (i = 0; i < endpoint->setup.length; i += 9) {
			address = radio_reg_buf[i];
			value = radio_reg_buf[i + 1] |
				((uint64_t) radio_reg_buf[i + 2] << 8) |
				((uint64_t) radio_reg_buf[i + 3] << 16) |
				((uint64_t) radio_reg_buf[i + 4] << 24) |
				((uint64_t) radio_reg_buf[i + 5] << 32) |
				((uint64_t) radio_reg_buf[i + 6] << 40) |
				((uint64_t) radio_reg_buf[i + 7] << 48) |
				((uint64_t) radio_reg_buf[i + 8] << 56);
			radio_error_t result =
				radio_reg_write(&radio, bank, address, value);
			if (result != RADIO_OK) {
				return USB_REQUEST_STATUS_STALL;
			}
		}
		usb_transfer_schedule_ack(endpoint->in);
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_read_radio_reg(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		uint8_t bank = endpoint->setup.index;
		uint8_t address = endpoint->setup.value;
		if ((bank >= RADIO_NUM_BANKS) || (address >= RADIO_NUM_REGS)) {
			return USB_REQUEST_STATUS_STALL;
		}
		uint64_t value = radio_reg_read(&radio, bank, address);
		endpoint->buffer[0] = value & 0xff;
		endpoint->buffer[1] = (value >> 8) & 0xff;
		endpoint->buffer[2] = (value >> 16) & 0xff;
		endpoint->buffer[3] = (value >> 24) & 0xff;
		endpoint->buffer[4] = (value >> 32) & 0xff;
		endpoint->buffer[5] = (value >> 40) & 0xff;
		endpoint->buffer[6] = (value >> 48) & 0xff;
		endpoint->buffer[7] = (value >> 56) & 0xff;
		usb_transfer_schedule_block(
			endpoint->in,
			&endpoint->buffer,
			8,
			NULL,
			NULL);
		usb_transfer_schedule_ack(endpoint->out);
	}
	return USB_REQUEST_STATUS_OK;
}
