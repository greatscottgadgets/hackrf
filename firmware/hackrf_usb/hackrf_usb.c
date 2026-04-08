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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/lpc43xx/ipc.h>

#include <clkin.h>
#include <da7219.h>
#include <delay.h>
#include <hackrf_core.h>
#include <hackrf_ui.h>
#include <operacake.h>
#include <platform_detect.h>
#include <radio.h>
#include <rf_path.h>
#include <rom_iap.h>
#include <selftest.h>
#include <usb.h>
#include <usb_queue.h>
#include <usb_request.h>
#include <usb_standard_request.h>
#include <usb_type.h>
#if defined(HACKRF_ONE)
	#include <mixer.h>
#endif
#if defined(PRALINE) || defined(HACKRF_ONE)
	#include <portapack.h>
#endif
#if defined(PRALINE) || defined(HACKRF_ONE) || defined(JAWBREAKER)
	#include <rffc5071.h>
#endif
#if defined(PRALINE)
	#include <fpga.h>
	#if !(defined(DFU_MODE) || defined(RAM_MODE))
		#include <lz4_buf.h>
		#include <spi_bus.h>
		#include <w25q80bv.h>
	#endif
#else
	#include <cpld_jtag.h>
	#include <cpld_xc2c.h>
#endif

#include "usb_api_adc.h"
#include "usb_api_board_info.h"
#include "usb_api_m0_state.h"
#include "usb_api_operacake.h"
#include "usb_api_register.h"
#include "usb_api_selftest.h"
#include "usb_api_spiflash.h"
#include "usb_api_sweep.h"
#include "usb_api_transceiver.h"
#include "usb_api_ui.h"
#include "usb_descriptor.h"
#include "usb_device.h"
#include "usb_endpoint.h"
#if defined(PRALINE)
	#include "usb_api_praline.h"
#else
	#include "usb_api_cpld.h"
#endif

#include <hackrf_usb_protocol.h>

extern uint32_t __m0_start__;
extern uint32_t __m0_end__;
extern uint32_t __ram_m0_start__;
extern uint32_t _etext_ram, _text_ram, _etext_rom;

