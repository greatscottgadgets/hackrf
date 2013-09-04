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

#include <string.h>

#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/m4/nvic.h>
#include <libopencm3/lpc43xx/sgpio.h>

#include <hackrf_core.h>
#include <si5351c.h>
#include <max5864.h>
#include <max2837.h>
#include <rffc5071.h>
#include <w25q80bv.h>
#include <cpld_jtag.h>
#include <sgpio.h>
#include <rom_iap.h>

#include "usb.h"
#include "usb_type.h"
#include "usb_request.h"
#include "usb_descriptor.h"
#include "usb_standard_request.h"

static volatile transceiver_mode_t transceiver_mode = TRANSCEIVER_MODE_OFF;

uint8_t* const usb_bulk_buffer = (uint8_t*)0x20004000;
static volatile uint32_t usb_bulk_buffer_offset = 0;
static const uint32_t usb_bulk_buffer_mask = 32768 - 1;

usb_transfer_descriptor_t usb_td_bulk[2] ATTR_ALIGNED(64);
const uint_fast8_t usb_td_bulk_count = sizeof(usb_td_bulk) / sizeof(usb_td_bulk[0]);
 
/* TODO remove this big buffer and use streaming for CPLD */ 
#define CPLD_XSVF_MAX_LEN (65536)
uint8_t cpld_xsvf_buffer[CPLD_XSVF_MAX_LEN];
uint16_t write_cpld_idx = 0;

uint8_t spiflash_buffer[W25Q80BV_PAGE_LEN];
char version_string[] = VERSION_STRING;

typedef struct {
	uint32_t freq_mhz;
	uint32_t freq_hz;
} set_freq_params_t;

set_freq_params_t set_freq_params;

typedef struct {
	uint32_t freq_hz;
	uint32_t divider;
} set_sample_r_params_t;

set_sample_r_params_t set_sample_r_params;

uint8_t switchctrl = 0;

void update_switches(void)
{
	if (transceiver_mode == TRANSCEIVER_MODE_RX) {
		rffc5071_rx(switchctrl);
	} else if (transceiver_mode == TRANSCEIVER_MODE_TX) {
		rffc5071_tx(switchctrl);
	}
}

#define FREQ_ONE_MHZ     (1000*1000)

#define MIN_LP_FREQ_MHZ (5)
#define MAX_LP_FREQ_MHZ (2300)

#define MIN_BYPASS_FREQ_MHZ (2300)
#define MAX_BYPASS_FREQ_MHZ (2700)

#define MIN_HP_FREQ_MHZ (2700)
#define MAX_HP_FREQ_MHZ (6800)

static uint32_t MAX2837_FREQ_NOMINAL_HZ=2600000000;
#define MAX2837_FREQ_NOMINAL_MHZ (MAX2837_FREQ_NOMINAL_HZ / FREQ_ONE_MHZ)

uint32_t freq_mhz_cache=100, freq_hz_cache=0;
/*
 * Set freq/tuning between 5MHz to 6800 MHz (less than 16bits really used)
 * hz between 0 to 999999 Hz (not checked)
 * return false on error or true if success.
 */
