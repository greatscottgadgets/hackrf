/*
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

#include <libopencm3/cm3/vector.h>

#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/m4/nvic.h>

#include <streaming.h>

#include "usb.h"
#include "usb_standard_request.h"

#include "usb_device.h"
#include "usb_endpoint.h"
#include "usb_api_board_info.h"
#include "usb_api_cpld.h"
#include "usb_api_register.h"
#include "usb_api_spiflash.h"

#include "usb_api_transceiver.h"
#include "rf_path.h"
#include "sgpio_isr.h"
#include "usb_bulk_buffer.h"
#include "si5351c.h"
 
static volatile transceiver_mode_t _transceiver_mode = TRANSCEIVER_MODE_OFF;

void set_transceiver_mode(const transceiver_mode_t new_transceiver_mode) {
	baseband_streaming_disable();
	
	usb_endpoint_disable(&usb_endpoint_bulk_in);
	usb_endpoint_disable(&usb_endpoint_bulk_out);
	
	_transceiver_mode = new_transceiver_mode;
	
	if( _transceiver_mode == TRANSCEIVER_MODE_RX ) {
		gpio_clear(PORT_LED1_3, PIN_LED3);
		gpio_set(PORT_LED1_3, PIN_LED2);
		usb_endpoint_init(&usb_endpoint_bulk_in);
		rf_path_set_direction(RF_PATH_DIRECTION_RX);
		vector_table.irq[NVIC_SGPIO_IRQ] = sgpio_isr_rx;
	} else if (_transceiver_mode == TRANSCEIVER_MODE_TX) {
		gpio_clear(PORT_LED1_3, PIN_LED2);
		gpio_set(PORT_LED1_3, PIN_LED3);
		usb_endpoint_init(&usb_endpoint_bulk_out);
		rf_path_set_direction(RF_PATH_DIRECTION_TX);
		vector_table.irq[NVIC_SGPIO_IRQ] = sgpio_isr_tx;
	} else {
		gpio_clear(PORT_LED1_3, PIN_LED2);
		gpio_clear(PORT_LED1_3, PIN_LED3);
		rf_path_set_direction(RF_PATH_DIRECTION_OFF);
		vector_table.irq[NVIC_SGPIO_IRQ] = sgpio_isr_rx;
	}

	if( _transceiver_mode != TRANSCEIVER_MODE_OFF ) {
		si5351c_activate_best_clock_source();
		baseband_streaming_enable();
	}
}

transceiver_mode_t transceiver_mode(void) {
	return _transceiver_mode;
}

usb_request_status_t usb_vendor_request_set_transceiver_mode(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
		switch( endpoint->setup.value ) {
		case TRANSCEIVER_MODE_OFF:
		case TRANSCEIVER_MODE_RX:
		case TRANSCEIVER_MODE_TX:
			set_transceiver_mode(endpoint->setup.value);
			usb_transfer_schedule_ack(endpoint->in);
			return USB_REQUEST_STATUS_OK;
		default:
			return USB_REQUEST_STATUS_STALL;
		}
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

static const usb_request_handler_fn vendor_request_handler[] = {
	NULL,
	usb_vendor_request_set_transceiver_mode,
	usb_vendor_request_write_max2837,
	usb_vendor_request_read_max2837,
	usb_vendor_request_write_si5351c,
	usb_vendor_request_read_si5351c,
	usb_vendor_request_set_sample_rate_frac,
	usb_vendor_request_set_baseband_filter_bandwidth,
	usb_vendor_request_write_rffc5071,
	usb_vendor_request_read_rffc5071,
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
#ifdef HACKRF_ONE
	usb_vendor_request_set_antenna_enable,
#else
	NULL,
#endif
	usb_vendor_request_set_freq_explicit,
};

static const uint32_t vendor_request_handler_count =
	sizeof(vendor_request_handler) / sizeof(vendor_request_handler[0]);

usb_request_status_t usb_vendor_request(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	usb_request_status_t status = USB_REQUEST_STATUS_STALL;
	
	if( endpoint->setup.request < vendor_request_handler_count ) {
		usb_request_handler_fn handler = vendor_request_handler[endpoint->setup.request];
		if( handler ) {
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

void usb_configuration_changed(
	usb_device_t* const device
) {
	/* Reset transceiver to idle state until other commands are received */
	set_transceiver_mode(TRANSCEIVER_MODE_OFF);
	if( device->configuration->number == 1 ) {
		// transceiver configuration
		cpu_clock_pll1_max_speed();
		gpio_set(PORT_LED1_3, PIN_LED1);
	} else if( device->configuration->number == 2 ) {
		// CPLD update configuration
		cpu_clock_pll1_max_speed();
		usb_endpoint_init(&usb_endpoint_bulk_out);
		start_cpld_update = true;
	} else {
		/* Configuration number equal 0 means usb bus reset. */
		cpu_clock_pll1_low_speed();
		gpio_clear(PORT_LED1_3, PIN_LED1);
	}
}

int main(void) {
	pin_setup();
	enable_1v8_power();
#ifdef HACKRF_ONE
	enable_rf_power();
#endif
	cpu_clock_init();

	usb_set_configuration_changed_cb(usb_configuration_changed);
	usb_peripheral_reset();
	
	usb_device_init(0, &usb_device);
	
	usb_queue_init(&usb_endpoint_control_out_queue);
	usb_queue_init(&usb_endpoint_control_in_queue);
	usb_queue_init(&usb_endpoint_bulk_out_queue);
	usb_queue_init(&usb_endpoint_bulk_in_queue);

	usb_endpoint_init(&usb_endpoint_control_out);
	usb_endpoint_init(&usb_endpoint_control_in);
	
	nvic_set_priority(NVIC_USB0_IRQ, 255);

	usb_run(&usb_device);
	
	ssp1_init();

	rf_path_init();

	unsigned int phase = 0;
	while(true) {
		// Check whether we need to initiate a CPLD update
		if (start_cpld_update)
			cpld_update();

		// Set up IN transfer of buffer 0.
		if ( usb_bulk_buffer_offset >= 16384
		     && phase == 1
		     && transceiver_mode() != TRANSCEIVER_MODE_OFF) {
			usb_transfer_schedule_block(
				(transceiver_mode() == TRANSCEIVER_MODE_RX)
				? &usb_endpoint_bulk_in : &usb_endpoint_bulk_out,
				&usb_bulk_buffer[0x0000],
				0x4000,
				NULL, NULL
				);
			phase = 0;
		}
	
		// Set up IN transfer of buffer 1.
		if ( usb_bulk_buffer_offset < 16384
		     && phase == 0
		     && transceiver_mode() != TRANSCEIVER_MODE_OFF) {
			usb_transfer_schedule_block(
				(transceiver_mode() == TRANSCEIVER_MODE_RX)
				? &usb_endpoint_bulk_in : &usb_endpoint_bulk_out,
				&usb_bulk_buffer[0x4000],
				0x4000,
				NULL, NULL
			);
			phase = 1;
		}
	}
	
	return 0;
}
