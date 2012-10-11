/*
 * Copyright 2012 Jared Boone
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

#include <string.h>

#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/nvic.h>
#include <libopencm3/lpc43xx/sgpio.h>

#include <hackrf_core.h>
#include <max5864.h>
#include <max2837.h>
#include <rffc5071.h>
#include <sgpio.h>

#include "usb.h"
#include "usb_type.h"
#include "usb_request.h"
#include "usb_descriptor.h"
#include "usb_standard_request.h"

static volatile transceiver_mode_t transceiver_mode = TRANSCEIVER_MODE_TX;

uint8_t* const usb_bulk_buffer = (uint8_t*)0x20004000;
static volatile uint32_t usb_bulk_buffer_offset = 0;
static const uint32_t usb_bulk_buffer_mask = 32768 - 1;

usb_transfer_descriptor_t usb_td_bulk[2] ATTR_ALIGNED(64);
const uint_fast8_t usb_td_bulk_count = sizeof(usb_td_bulk) / sizeof(usb_td_bulk[0]);

static void usb_init_buffers_bulk() {
	usb_td_bulk[0].next_dtd_pointer = USB_TD_NEXT_DTD_POINTER_TERMINATE;
	usb_td_bulk[0].total_bytes
		= USB_TD_DTD_TOKEN_TOTAL_BYTES(16384)
		| USB_TD_DTD_TOKEN_MULTO(0)
		;
	usb_td_bulk[0].buffer_pointer_page[0] = (uint32_t)&usb_bulk_buffer[0x0000];
	usb_td_bulk[0].buffer_pointer_page[1] = (uint32_t)&usb_bulk_buffer[0x1000];
	usb_td_bulk[0].buffer_pointer_page[2] = (uint32_t)&usb_bulk_buffer[0x2000];
	usb_td_bulk[0].buffer_pointer_page[3] = (uint32_t)&usb_bulk_buffer[0x3000];
	usb_td_bulk[0].buffer_pointer_page[4] = (uint32_t)&usb_bulk_buffer[0x4000];

	usb_td_bulk[1].next_dtd_pointer = USB_TD_NEXT_DTD_POINTER_TERMINATE;
	usb_td_bulk[1].total_bytes
		= USB_TD_DTD_TOKEN_TOTAL_BYTES(16384)
		| USB_TD_DTD_TOKEN_MULTO(0)
		;
	usb_td_bulk[1].buffer_pointer_page[0] = (uint32_t)&usb_bulk_buffer[0x4000];
	usb_td_bulk[1].buffer_pointer_page[1] = (uint32_t)&usb_bulk_buffer[0x5000];
	usb_td_bulk[1].buffer_pointer_page[2] = (uint32_t)&usb_bulk_buffer[0x6000];
	usb_td_bulk[1].buffer_pointer_page[3] = (uint32_t)&usb_bulk_buffer[0x7000];
	usb_td_bulk[1].buffer_pointer_page[4] = (uint32_t)&usb_bulk_buffer[0x8000];
}

void usb_endpoint_schedule_no_int(
	const usb_endpoint_t* const endpoint,
	usb_transfer_descriptor_t* const td
) {
	// Ensure that endpoint is ready to be primed.
	// It may have been flushed due to an aborted transaction.
	// TODO: This should be preceded by a flush?
	while( usb_endpoint_is_ready(endpoint) );

	// Configure a transfer.
	td->total_bytes =
		  USB_TD_DTD_TOKEN_TOTAL_BYTES(16384)
		/*| USB_TD_DTD_TOKEN_IOC*/
		| USB_TD_DTD_TOKEN_MULTO(0)
		| USB_TD_DTD_TOKEN_STATUS_ACTIVE
		;
	
	usb_endpoint_prime(endpoint, td);
}

usb_configuration_t usb_configuration_high_speed = {
	.number = 1,
	.speed = USB_SPEED_HIGH,
	.descriptor = usb_descriptor_configuration_high_speed,
};

usb_configuration_t usb_configuration_full_speed = {
	.number = 1,
	.speed = USB_SPEED_FULL,
	.descriptor = usb_descriptor_configuration_full_speed,
};

