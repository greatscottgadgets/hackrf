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

#include <stddef.h>
#include <string.h>

#include <libopencm3/lpc43xx/ipc.h>
#include <libopencm3/lpc43xx/m4/nvic.h>
#include <libopencm3/lpc43xx/rgu.h>
#include <libopencm3/lpc43xx/timer.h>

#include <streaming.h>

#include "tuning.h"

#include "usb.h"
#include "usb_standard_request.h"

#include <rom_iap.h>
#include "usb_descriptor.h"

#include "usb_device.h"
#include "usb_endpoint.h"
#include "usb_api_board_info.h"
#include "usb_api_cpld.h"
#include "usb_api_register.h"
#include "usb_api_spiflash.h"
#include "usb_api_operacake.h"
#include "usb_api_praline.h"
#include "usb_api_selftest.h"
#include "usb_api_adc.h"
#include "usb_api_radio.h"
#include "operacake.h"
#include "usb_api_sweep.h"
#include "usb_api_transceiver.h"
#include "usb_api_ui.h"
#include "usb_bulk_buffer.h"
#include "usb_api_m0_state.h"
#include "cpld_xc2c.h"
#include "portapack.h"
#include "hackrf_ui.h"
#include "platform_detect.h"
#include "clkin.h"
#include "fpga.h"
#include "selftest.h"

extern uint32_t __m0_start__;
extern uint32_t __m0_end__;
extern uint32_t __ram_m0_start__;
extern uint32_t _etext_ram, _text_ram, _etext_rom;

static usb_request_handler_fn vendor_request_handler[] = {
	NULL,
	usb_vendor_request_set_transceiver_mode,
	usb_vendor_request_write_max283x,
	usb_vendor_request_read_max283x,
	usb_vendor_request_write_si5351c,
	usb_vendor_request_read_si5351c,
	usb_vendor_request_set_sample_rate_frac,
	usb_vendor_request_set_baseband_filter_bandwidth,
#ifdef RAD1O
	NULL, // write_rffc5071 not used
	NULL, // read_rffc5071 not used
#else
	usb_vendor_request_write_rffc5071,
	usb_vendor_request_read_rffc5071,
#endif
	usb_vendor_request_erase_spiflash,
	usb_vendor_request_write_spiflash,
	usb_vendor_request_read_spiflash,
	NULL, // used to be write_cpld
	usb_vendor_request_read_board_id,
	usb_vendor_request_read_version_string,
	usb_vendor_request_set_freq,
	usb_vendor_request_set_amp_enable,
	usb_vendor_request_read_partid_serialno,
	usb_vendor_request_set_lna_gain,
	usb_vendor_request_set_vga_gain,
	usb_vendor_request_set_txvga_gain,
	NULL, // was set_if_freq
#if (defined HACKRF_ONE || defined PRALINE)
	usb_vendor_request_set_antenna_enable,
#else
	NULL,
#endif
	usb_vendor_request_set_freq_explicit,
	usb_vendor_request_read_wcid, // USB_WCID_VENDOR_REQ
	usb_vendor_request_init_sweep,
	usb_vendor_request_operacake_get_boards,
	usb_vendor_request_operacake_set_ports,
	usb_vendor_request_set_hw_sync_mode,
	usb_vendor_request_reset,
	usb_vendor_request_operacake_set_ranges,
	usb_vendor_request_set_clkout_enable,
	usb_vendor_request_spiflash_status,
	usb_vendor_request_spiflash_clear_status,
	usb_vendor_request_operacake_gpio_test,
#ifdef HACKRF_ONE
	usb_vendor_request_cpld_checksum,
#else
	NULL,
#endif
	usb_vendor_request_set_ui_enable,
	usb_vendor_request_operacake_set_mode,
	usb_vendor_request_operacake_get_mode,
	usb_vendor_request_operacake_set_dwell_times,
	usb_vendor_request_get_m0_state,
	usb_vendor_request_set_tx_underrun_limit,
	usb_vendor_request_set_rx_overrun_limit,
	usb_vendor_request_get_clkin_status,
	usb_vendor_request_read_board_rev,
	usb_vendor_request_read_supported_platform,
	usb_vendor_request_set_leds,
	usb_vendor_request_user_config_set_bias_t_opts,
#ifdef PRALINE
	usb_vendor_request_write_fpga_reg,
	usb_vendor_request_read_fpga_reg,
	usb_vendor_request_p2_ctrl,
	usb_vendor_request_p1_ctrl,
	usb_vendor_request_set_narrowband_filter,
	usb_vendor_request_set_fpga_bitstream,
	usb_vendor_request_clkin_ctrl,
#else
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
#endif
	usb_vendor_request_read_selftest,
	usb_vendor_request_adc_read,
	usb_vendor_request_test_rtc_osc,
	usb_vendor_request_set_mode_frequency,
	usb_vendor_request_set_mode_sample_rate,
	usb_vendor_request_supported_sample_rate,
	usb_vendor_request_get_sample_rate_element,
	usb_vendor_request_get_filter_element,
	usb_vendor_request_supported_filter_element_bandwidths,
	usb_vendor_request_get_frequency_element,
	usb_vendor_request_get_gain_element,
	usb_vendor_request_get_antenna_element,
};

