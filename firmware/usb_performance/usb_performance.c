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
#include <si5351c.h>
#include <max5864.h>
#include <max2837.h>
#include <rffc5071.h>
#include <w25q80bv.h>
#include <cpld_jtag.h>
#include <sgpio.h>

#include "usb.h"
#include "usb_type.h"
#include "usb_request.h"
#include "usb_descriptor.h"
#include "usb_standard_request.h"

static volatile transceiver_mode_t transceiver_mode = TRANSCEIVER_MODE_RX;

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

void baseband_streaming_disable() {
	sgpio_cpld_stream_disable();

	nvic_disable_irq(NVIC_M4_SGPIO_IRQ);
	
	usb_endpoint_disable(&usb_endpoint_bulk_in);
	usb_endpoint_disable(&usb_endpoint_bulk_out);
}

void set_transceiver_mode(const transceiver_mode_t new_transceiver_mode) {
	baseband_streaming_disable();
	
	transceiver_mode = new_transceiver_mode;
	
	usb_init_buffers_bulk();

	if( transceiver_mode == TRANSCEIVER_MODE_RX ) {
		gpio_clear(PORT_LED1_3, PIN_LED3);
		usb_endpoint_init(&usb_endpoint_bulk_in);

		max2837_rx();
	} else {
		gpio_set(PORT_LED1_3, PIN_LED3);
		usb_endpoint_init(&usb_endpoint_bulk_out);

		max2837_tx();
	}

	sgpio_configure(transceiver_mode, true);

	nvic_set_priority(NVIC_M4_SGPIO_IRQ, 0);
	nvic_enable_irq(NVIC_M4_SGPIO_IRQ);
	SGPIO_SET_EN_1 = (1 << SGPIO_SLICE_A);

    sgpio_cpld_stream_enable();
}