static usb_request_handler_fn vendor_request_handler[] = {
	[0] = NULL,
	[HACKRF_VENDOR_REQUEST_SET_TRANSCEIVER_MODE] = usb_vendor_request_set_transceiver_mode,
	[HACKRF_VENDOR_REQUEST_MAX283X_WRITE] = usb_vendor_request_write_max283x,
	[HACKRF_VENDOR_REQUEST_MAX283X_READ] = usb_vendor_request_read_max283x,
	[HACKRF_VENDOR_REQUEST_SI5351C_WRITE] = usb_vendor_request_write_si5351c,
	[HACKRF_VENDOR_REQUEST_SI5351C_READ] = usb_vendor_request_read_si5351c,
	[HACKRF_VENDOR_REQUEST_SAMPLE_RATE_SET] = usb_vendor_request_set_sample_rate_frac,
	[HACKRF_VENDOR_REQUEST_BASEBAND_FILTER_BANDWIDTH_SET] = usb_vendor_request_set_baseband_filter_bandwidth,
#ifdef RAD1O
	[HACKRF_VENDOR_REQUEST_RFFC5071_WRITE] = NULL, // write_rffc5071 not used
	[HACKRF_VENDOR_REQUEST_RFFC5071_READ] = NULL, // read_rffc5071 not used
#else
	[HACKRF_VENDOR_REQUEST_RFFC5071_WRITE] = usb_vendor_request_write_rffc5071,
	[HACKRF_VENDOR_REQUEST_RFFC5071_READ] = usb_vendor_request_read_rffc5071,
#endif
	[HACKRF_VENDOR_REQUEST_SPIFLASH_ERASE] = usb_vendor_request_erase_spiflash,
	[HACKRF_VENDOR_REQUEST_SPIFLASH_WRITE] = usb_vendor_request_write_spiflash,
	[HACKRF_VENDOR_REQUEST_SPIFLASH_READ] = usb_vendor_request_read_spiflash,
	[HACKRF_VENDOR_REQUEST_BOARD_ID_READ] = usb_vendor_request_read_board_id,
	[HACKRF_VENDOR_REQUEST_VERSION_STRING_READ] = usb_vendor_request_read_version_string,
	[HACKRF_VENDOR_REQUEST_SET_FREQ] = usb_vendor_request_set_freq,
	[HACKRF_VENDOR_REQUEST_AMP_ENABLE] = usb_vendor_request_set_amp_enable,
	[HACKRF_VENDOR_REQUEST_BOARD_PARTID_SERIALNO_READ] = usb_vendor_request_read_partid_serialno,
	[HACKRF_VENDOR_REQUEST_SET_LNA_GAIN] = usb_vendor_request_set_lna_gain,
	[HACKRF_VENDOR_REQUEST_SET_VGA_GAIN] = usb_vendor_request_set_vga_gain,
	[HACKRF_VENDOR_REQUEST_SET_TXVGA_GAIN] = usb_vendor_request_set_txvga_gain,
#if (defined HACKRF_ONE || defined PRALINE)
	[HACKRF_VENDOR_REQUEST_ANTENNA_ENABLE] = usb_vendor_request_set_antenna_enable,
#endif
	[HACKRF_VENDOR_REQUEST_SET_FREQ_EXPLICIT] = usb_vendor_request_set_freq_explicit,
	[HACKRF_VENDOR_REQUEST_USB_WCID_VENDOR_REQ] = usb_vendor_request_read_wcid,
	[HACKRF_VENDOR_REQUEST_INIT_SWEEP] = usb_vendor_request_init_sweep,
	[HACKRF_VENDOR_REQUEST_OPERACAKE_GET_BOARDS] = usb_vendor_request_operacake_get_boards,
	[HACKRF_VENDOR_REQUEST_OPERACAKE_SET_PORTS] = usb_vendor_request_operacake_set_ports,
	[HACKRF_VENDOR_REQUEST_SET_HW_SYNC_MODE] = usb_vendor_request_set_hw_sync_mode,
	[HACKRF_VENDOR_REQUEST_RESET] = usb_vendor_request_reset,
	[HACKRF_VENDOR_REQUEST_OPERACAKE_SET_RANGES] = usb_vendor_request_operacake_set_ranges,
	[HACKRF_VENDOR_REQUEST_CLKOUT_ENABLE] = usb_vendor_request_set_clkout_enable,
	[HACKRF_VENDOR_REQUEST_SPIFLASH_STATUS] = usb_vendor_request_spiflash_status,
	[HACKRF_VENDOR_REQUEST_SPIFLASH_CLEAR_STATUS] = usb_vendor_request_spiflash_clear_status,
	[HACKRF_VENDOR_REQUEST_OPERACAKE_GPIO_TEST] = usb_vendor_request_operacake_gpio_test,
#ifdef HACKRF_ONE
	[HACKRF_VENDOR_REQUEST_CPLD_CHECKSUM] = usb_vendor_request_cpld_checksum,
#endif
	[HACKRF_VENDOR_REQUEST_UI_ENABLE] = usb_vendor_request_set_ui_enable,
	[HACKRF_VENDOR_REQUEST_OPERACAKE_SET_MODE] = usb_vendor_request_operacake_set_mode,
	[HACKRF_VENDOR_REQUEST_OPERACAKE_GET_MODE] = usb_vendor_request_operacake_get_mode,
	[HACKRF_VENDOR_REQUEST_OPERACAKE_SET_DWELL_TIMES] = usb_vendor_request_operacake_set_dwell_times,
	[HACKRF_VENDOR_REQUEST_GET_M0_STATE] = usb_vendor_request_get_m0_state,
	[HACKRF_VENDOR_REQUEST_SET_TX_UNDERRUN_LIMIT] = usb_vendor_request_set_tx_underrun_limit,
	[HACKRF_VENDOR_REQUEST_SET_RX_OVERRUN_LIMIT] = usb_vendor_request_set_rx_overrun_limit,
	[HACKRF_VENDOR_REQUEST_GET_CLKIN_STATUS] = usb_vendor_request_get_clkin_status,
	[HACKRF_VENDOR_REQUEST_BOARD_REV_READ] = usb_vendor_request_read_board_rev,
	[HACKRF_VENDOR_REQUEST_SUPPORTED_PLATFORM_READ] = usb_vendor_request_read_supported_platform,
	[HACKRF_VENDOR_REQUEST_SET_LEDS] = usb_vendor_request_set_leds,
	[HACKRF_VENDOR_REQUEST_SET_USER_BIAS_T_OPTS] = usb_vendor_request_user_config_set_bias_t_opts,
#ifdef PRALINE
	[HACKRF_VENDOR_REQUEST_FPGA_WRITE_REG] = usb_vendor_request_write_fpga_reg,
	[HACKRF_VENDOR_REQUEST_FPGA_READ_REG] = usb_vendor_request_read_fpga_reg,
	[HACKRF_VENDOR_REQUEST_P2_CTRL] = usb_vendor_request_p2_ctrl,
	[HACKRF_VENDOR_REQUEST_P1_CTRL] = usb_vendor_request_p1_ctrl,
	[HACKRF_VENDOR_REQUEST_SET_NARROWBAND_FILTER] = usb_vendor_request_set_narrowband_filter,
	[HACKRF_VENDOR_REQUEST_SET_FPGA_BITSTREAM] = usb_vendor_request_set_fpga_bitstream,
	[HACKRF_VENDOR_REQUEST_CLKIN_CTRL] = usb_vendor_request_clkin_ctrl,
#endif
	[HACKRF_VENDOR_REQUEST_READ_SELFTEST] = usb_vendor_request_read_selftest,
	[HACKRF_VENDOR_REQUEST_READ_ADC] = usb_vendor_request_adc_read,
	[HACKRF_VENDOR_REQUEST_TEST_RTC_OSC] = usb_vendor_request_test_rtc_osc,
	[HACKRF_VENDOR_REQUEST_RADIO_WRITE_REG] = usb_vendor_request_write_radio_reg,
	[HACKRF_VENDOR_REQUEST_RADIO_READ_REG] = usb_vendor_request_read_radio_reg,
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

static void m0_rom_to_ram(void)
{
	uint32_t* dest = &__ram_m0_start__;

	// Calculate the base address of ROM
	uint32_t base = (uint32_t) (&_etext_rom - (&_etext_ram - &_text_ram));

	// M0 image location, relative to the start of ROM
	uint32_t src = (uint32_t) &__m0_start__;

	uint32_t len = (uint32_t) &__m0_end__ - (uint32_t) src;
	memcpy(dest, (uint32_t*) (base + src), len);
}

#if defined(PRALINE) && !(defined(DFU_MODE) || defined(RAM_MODE))
extern uint32_t _binary_fpga_bin_start;

void fpga_loader_setup(void)
{
	spi_bus_start(spi_flash.bus, &ssp_config_w25q80bv);
	w25q80bv_setup(&spi_flash);
}

void fpga_loader_read(uint32_t addr, uint32_t size, uint8_t* buf)
{
	w25q80bv_read(&spi_flash, addr, size, buf);
}

struct fpga_loader_t fpga_loader = {
	.start_addr = (uint32_t) &_binary_fpga_bin_start,
	.setup = fpga_loader_setup,
	.read = fpga_loader_read,
	.in_buffer = lz4_in_buf,
	.out_buffer = lz4_out_buf,
};
#endif

int main(void)
{
	// Copy M0 image from ROM before SPIFI is disabled
	m0_rom_to_ram();

	// This will be cleared if any self-test check fails.
	selftest.report.pass = true;

	detect_hardware_platform();
	pin_shutdown();
#ifndef RAD1O
	clock_gen_shutdown();
#endif
	delay_us_at_mhz(10000, 96);
	pin_setup();
#ifndef PRALINE
	enable_1v8_power();
	#ifndef RAD1O
	/*
	 * On rad1o, the clock generator power supply comes from the RF supply
	 * which is enabled later. On H1 and Jawbreaker, the clock generator is
	 * on the main 3V3 supply.
	 */
	clock_gen_init();
	#endif
#else
	enable_3v3aux_power();
	#if !defined(DFU_MODE) && !defined(RAM_MODE)
	enable_1v2_power();
	enable_rf_power();
	/*
	 * On Praline, the clock generator power supply comes from 3V3FPGA
	 * which is enabled when 1V2FPGA is turned on.
	 */
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
#ifdef RAD1O
	clock_gen_init();
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
	#if defined(DFU_MODE) || defined(RAM_MODE)
	selftest.fpga_image_load = SKIPPED;
	selftest.report.pass = false;
	#else
	fpga_image_load(&fpga_loader, 0);
	#endif
	delay_us_at_mhz(100, 204);
	fpga_spi_selftest();
	fpga_sgpio_selftest();
#endif
	radio_init(&radio);

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

#ifndef RAD1O
	rffc5071_lock_test(&mixer);
#endif

#ifdef PRALINE
	fpga_if_xcvr_selftest();
#endif

	if (da7219_detect()) {
		operacake_skip_i2c_address(DA7219_ADDRESS);
	}
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
		radio_update(&radio);

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