static const uint32_t vendor_request_handler_count =
	sizeof(vendor_request_handler) / sizeof(vendor_request_handler[0]);

usb_request_status_t usb_vendor_request(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	usb_request_status_t status = USB_REQUEST_STATUS_STALL;

	if (endpoint->setup.request < vendor_request_handler_count) {
		usb_request_handler_fn handler =
			vendor_request_handler[endpoint->setup.request];
		if (handler) {
			status = handler(endpoint, stage);
		}
	}

	return status;
}

const usb_request_handlers_t usb_request_handlers = {
	.standard = usb_standard_request,
	.class = 0,
	.vendor = usb_vendor_request,
	.reserved = 0,
};

void usb_configuration_changed(usb_device_t* const device)
{
	/* Reset transceiver to idle state until other commands are received */
	request_transceiver_mode(TRANSCEIVER_MODE_OFF);
	if (device->configuration->number == 1) {
		// transceiver configuration
		led_on(LED1);
	} else {
		/* Configuration number equal 0 means usb bus reset. */
		led_off(LED1);
	}
	usb_endpoint_init(&usb_endpoint_bulk_in, false);
	usb_endpoint_init(&usb_endpoint_bulk_out, false);
}

void usb_set_descriptor_by_serial_number(void)
{
	iap_cmd_res_t iap_cmd_res;

	/* Read IAP Serial Number Identification */
	iap_cmd_res.cmd_param.command_code = IAP_CMD_READ_SERIAL_NO;
	iap_cmd_call(&iap_cmd_res);

	if (iap_cmd_res.status_res.status_ret == CMD_SUCCESS) {
		usb_descriptor_string_serial_number[0] =
			USB_DESCRIPTOR_STRING_SERIAL_BUF_LEN;
		usb_descriptor_string_serial_number[1] = USB_DESCRIPTOR_TYPE_STRING;

		/* 32 characters of serial number, convert to UTF-16LE */
		for (size_t i = 0; i < USB_DESCRIPTOR_STRING_SERIAL_LEN; i++) {
			const uint_fast8_t nibble =
				(iap_cmd_res.status_res.iap_result[i >> 3] >>
				 (28 - (i & 7) * 4)) &
				0xf;
			const char c =
				(nibble > 9) ? ('a' + nibble - 10) : ('0' + nibble);
			usb_descriptor_string_serial_number[2 + i * 2] = c;
			usb_descriptor_string_serial_number[3 + i * 2] = 0x00;
		}
	} else {
		usb_descriptor_string_serial_number[0] = 2;
		usb_descriptor_string_serial_number[1] = USB_DESCRIPTOR_TYPE_STRING;
	}
}

#ifndef PRALINE
static bool cpld_jtag_sram_load(jtag_t* const jtag)
{
	cpld_jtag_take(jtag);
	cpld_xc2c64a_jtag_sram_write(jtag, &cpld_hackrf_program_sram);
	const bool success = cpld_xc2c64a_jtag_sram_verify(
		jtag,
		&cpld_hackrf_program_sram,
		&cpld_hackrf_verify);
	cpld_jtag_release(jtag);
	return success;
}
#endif