bool set_freq(uint32_t freq_mhz, uint32_t freq_hz)
{
	bool success;
	uint32_t RFFC5071_freq_mhz;
	uint32_t MAX2837_freq_hz;
	uint32_t real_RFFC5071_freq_hz;
	uint32_t tmp_hz;

	success = true;

	gpio_clear(PORT_XCVR_ENABLE, (PIN_XCVR_RXENABLE | PIN_XCVR_TXENABLE));
	if(freq_mhz >= MIN_LP_FREQ_MHZ)
	{
		if(freq_mhz < MAX_LP_FREQ_MHZ)
		{
			switchctrl &= ~(SWITCHCTRL_HP | SWITCHCTRL_MIX_BYPASS);

			RFFC5071_freq_mhz = MAX2837_FREQ_NOMINAL_MHZ - freq_mhz;
			/* Set Freq and read real freq */
			real_RFFC5071_freq_hz = rffc5071_set_frequency(RFFC5071_freq_mhz);
			if(real_RFFC5071_freq_hz < RFFC5071_freq_mhz * FREQ_ONE_MHZ)
			{
				tmp_hz = -(RFFC5071_freq_mhz  * FREQ_ONE_MHZ - real_RFFC5071_freq_hz);
			}else
			{
				tmp_hz = (real_RFFC5071_freq_hz - RFFC5071_freq_mhz  * FREQ_ONE_MHZ);
			}
			MAX2837_freq_hz = MAX2837_FREQ_NOMINAL_HZ + tmp_hz + freq_hz;
			max2837_set_frequency(MAX2837_freq_hz);
			update_switches();
		}else if( (freq_mhz >= MIN_BYPASS_FREQ_MHZ) && (freq_mhz < MAX_BYPASS_FREQ_MHZ) )
		{
			switchctrl |= SWITCHCTRL_MIX_BYPASS;

			MAX2837_freq_hz = (freq_mhz * FREQ_ONE_MHZ) + freq_hz;
			/* RFFC5071_freq_mhz <= not used in Bypass mode */
			max2837_set_frequency(MAX2837_freq_hz);
			update_switches();
		}else if(  (freq_mhz >= MIN_HP_FREQ_MHZ) && (freq_mhz < MAX_HP_FREQ_MHZ) )
		{
			switchctrl &= ~SWITCHCTRL_MIX_BYPASS;
			switchctrl |= SWITCHCTRL_HP;

			RFFC5071_freq_mhz = freq_mhz - MAX2837_FREQ_NOMINAL_MHZ;
			/* Set Freq and read real freq */
			real_RFFC5071_freq_hz = rffc5071_set_frequency(RFFC5071_freq_mhz);
			if(real_RFFC5071_freq_hz < RFFC5071_freq_mhz * FREQ_ONE_MHZ)
			{
				tmp_hz = (RFFC5071_freq_mhz * FREQ_ONE_MHZ - real_RFFC5071_freq_hz);
			}else
			{
				tmp_hz = -(real_RFFC5071_freq_hz - RFFC5071_freq_mhz * FREQ_ONE_MHZ);
			}
			MAX2837_freq_hz = MAX2837_FREQ_NOMINAL_HZ + tmp_hz + freq_hz;
			max2837_set_frequency(MAX2837_freq_hz);
			update_switches();
		}else
		{
			/* Error freq_mhz too high */
			success = false;
		}
	}else
	{
		/* Error freq_mhz too low */
		success = false;
	}
	if(transceiver_mode == TRANSCEIVER_MODE_RX)
		gpio_set(PORT_XCVR_ENABLE, PIN_XCVR_RXENABLE);
	else if(transceiver_mode == TRANSCEIVER_MODE_TX)
		gpio_set(PORT_XCVR_ENABLE, PIN_XCVR_TXENABLE);
	
	freq_mhz_cache = freq_mhz;
	freq_hz_cache = freq_hz;
	return success;
}

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

	nvic_disable_irq(NVIC_SGPIO_IRQ);
	
	usb_endpoint_disable(&usb_endpoint_bulk_in);
	usb_endpoint_disable(&usb_endpoint_bulk_out);
}