usb_configuration_t* usb_configurations[] = {
	&usb_configuration_high_speed,
	&usb_configuration_full_speed,
	0,
};

usb_device_t usb_device = {
	.descriptor = usb_descriptor_device,
	.configurations = &usb_configurations,
	.configuration = 0,
};

usb_endpoint_t usb_endpoint_control_out;
usb_endpoint_t usb_endpoint_control_in;

usb_endpoint_t usb_endpoint_control_out = {
	.address = 0x00,
	.device = &usb_device,
	.in = &usb_endpoint_control_in,
	.out = &usb_endpoint_control_out,
	.setup_complete = usb_setup_complete,
	.transfer_complete = usb_control_out_complete,
};

usb_endpoint_t usb_endpoint_control_in = {
	.address = 0x80,
	.device = &usb_device,
	.in = &usb_endpoint_control_in,
	.out = &usb_endpoint_control_out,
	.setup_complete = 0,
	.transfer_complete = usb_control_in_complete,
};

// NOTE: Endpoint number for IN and OUT are different. I wish I had some
// evidence that having BULK IN and OUT on separate endpoint numbers was
// actually a good idea. Seems like everybody does it that way, but why?

usb_endpoint_t usb_endpoint_bulk_in = {
	.address = 0x81,
	.device = &usb_device,
	.in = &usb_endpoint_bulk_in,
	.out = 0,
	.setup_complete = 0,
	.transfer_complete = 0,
};

usb_endpoint_t usb_endpoint_bulk_out = {
	.address = 0x02,
	.device = &usb_device,
	.in = 0,
	.out = &usb_endpoint_bulk_out,
	.setup_complete = 0,
	.transfer_complete = 0,
};

const usb_request_handlers_t usb_request_handlers = {
	.standard = usb_standard_request,
	.class = 0,
	.vendor = 0,
	.reserved = 0,
};

// TODO: Seems like this should live in usb_standard_request.c.
bool usb_set_configuration(
	usb_device_t* const device,
	const uint_fast8_t configuration_number
) {
	const usb_configuration_t* new_configuration = 0;
	if( configuration_number != 0 ) {
		
		// Locate requested configuration.
		if( device->configurations ) {
			usb_configuration_t** configurations = *(device->configurations);
			uint32_t i = 0;
			const usb_speed_t usb_speed_current = usb_speed(device);
			while( configurations[i] ) {
				if( (configurations[i]->speed == usb_speed_current) &&
				    (configurations[i]->number == configuration_number) ) {
					new_configuration = configurations[i];
					break;
				}
				i++;
			}
		}

		// Requested configuration not found: request error.
		if( new_configuration == 0 ) {
			return false;
		}
	}
	
	if( new_configuration != device->configuration ) {
		// Configuration changed.
		device->configuration = new_configuration;

		// TODO: This is lame. There should be a way to link stuff together so
		// that changing the configuration will initialize the related
		// endpoints. No hard-coding crap like this! Then, when there's no more
		// hard-coding, this whole function can move into a shared/reusable
		// library.
		if( device->configuration && (device->configuration->number == 1) ) {
			if( transceiver_mode == TRANSCEIVER_MODE_RX ) {
				sgpio_configure(transceiver_mode, true);
				usb_endpoint_init(&usb_endpoint_bulk_in);
			} else {
				sgpio_configure(transceiver_mode, true);
				usb_endpoint_init(&usb_endpoint_bulk_out);
			}

			usb_init_buffers_bulk();

			nvic_set_priority(NVIC_M4_SGPIO_IRQ, 0);
			nvic_enable_irq(NVIC_M4_SGPIO_IRQ);
			SGPIO_SET_EN_1 = (1 << SGPIO_SLICE_A);

		    sgpio_cpld_stream_enable();
		} else {
			sgpio_cpld_stream_disable();

			nvic_disable_irq(NVIC_M4_SGPIO_IRQ);
			
			usb_endpoint_disable(&usb_endpoint_bulk_in);
			usb_endpoint_disable(&usb_endpoint_bulk_out);
		}

		if( device->configuration ) {
			gpio_set(PORT_LED1_3, PIN_LED1);
		} else {
			gpio_clear(PORT_LED1_3, PIN_LED1);
		}
	}

	return true;
};

