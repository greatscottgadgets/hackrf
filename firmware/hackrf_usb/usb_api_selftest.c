/*
 * Copyright 2025 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include <stdio.h>
#include <stddef.h>
#include <usb_queue.h>
#include "usb_api_selftest.h"
#include "selftest.h"
#include "platform_detect.h"

static char* itoa(int val, int base)
{
	static char buf[32] = {0};
	int i = 30;
	if (val == 0) {
		buf[0] = '0';
		buf[1] = '\0';
		return &buf[0];
	}
	for (; val && i; --i, val /= base)
		buf[i] = "0123456789abcdef"[val % base];
	return &buf[i + 1];
}

void append(char** dest, size_t* capacity, const char* str)
{
	for (int i = 0;; i++) {
		if (capacity == 0 || str[i] == '\0') {
			return;
		}
		*((*dest)++) = str[i];
		*capacity -= 1;
	}
}

void generate_selftest_report(void)
{
	char* s = &selftest.report.msg[0];
	size_t c = sizeof(selftest.report.msg);
#ifdef RAD1O
	append(&s, &c, "Mixer: MAX2871, ID: ");
	append(&s, &c, itoa(selftest.mixer_id, 10));
	append(&s, &c, "\n");
#else
	append(&s, &c, "Mixer: RFFC5072, ID: ");
	append(&s, &c, itoa(selftest.mixer_id >> 3, 10));
	append(&s, &c, ", Rev: ");
	append(&s, &c, itoa(selftest.mixer_id & 0x7, 10));
	append(&s, &c, "\n");
#endif
	append(&s, &c, "Clock: Si5351");
	append(&s, &c, ", Rev: ");
	append(&s, &c, itoa(selftest.si5351_rev_id, 10));
	append(&s, &c, ", readback: ");
	append(&s, &c, selftest.si5351_readback_ok ? "OK" : "FAIL");
	append(&s, &c, "\n");
#ifdef PRALINE
	append(&s, &c, "Transceiver: MAX2831, RSSI mux test: ");
	append(&s, &c, selftest.max2831_mux_test_ok ? "PASS" : "FAIL");
	append(&s, &c, "\n");
#else
	append(&s, &c, "Transceiver: ");
	append(&s,
	       &c,
	       (detected_platform() == BOARD_ID_HACKRF1_R9 ? "MAX2839" : "MAX2837"));
	append(&s, &c, ", readback success: ");
	append(&s, &c, itoa(selftest.max283x_readback_register_count, 10));
	append(&s, &c, "/");
	append(&s, &c, itoa(selftest.max283x_readback_total_registers, 10));
	if (selftest.max283x_readback_register_count <
	    selftest.max283x_readback_total_registers) {
		append(&s, &c, ", bad value: 0x");
		append(&s, &c, itoa(selftest.max283x_readback_bad_value, 10));
		append(&s, &c, ", expected: 0x");
		append(&s, &c, itoa(selftest.max283x_readback_expected_value, 10));
	}
	append(&s, &c, "\n");
#endif
#ifdef PRALINE
	append(&s, &c, "SGPIO RX test: ");
	append(&s, &c, selftest.sgpio_rx_ok ? "PASS" : "FAIL");
	append(&s, &c, "\n");
	append(&s, &c, "Loopback test: ");
	append(&s, &c, selftest.xcvr_loopback_ok ? "PASS" : "FAIL");
	append(&s, &c, "\n");
#endif
}

usb_request_status_t usb_vendor_request_read_selftest(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		generate_selftest_report();
		usb_transfer_schedule_block(
			endpoint->in,
			&selftest.report,
			sizeof(selftest.report),
			NULL,
			NULL);
		usb_transfer_schedule_ack(endpoint->out);
		return USB_REQUEST_STATUS_OK;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}