usb_request_status_t usb_vendor_request_set_transceiver_mode(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
		switch( endpoint->setup.value ) {
		case 1:
			set_transceiver_mode(TRANSCEIVER_MODE_RX);
			usb_endpoint_schedule_ack(endpoint->in);
			return USB_REQUEST_STATUS_OK;
			
		case 2:
			set_transceiver_mode(TRANSCEIVER_MODE_TX);
			usb_endpoint_schedule_ack(endpoint->in);
			return USB_REQUEST_STATUS_OK;
		
		default:
			return USB_REQUEST_STATUS_STALL;
		}
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_write_max2837(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
		if( endpoint->setup.index < MAX2837_NUM_REGS ) {
			if( endpoint->setup.value < MAX2837_DATA_REGS_MAX_VALUE ) {
				max2837_reg_write(endpoint->setup.index, endpoint->setup.value);
				usb_endpoint_schedule_ack(endpoint->in);
				return USB_REQUEST_STATUS_OK;
			}
		}
		return USB_REQUEST_STATUS_STALL;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_read_max2837(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
		if( endpoint->setup.index < MAX2837_NUM_REGS ) {
			const uint16_t value = max2837_reg_read(endpoint->setup.index);
			endpoint->buffer[0] = value & 0xff;
			endpoint->buffer[1] = value >> 8;
			usb_endpoint_schedule(endpoint->in, &endpoint->buffer, 2);
			usb_endpoint_schedule_ack(endpoint->out);
			return USB_REQUEST_STATUS_OK;
		}
		return USB_REQUEST_STATUS_STALL;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_write_si5351c(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
		if( endpoint->setup.index < 256 ) {
			if( endpoint->setup.value < 256 ) {
				si5351c_write_single(endpoint->setup.index, endpoint->setup.value);
				usb_endpoint_schedule_ack(endpoint->in);
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
	const usb_transfer_stage_t stage
) {
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
		if( endpoint->setup.index < 256 ) {
			const uint8_t value = si5351c_read_single(endpoint->setup.index);
			endpoint->buffer[0] = value;
			usb_endpoint_schedule(endpoint->in, &endpoint->buffer, 1);
			usb_endpoint_schedule_ack(endpoint->out);
			return USB_REQUEST_STATUS_OK;
		}
		return USB_REQUEST_STATUS_STALL;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_set_sample_rate(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
		const uint32_t sample_rate = (endpoint->setup.index << 16) | endpoint->setup.value;
		if( sample_rate_set(sample_rate) ) {
			usb_endpoint_schedule_ack(endpoint->in);
			return USB_REQUEST_STATUS_OK;
		}
		return USB_REQUEST_STATUS_STALL;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_set_baseband_filter_bandwidth(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
		const uint32_t bandwidth = (endpoint->setup.index << 16) | endpoint->setup.value;
		if( baseband_filter_bandwidth_set(bandwidth) ) {
			usb_endpoint_schedule_ack(endpoint->in);
			return USB_REQUEST_STATUS_OK;
		}
		return USB_REQUEST_STATUS_STALL;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_write_rffc5071(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	if( stage == USB_TRANSFER_STAGE_SETUP ) 
	{
		if( endpoint->setup.index < RFFC5071_NUM_REGS ) 
		{
			rffc5071_reg_write(endpoint->setup.index, endpoint->setup.value);
			usb_endpoint_schedule_ack(endpoint->in);
			return USB_REQUEST_STATUS_OK;
		}
		return USB_REQUEST_STATUS_STALL;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_read_rffc5071(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	uint16_t value;
	if( stage == USB_TRANSFER_STAGE_SETUP ) 
	{
		if( endpoint->setup.index < RFFC5071_NUM_REGS ) 
		{
			value = rffc5071_reg_read(endpoint->setup.index);
			endpoint->buffer[0] = value & 0xff;
			endpoint->buffer[1] = value >> 8;
			usb_endpoint_schedule(endpoint->in, &endpoint->buffer, 2);
			usb_endpoint_schedule_ack(endpoint->out);
			return USB_REQUEST_STATUS_OK;
		}
		return USB_REQUEST_STATUS_STALL;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_write_spiflash(
	usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage)
{
	static uint32_t addr;
	static uint16_t len;
	static uint8_t spiflash_buffer[0xffff];

	if (stage == USB_TRANSFER_STAGE_SETUP) {
		addr = (endpoint->setup.value << 16) | endpoint->setup.index;
		len = endpoint->setup.length;
		if ((len > W25Q80BV_NUM_BYTES) || (addr > W25Q80BV_NUM_BYTES)
				|| ((addr + len) > W25Q80BV_NUM_BYTES)) {
			return USB_REQUEST_STATUS_STALL;
		} else {
			usb_endpoint_schedule(endpoint->out, &spiflash_buffer[0], len);
			return USB_REQUEST_STATUS_OK;
		}
	} else if (stage == USB_TRANSFER_STAGE_DATA) {
			//FIXME still trying to make this work
			w25q80bv_program(addr, len, &spiflash_buffer[0]);
			usb_endpoint_schedule_ack(endpoint->in);
			return USB_REQUEST_STATUS_OK;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_read_spiflash(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	uint32_t addr;
	uint16_t len;
	if( stage == USB_TRANSFER_STAGE_SETUP )
	{
		addr = (endpoint->setup.value << 16) | endpoint->setup.index;
		len = endpoint->setup.length;
		if ((len > W25Q80BV_NUM_BYTES) || (addr > W25Q80BV_NUM_BYTES)
		            || ((addr + len) > W25Q80BV_NUM_BYTES)) {
			return USB_REQUEST_STATUS_STALL;
		} else {
			//FIXME need implementation
			//usb_endpoint_schedule(endpoint->in, &endpoint->buffer, len);
			usb_endpoint_schedule_ack(endpoint->out);
			return USB_REQUEST_STATUS_OK;
		}
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_write_cpld(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	if( stage == USB_TRANSFER_STAGE_SETUP )
	{
		//FIXME endpoint->buffer can't be used like this
		cpld_jtag_program(endpoint->setup.length, endpoint->buffer);
		usb_endpoint_schedule_ack(endpoint->in);
		return USB_REQUEST_STATUS_OK;
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_read_board_id(
		usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		endpoint->buffer[0] = BOARD_ID;
		usb_endpoint_schedule(endpoint->in, &endpoint->buffer, 1);
		usb_endpoint_schedule_ack(endpoint->out);
		return USB_REQUEST_STATUS_OK;
	}
	return USB_REQUEST_STATUS_OK;
}

static const usb_request_handler_fn vendor_request_handler[] = {
	NULL,
	usb_vendor_request_set_transceiver_mode,
	usb_vendor_request_write_max2837,
	usb_vendor_request_read_max2837,
	usb_vendor_request_write_si5351c,
	usb_vendor_request_read_si5351c,
	usb_vendor_request_set_sample_rate,
	usb_vendor_request_set_baseband_filter_bandwidth,
	usb_vendor_request_write_rffc5071,
	usb_vendor_request_read_rffc5071,
	usb_vendor_request_write_spiflash,
	usb_vendor_request_read_spiflash,
	usb_vendor_request_write_cpld,
	usb_vendor_request_read_board_id
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
		set_transceiver_mode(transceiver_mode);

		if( device->configuration ) {
			gpio_set(PORT_LED1_3, PIN_LED1);
		} else {
			gpio_clear(PORT_LED1_3, PIN_LED1);
		}
	}

	return true;
};

void sgpio_irqhandler() {
	SGPIO_CLR_STATUS_1 = (1 << SGPIO_SLICE_A);

	uint32_t* const p = (uint32_t*)&usb_bulk_buffer[usb_bulk_buffer_offset];
	if( transceiver_mode == TRANSCEIVER_MODE_RX ) {
		__asm__(
			"ldr r0, [%[SGPIO_REG_SS], #44]\n\t"
			"str r0, [%[p], #0]\n\t"
			"ldr r0, [%[SGPIO_REG_SS], #20]\n\t"
			"str r0, [%[p], #4]\n\t"
			"ldr r0, [%[SGPIO_REG_SS], #40]\n\t"
			"str r0, [%[p], #8]\n\t"
			"ldr r0, [%[SGPIO_REG_SS], #8]\n\t"
			"str r0, [%[p], #12]\n\t"
			"ldr r0, [%[SGPIO_REG_SS], #36]\n\t"
			"str r0, [%[p], #16]\n\t"
			"ldr r0, [%[SGPIO_REG_SS], #16]\n\t"
			"str r0, [%[p], #20]\n\t"
			"ldr r0, [%[SGPIO_REG_SS], #32]\n\t"
			"str r0, [%[p], #24]\n\t"
			"ldr r0, [%[SGPIO_REG_SS], #0]\n\t"
			"str r0, [%[p], #28]\n\t"
			:
			: [SGPIO_REG_SS] "l" (SGPIO_PORT_BASE + 0x100),
			  [p] "l" (p)
			: "r0"
		);
	} else {
		__asm__(
			"ldr r0, [%[p], #0]\n\t"
			"str r0, [%[SGPIO_REG_SS], #44]\n\t"
			"ldr r0, [%[p], #4]\n\t"
			"str r0, [%[SGPIO_REG_SS], #20]\n\t"
			"ldr r0, [%[p], #8]\n\t"
			"str r0, [%[SGPIO_REG_SS], #40]\n\t"
			"ldr r0, [%[p], #12]\n\t"
			"str r0, [%[SGPIO_REG_SS], #8]\n\t"
			"ldr r0, [%[p], #16]\n\t"
			"str r0, [%[SGPIO_REG_SS], #36]\n\t"
			"ldr r0, [%[p], #20]\n\t"
			"str r0, [%[SGPIO_REG_SS], #16]\n\t"
			"ldr r0, [%[p], #24]\n\t"
			"str r0, [%[SGPIO_REG_SS], #32]\n\t"
			"ldr r0, [%[p], #28]\n\t"
			"str r0, [%[SGPIO_REG_SS], #0]\n\t"
			:
			: [SGPIO_REG_SS] "l" (SGPIO_PORT_BASE + 0x100),
			  [p] "l" (p)
			: "r0"
		);
	}
	
	usb_bulk_buffer_offset = (usb_bulk_buffer_offset + 32) & usb_bulk_buffer_mask;
}

int main(void) {
	const uint32_t ifreq = 2600000000U;
	uint8_t switchctrl = 0;

	pin_setup();
	enable_1v8_power();
	cpu_clock_init();

	usb_peripheral_reset();
	
	usb_device_init(0, &usb_device);
	
	usb_endpoint_init(&usb_endpoint_control_out);
	usb_endpoint_init(&usb_endpoint_control_in);
	
	nvic_set_priority(NVIC_M4_USB0_IRQ, 255);

	usb_run(&usb_device);
	
    ssp1_init();
	ssp1_set_mode_max5864();
	max5864_xcvr();

	ssp1_set_mode_max2837();
	max2837_setup();

	rffc5071_setup();
	
#ifdef JAWBREAKER
	switchctrl = SWITCHCTRL_AMP_BYPASS;
#endif
	rffc5071_rx(switchctrl);
	rffc5071_set_frequency(1700, 0); // 2600 MHz IF - 1700 MHz LO = 900 MHz RF

	max2837_set_frequency(ifreq);
	max2837_start();
	max2837_rx();

	while(true) {
		// Wait until buffer 0 is transmitted/received.
		while( usb_bulk_buffer_offset < 16384 );

		// Set up IN transfer of buffer 0.
		usb_endpoint_schedule_no_int(
			(transceiver_mode == TRANSCEIVER_MODE_RX)
			? &usb_endpoint_bulk_in : &usb_endpoint_bulk_out,
			&usb_td_bulk[0]
		);
	
		// Wait until buffer 1 is transmitted/received.
		while( usb_bulk_buffer_offset >= 16384 );

		// Set up IN transfer of buffer 1.
		usb_endpoint_schedule_no_int(
			(transceiver_mode == TRANSCEIVER_MODE_RX)
			? &usb_endpoint_bulk_in : &usb_endpoint_bulk_out,
			&usb_td_bulk[1]
		);
	}
	
	return 0;
}