void sgpio_irqhandler() {
	SGPIO_CLR_STATUS_1 = 0xFFFFFFFF;

	uint32_t* const p = (uint32_t*)&usb_bulk_buffer[usb_bulk_buffer_offset];
	if( transceiver_mode == TRANSCEIVER_MODE_RX ) {
		p[7] = SGPIO_REG_SS(SGPIO_SLICE_A);
		p[6] = SGPIO_REG_SS(SGPIO_SLICE_I);
		p[5] = SGPIO_REG_SS(SGPIO_SLICE_E);
		p[4] = SGPIO_REG_SS(SGPIO_SLICE_J);
		p[3] = SGPIO_REG_SS(SGPIO_SLICE_C);
		p[2] = SGPIO_REG_SS(SGPIO_SLICE_K);
		p[1] = SGPIO_REG_SS(SGPIO_SLICE_F);
		p[0] = SGPIO_REG_SS(SGPIO_SLICE_L);
	} else {
		SGPIO_REG_SS(SGPIO_SLICE_A) = p[7];
		SGPIO_REG_SS(SGPIO_SLICE_I) = p[6];
		SGPIO_REG_SS(SGPIO_SLICE_E) = p[5];
		SGPIO_REG_SS(SGPIO_SLICE_J) = p[4];
		SGPIO_REG_SS(SGPIO_SLICE_C) = p[3];
		SGPIO_REG_SS(SGPIO_SLICE_K) = p[2];
		SGPIO_REG_SS(SGPIO_SLICE_F) = p[1];
		SGPIO_REG_SS(SGPIO_SLICE_L) = p[0];
	}
	
	usb_bulk_buffer_offset = (usb_bulk_buffer_offset + 32) & usb_bulk_buffer_mask;
}

int main(void) {
	const uint32_t freq = 2441000000U;
	uint8_t switchctrl = 0;

	pin_setup();
	enable_1v8_power();
	cpu_clock_init();

	CGU_BASE_PERIPH_CLK = CGU_BASE_PERIPH_CLK_AUTOBLOCK
			| CGU_BASE_PERIPH_CLK_CLK_SEL(CGU_SRC_PLL1);

	CGU_BASE_APB1_CLK = CGU_BASE_APB1_CLK_AUTOBLOCK
			| CGU_BASE_APB1_CLK_CLK_SEL(CGU_SRC_PLL1);
	
	usb_peripheral_reset();
	
	usb_device_init(0, &usb_device);
	
	usb_endpoint_init(&usb_endpoint_control_out);
	usb_endpoint_init(&usb_endpoint_control_in);
	
	nvic_set_priority(NVIC_M4_USB0_IRQ, 255);

	usb_run(&usb_device);
	
    ssp1_init();
	ssp1_set_mode_max2837();
	max2837_setup();

	rffc5071_setup();
#ifdef JAWBREAKER
	switchctrl = (SWITCHCTRL_AMP_BYPASS | SWITCHCTRL_HP);
#endif
	rffc5071_rx(switchctrl);
	rffc5071_set_frequency(500, 0); // 500 MHz, 0 Hz (Hz ignored)

	max2837_set_frequency(freq);
	max2837_start();
	max2837_rx();
	ssp1_set_mode_max5864();
	max5864_xcvr();

	while(true) {
		// Wait until buffer 0 is transmitted/received.
		while( usb_bulk_buffer_offset < 16384 );

		// Set up IN transfer of buffer 0.
		usb_endpoint_schedule_no_int(&usb_endpoint_bulk_in, &usb_td_bulk[0]);
	
		// Wait until buffer 1 is transmitted/received.
		while( usb_bulk_buffer_offset >= 16384 );

		// Set up IN transfer of buffer 1.
		usb_endpoint_schedule_no_int(&usb_endpoint_bulk_in, &usb_td_bulk[1]);
	}
	
	return 0;
}