void set_transceiver_mode(const transceiver_mode_t new_transceiver_mode) {
	baseband_streaming_disable();
	
	transceiver_mode = new_transceiver_mode;
	
	usb_init_buffers_bulk();

	if( transceiver_mode == TRANSCEIVER_MODE_RX ) {
		gpio_clear(PORT_LED1_3, PIN_LED3);
		gpio_set(PORT_LED1_3, PIN_LED2);
		usb_endpoint_init(&usb_endpoint_bulk_in);

		rffc5071_rx(switchctrl);
		//rffc5071_set_frequency(1700, 0); // 2600 MHz IF - 1700 MHz LO = 900 MHz RF
		max2837_start();
		max2837_rx();
	} else if (transceiver_mode == TRANSCEIVER_MODE_TX) {
		gpio_clear(PORT_LED1_3, PIN_LED2);
		gpio_set(PORT_LED1_3, PIN_LED3);
		usb_endpoint_init(&usb_endpoint_bulk_out);

		rffc5071_tx(switchctrl);
		//rffc5071_set_frequency(1700, 0); // 2600 MHz IF - 1700 MHz LO = 900 MHz RF
		max2837_start();
		max2837_tx();
	} else {
		gpio_clear(PORT_LED1_3, PIN_LED2);
		gpio_clear(PORT_LED1_3, PIN_LED3);
		max2837_stop();
		return;
	}

	sgpio_configure(transceiver_mode, true);

	nvic_set_priority(NVIC_SGPIO_IRQ, 0);
	nvic_enable_irq(NVIC_SGPIO_IRQ);
	SGPIO_SET_EN_1 = (1 << SGPIO_SLICE_A);

    sgpio_cpld_stream_enable();
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

usb_request_status_t usb_vendor_request_erase_spiflash(
		usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage)
{
	//FIXME This should refuse to run if executing from SPI flash.

	if (stage == USB_TRANSFER_STAGE_SETUP) {
		w25q80bv_setup();
		/* only chip erase is implemented */
		w25q80bv_chip_erase();
		usb_endpoint_schedule_ack(endpoint->in);
		//FIXME probably should undo w25q80bv_setup()
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_write_spiflash(
	usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage)
{
	uint32_t addr = 0;
	uint16_t len = 0;

	//FIXME This should refuse to run if executing from SPI flash.

	if (stage == USB_TRANSFER_STAGE_SETUP) {
		addr = (endpoint->setup.value << 16) | endpoint->setup.index;
		len = endpoint->setup.length;
		if ((len > W25Q80BV_PAGE_LEN) || (addr > W25Q80BV_NUM_BYTES)
				|| ((addr + len) > W25Q80BV_NUM_BYTES)) {
			return USB_REQUEST_STATUS_STALL;
		} else {
			usb_endpoint_schedule(endpoint->out, &spiflash_buffer[0], len);
			w25q80bv_setup();
			return USB_REQUEST_STATUS_OK;
		}
	} else if (stage == USB_TRANSFER_STAGE_DATA) {
		addr = (endpoint->setup.value << 16) | endpoint->setup.index;
		len = endpoint->setup.length;
		/* This check is redundant but makes me feel better. */
		if ((len > W25Q80BV_PAGE_LEN) || (addr > W25Q80BV_NUM_BYTES)
				|| ((addr + len) > W25Q80BV_NUM_BYTES)) {
			return USB_REQUEST_STATUS_STALL;
		} else {
			w25q80bv_program(addr, len, &spiflash_buffer[0]);
			usb_endpoint_schedule_ack(endpoint->in);
			//FIXME probably should undo w25q80bv_setup()
			return USB_REQUEST_STATUS_OK;
		}
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_read_spiflash(
	usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage)
{
	uint32_t i;
	uint32_t addr;
	uint16_t len;
	uint8_t* u8_addr_pt;

	if (stage == USB_TRANSFER_STAGE_SETUP) 
	{
		addr = (endpoint->setup.value << 16) | endpoint->setup.index;
		len = endpoint->setup.length;
		if ((len > W25Q80BV_PAGE_LEN) || (addr > W25Q80BV_NUM_BYTES)
		            || ((addr + len) > W25Q80BV_NUM_BYTES)) {
			return USB_REQUEST_STATUS_STALL;
		} else {
			/* TODO flush SPIFI "cache" before to read the SPIFI memory */
			u8_addr_pt = (uint8_t*)addr;
			for(i=0; i<len; i++)
			{
				spiflash_buffer[i] = u8_addr_pt[i];
			}
			usb_endpoint_schedule(endpoint->in, &spiflash_buffer[0], len);
			return USB_REQUEST_STATUS_OK;
		}
	} else if (stage == USB_TRANSFER_STAGE_DATA) 
	{
			addr = (endpoint->setup.value << 16) | endpoint->setup.index;
			len = endpoint->setup.length;
			/* This check is redundant but makes me feel better. */
			if ((len > W25Q80BV_PAGE_LEN) || (addr > W25Q80BV_NUM_BYTES)
					|| ((addr + len) > W25Q80BV_NUM_BYTES)) 
			{
				return USB_REQUEST_STATUS_STALL;
			} else
			{
				usb_endpoint_schedule_ack(endpoint->out);
				return USB_REQUEST_STATUS_OK;
			}
	} else 
	{
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_write_cpld(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	int error, i;
	uint16_t total_len;
	uint16_t len;
	#define WAIT_LOOP_DELAY (6000000)
	#define ALL_LEDS	(PIN_LED1|PIN_LED2|PIN_LED3)

	if (stage == USB_TRANSFER_STAGE_SETUP) 
	{
		// len is limited to 64KB 16bits no overflow can happen
		total_len = endpoint->setup.value;
		len = endpoint->setup.length;
		usb_endpoint_schedule(endpoint->out, &cpld_xsvf_buffer[write_cpld_idx], len);
		return USB_REQUEST_STATUS_OK;
	} else if (stage == USB_TRANSFER_STAGE_DATA) 
	{
		// len is limited to 64KB 16bits no overflow can happen
		total_len = endpoint->setup.value;
		len = endpoint->setup.length;
		write_cpld_idx = write_cpld_idx + len;
		// Check if all bytes received and write CPLD
		if(write_cpld_idx == total_len)
		{
			write_cpld_idx = 0;
			error = cpld_jtag_program(total_len, &cpld_xsvf_buffer[write_cpld_idx]);
			// TO FIX ACK shall be not delayed so much as cpld prog can take up to 5s.
			if(error == 0)
			{		
				usb_endpoint_schedule_ack(endpoint->in);
				
				/* blink LED1, LED2, and LED3 on success */
				while (1)
				{
					gpio_set(PORT_LED1_3, ALL_LEDS); /* LEDs on */
					for (i = 0; i < WAIT_LOOP_DELAY; i++)	/* Wait a bit. */
						__asm__("nop");
					gpio_clear(PORT_LED1_3, ALL_LEDS); /* LEDs off */
					for (i = 0; i < WAIT_LOOP_DELAY; i++)	/* Wait a bit. */
						__asm__("nop");
				}
				return USB_REQUEST_STATUS_OK;
			}else
			{
				/* LED3 (Red) steady on error */
				gpio_set(PORT_LED1_3, PIN_LED3); /* LEDs on */
				while (1);
				return USB_REQUEST_STATUS_STALL;
			}
		}else
		{
			usb_endpoint_schedule_ack(endpoint->in);
			return USB_REQUEST_STATUS_OK;
		}
	} else 
	{
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
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_read_version_string(
	usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage)
{
	uint8_t length;

	if (stage == USB_TRANSFER_STAGE_SETUP) {
		length = (uint8_t)strlen(version_string);
		usb_endpoint_schedule(endpoint->in, version_string, length);
		usb_endpoint_schedule_ack(endpoint->out);
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_freq(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage) 
{
	if (stage == USB_TRANSFER_STAGE_SETUP) 
	{
		usb_endpoint_schedule(endpoint->out, &set_freq_params, sizeof(set_freq_params_t));
		return USB_REQUEST_STATUS_OK;
	} else if (stage == USB_TRANSFER_STAGE_DATA) 
	{
		if( set_freq(set_freq_params.freq_mhz, set_freq_params.freq_hz) ) 
		{
			usb_endpoint_schedule_ack(endpoint->in);
			return USB_REQUEST_STATUS_OK;
		}
		return USB_REQUEST_STATUS_STALL;
	} else
	{
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_set_sample_rate_frac(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage) 
{
	if (stage == USB_TRANSFER_STAGE_SETUP) 
	{
		usb_endpoint_schedule(endpoint->out, &set_sample_r_params, sizeof(set_sample_r_params_t));
		return USB_REQUEST_STATUS_OK;
	} else if (stage == USB_TRANSFER_STAGE_DATA) 
	{
		if( sample_rate_frac_set(set_sample_r_params.freq_hz * 2, set_sample_r_params.divider ) )
		{
			usb_endpoint_schedule_ack(endpoint->in);
			return USB_REQUEST_STATUS_OK;
		}
		return USB_REQUEST_STATUS_STALL;
	} else
	{
		return USB_REQUEST_STATUS_OK;
	}
}

usb_request_status_t usb_vendor_request_set_amp_enable(
	usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		switch (endpoint->setup.value) {
		case 0:
			switchctrl |= SWITCHCTRL_AMP_BYPASS;
			update_switches();
			usb_endpoint_schedule_ack(endpoint->in);
			return USB_REQUEST_STATUS_OK;
		case 1:
			switchctrl &= ~SWITCHCTRL_AMP_BYPASS;
			update_switches();
			usb_endpoint_schedule_ack(endpoint->in);
			return USB_REQUEST_STATUS_OK;
		default:
			return USB_REQUEST_STATUS_STALL;
		}
	} else {
		return USB_REQUEST_STATUS_OK;
	}
}

typedef struct {
	uint32_t part_id[2];
	uint32_t serial_no[4];
} read_partid_serialno_t;

usb_request_status_t usb_vendor_request_read_partid_serialno(
	usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage)
{
	uint8_t length;
	read_partid_serialno_t read_partid_serialno;
	iap_cmd_res_t iap_cmd_res;

	if (stage == USB_TRANSFER_STAGE_SETUP) 
	{
		/* Read IAP Part Number Identification */
		iap_cmd_res.cmd_param.command_code = IAP_CMD_READ_PART_ID_NO;
		iap_cmd_call(&iap_cmd_res);
		if(iap_cmd_res.status_res.status_ret != CMD_SUCCESS)
			return USB_REQUEST_STATUS_STALL;

		read_partid_serialno.part_id[0] = iap_cmd_res.status_res.iap_result[0];
		read_partid_serialno.part_id[1] = iap_cmd_res.status_res.iap_result[1];
		
		/* Read IAP Serial Number Identification */
		iap_cmd_res.cmd_param.command_code = IAP_CMD_READ_SERIAL_NO;
		iap_cmd_call(&iap_cmd_res);
		if(iap_cmd_res.status_res.status_ret != CMD_SUCCESS)
			return USB_REQUEST_STATUS_STALL;

		read_partid_serialno.serial_no[0] = iap_cmd_res.status_res.iap_result[0];
		read_partid_serialno.serial_no[1] = iap_cmd_res.status_res.iap_result[1];
		read_partid_serialno.serial_no[2] = iap_cmd_res.status_res.iap_result[2];
		read_partid_serialno.serial_no[3] = iap_cmd_res.status_res.iap_result[3];
		
		length = (uint8_t)sizeof(read_partid_serialno_t);
		usb_endpoint_schedule(endpoint->in, &read_partid_serialno, length);
		usb_endpoint_schedule_ack(endpoint->out);
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_lna_gain(
	usb_endpoint_t* const endpoint,	const usb_transfer_stage_t stage)
{
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
			const uint8_t value = max2837_set_lna_gain(endpoint->setup.index);
			endpoint->buffer[0] = value;
			usb_endpoint_schedule(endpoint->in, &endpoint->buffer, 1);
			usb_endpoint_schedule_ack(endpoint->out);
			return USB_REQUEST_STATUS_OK;
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_vga_gain(
	usb_endpoint_t* const endpoint,	const usb_transfer_stage_t stage)
{
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
			const uint8_t value = max2837_set_vga_gain(endpoint->setup.index);
			endpoint->buffer[0] = value;
			usb_endpoint_schedule(endpoint->in, &endpoint->buffer, 1);
			usb_endpoint_schedule_ack(endpoint->out);
			return USB_REQUEST_STATUS_OK;
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_txvga_gain(
	usb_endpoint_t* const endpoint,	const usb_transfer_stage_t stage)
{
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
			const uint8_t value = max2837_set_txvga_gain(endpoint->setup.index);
			endpoint->buffer[0] = value;
			usb_endpoint_schedule(endpoint->in, &endpoint->buffer, 1);
			usb_endpoint_schedule_ack(endpoint->out);
			return USB_REQUEST_STATUS_OK;
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_if_freq(
	usb_endpoint_t* const endpoint,	const usb_transfer_stage_t stage
) {
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
		MAX2837_FREQ_NOMINAL_HZ = (uint32_t)endpoint->setup.index * 1000 * 1000;
		set_freq(freq_mhz_cache, freq_hz_cache);
		usb_endpoint_schedule_ack(endpoint->in);
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
	usb_vendor_request_set_sample_rate_frac,
	usb_vendor_request_set_baseband_filter_bandwidth,
	usb_vendor_request_write_rffc5071,
	usb_vendor_request_read_rffc5071,
	usb_vendor_request_erase_spiflash,
	usb_vendor_request_write_spiflash,
	usb_vendor_request_read_spiflash,
	usb_vendor_request_write_cpld,
	usb_vendor_request_read_board_id,
	usb_vendor_request_read_version_string,
	usb_vendor_request_set_freq,
	usb_vendor_request_set_amp_enable,
	usb_vendor_request_read_partid_serialno,
	usb_vendor_request_set_lna_gain,
	usb_vendor_request_set_vga_gain,
	usb_vendor_request_set_txvga_gain,
	usb_vendor_request_set_if_freq,
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
	set_transceiver_mode(transceiver_mode);
	
	if( device->configuration->number ) {
		cpu_clock_pll1_max_speed();
		gpio_set(PORT_LED1_3, PIN_LED1);
	} else {
		/* Configuration number equal 0 means usb bus reset. */
		cpu_clock_pll1_low_speed();
		gpio_clear(PORT_LED1_3, PIN_LED1);
	}
};

void sgpio_isr() {
	SGPIO_CLR_STATUS_1 = (1 << SGPIO_SLICE_A);

	uint32_t* const p = (uint32_t*)&usb_bulk_buffer[usb_bulk_buffer_offset];
	if( transceiver_mode == TRANSCEIVER_MODE_RX ) {
		__asm__(
			"ldr r0, [%[SGPIO_REG_SS], #44]\n\t"
			"rev16 r0, r0\n\t" /* Swap QI -> IQ */
			"str r0, [%[p], #0]\n\t"
			"ldr r0, [%[SGPIO_REG_SS], #20]\n\t"
			"rev16 r0, r0\n\t" /* Swap QI -> IQ */
			"str r0, [%[p], #4]\n\t"
			"ldr r0, [%[SGPIO_REG_SS], #40]\n\t"
			"rev16 r0, r0\n\t" /* Swap QI -> IQ */
			"str r0, [%[p], #8]\n\t"
			"ldr r0, [%[SGPIO_REG_SS], #8]\n\t"
			"rev16 r0, r0\n\t" /* Swap QI -> IQ */
			"str r0, [%[p], #12]\n\t"
			"ldr r0, [%[SGPIO_REG_SS], #36]\n\t"
			"rev16 r0, r0\n\t" /* Swap QI -> IQ */
			"str r0, [%[p], #16]\n\t"
			"ldr r0, [%[SGPIO_REG_SS], #16]\n\t"
			"rev16 r0, r0\n\t" /* Swap QI -> IQ */
			"str r0, [%[p], #20]\n\t"
			"ldr r0, [%[SGPIO_REG_SS], #32]\n\t"
			"rev16 r0, r0\n\t" /* Swap QI -> IQ */
			"str r0, [%[p], #24]\n\t"
			"ldr r0, [%[SGPIO_REG_SS], #0]\n\t"
			"rev16 r0, r0\n\t" /* Swap QI -> IQ */
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

	pin_setup();
	enable_1v8_power();
	cpu_clock_init();

	usb_peripheral_reset();
	
	usb_device_init(0, &usb_device);
	
	usb_endpoint_init(&usb_endpoint_control_out);
	usb_endpoint_init(&usb_endpoint_control_in);
	
	nvic_set_priority(NVIC_USB0_IRQ, 255);

	usb_run(&usb_device);
	
    ssp1_init();
	ssp1_set_mode_max5864();
	max5864_xcvr();

	ssp1_set_mode_max2837();
	max2837_setup();
	max2837_set_frequency(ifreq);

	rffc5071_setup();

#ifdef JAWBREAKER
	switchctrl = SWITCHCTRL_AMP_BYPASS;
#endif

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