static void m0_rom_to_ram()
{
	uint32_t* dest = &__ram_m0_start__;

	// Calculate the base address of ROM
	uint32_t base = (uint32_t) (&_etext_rom - (&_etext_ram - &_text_ram));

	// M0 image location, relative to the start of ROM
	uint32_t src = (uint32_t) &__m0_start__;

	uint32_t len = (uint32_t) &__m0_end__ - (uint32_t) src;
	memcpy(dest, (uint32_t*) (base + src), len);
}

int main(void)
{
	// Copy M0 image from ROM before SPIFI is disabled
	m0_rom_to_ram();

	// This will be cleared if any self-test check fails.
	selftest.report.pass = true;

	detect_hardware_platform();
	pin_shutdown();
	clock_gen_shutdown();
	delay_us_at_mhz(10000, 96);
	pin_setup();
#ifndef PRALINE
	enable_1v8_power();
	clock_gen_init();
#else
	enable_3v3aux_power();
	#if !defined(DFU_MODE) && !defined(RAM_MODE)
	enable_1v2_power();
	enable_rf_power();
	clock_gen_init();
	#endif
#endif
#ifdef HACKRF_ONE
	// Set up mixer before enabling RF power, because its
	// GPO is used to control the antenna bias tee.
	mixer_setup(&mixer);
#endif
#if (defined HACKRF_ONE || defined RAD1O)
	enable_rf_power();
#endif
	cpu_clock_init();

	/* Wake the M0 */
	ipc_halt_m0();
	ipc_start_m0((uint32_t) &__ram_m0_start__);

#ifndef PRALINE
	if (!cpld_jtag_sram_load(&jtag_cpld)) {
		halt_and_flash(6000000);
	}
#else
	fpga_image_load(0);
	delay_us_at_mhz(100, 204);
	fpga_spi_selftest();
	fpga_sgpio_selftest();
#endif

#if (defined HACKRF_ONE || defined PRALINE)
	portapack_init();
#endif

#ifndef DFU_MODE
	usb_set_descriptor_by_serial_number();
#endif

	usb_set_configuration_changed_cb(usb_configuration_changed);
	usb_peripheral_reset();

	usb_device_init(0, &usb_device);

	usb_queue_init(&usb_endpoint_control_out_queue);
	usb_queue_init(&usb_endpoint_control_in_queue);
	usb_queue_init(&usb_endpoint_bulk_out_queue);
	usb_queue_init(&usb_endpoint_bulk_in_queue);

	usb_endpoint_init(&usb_endpoint_control_out, false);
	usb_endpoint_init(&usb_endpoint_control_in, true);

	nvic_set_priority(NVIC_USB0_IRQ, 255);

	hackrf_ui()->init();

	usb_run(&usb_device);

	rf_path_init(&rf_path);

#ifdef PRALINE
	fpga_if_xcvr_selftest();
#endif

	bool operacake_allow_gpio;
	if (hackrf_ui()->operacake_gpio_compatible()) {
		operacake_allow_gpio = true;
	} else {
		operacake_allow_gpio = false;
	}
	operacake_init(operacake_allow_gpio);

	// FIXME: clock detection on r9 only works when calling init twice
	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
		clkin_detect_init();
		clkin_detect_init();
	}

	while (true) {
		transceiver_request_t request;

		// Briefly disable USB interrupt so that we can
		// atomically retrieve both the transceiver mode
		// and the mode change sequence number. They are
		// changed together by request_transceiver_mode()
		// called from the USB ISR.

		nvic_disable_irq(NVIC_USB0_IRQ);
		request = transceiver_request;
		nvic_enable_irq(NVIC_USB0_IRQ);

		switch (request.mode) {
		case TRANSCEIVER_MODE_OFF:
			off_mode(request.seq);
			break;
		case TRANSCEIVER_MODE_RX:
			rx_mode(request.seq);
			break;
		case TRANSCEIVER_MODE_TX:
			tx_mode(request.seq);
			break;
		case TRANSCEIVER_MODE_RX_SWEEP:
			sweep_mode(request.seq);
			break;
#ifndef PRALINE
		case TRANSCEIVER_MODE_CPLD_UPDATE:
			cpld_update();
			break;
#endif
		default:
			break;
		}
	}

	return 0;
}
