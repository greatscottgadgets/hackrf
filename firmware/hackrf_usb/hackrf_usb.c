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

#include <libopencm3/cm3/vector.h>

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
#include "usb_queue.h"
#include "usb_request.h"
#include "usb_descriptor.h"
#include "usb_standard_request.h"

#include "rf_path.h"
#include "tuning.h"
#include "sgpio_isr.h"
#include "usb_bulk_buffer.h"

static volatile transceiver_mode_t transceiver_mode = TRANSCEIVER_MODE_OFF;

static volatile bool start_cpld_update = false;
uint8_t cpld_xsvf_buffer[512];
volatile bool cpld_wait = false;

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

usb_configuration_t usb_configuration_cpld_update_full_speed = {
	.number = 2,
	.speed = USB_SPEED_FULL,
	.descriptor = usb_descriptor_configuration_cpld_update_full_speed,
};

usb_configuration_t usb_configuration_cpld_update_high_speed = {
	.number = 2,
	.speed = USB_SPEED_HIGH,
	.descriptor = usb_descriptor_configuration_cpld_update_high_speed,
};

usb_configuration_t* usb_configurations[] = {
	&usb_configuration_high_speed,
	&usb_configuration_full_speed,
	&usb_configuration_cpld_update_full_speed,
	&usb_configuration_cpld_update_high_speed,
	0,
};

usb_device_t usb_device = {
	.descriptor = usb_descriptor_device,
	.descriptor_strings = usb_descriptor_strings,
	.qualifier_descriptor = usb_descriptor_device_qualifier,
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
USB_DEFINE_QUEUE(usb_endpoint_control_out, 4);

usb_endpoint_t usb_endpoint_control_in = {
	.address = 0x80,
	.device = &usb_device,
	.in = &usb_endpoint_control_in,
	.out = &usb_endpoint_control_out,
	.setup_complete = 0,
	.transfer_complete = usb_control_in_complete,
};
static USB_DEFINE_QUEUE(usb_endpoint_control_in, 4);

// NOTE: Endpoint number for IN and OUT are different. I wish I had some
// evidence that having BULK IN and OUT on separate endpoint numbers was
// actually a good idea. Seems like everybody does it that way, but why?

usb_endpoint_t usb_endpoint_bulk_in = {
	.address = 0x81,
	.device = &usb_device,
	.in = &usb_endpoint_bulk_in,
	.out = 0,
	.setup_complete = 0,
	.transfer_complete = usb_queue_transfer_complete
};
static USB_DEFINE_QUEUE(usb_endpoint_bulk_in, 4);

usb_endpoint_t usb_endpoint_bulk_out = {
	.address = 0x02,
	.device = &usb_device,
	.in = 0,
	.out = &usb_endpoint_bulk_out,
	.setup_complete = 0,
	.transfer_complete = usb_queue_transfer_complete
};
static USB_DEFINE_QUEUE(usb_endpoint_bulk_out, 4);

void baseband_streaming_disable() {
	sgpio_cpld_stream_disable();

	nvic_disable_irq(NVIC_SGPIO_IRQ);
	
	usb_endpoint_disable(&usb_endpoint_bulk_in);
	usb_endpoint_disable(&usb_endpoint_bulk_out);
}

void set_transceiver_mode(const transceiver_mode_t new_transceiver_mode) {
	baseband_streaming_disable();
	
	transceiver_mode = new_transceiver_mode;
	
	if( transceiver_mode == TRANSCEIVER_MODE_RX ) {
		gpio_clear(PORT_LED1_3, PIN_LED3);
		gpio_set(PORT_LED1_3, PIN_LED2);
		usb_endpoint_init(&usb_endpoint_bulk_in);
		rf_path_set_direction(RF_PATH_DIRECTION_RX);
	} else if (transceiver_mode == TRANSCEIVER_MODE_TX) {
		gpio_clear(PORT_LED1_3, PIN_LED2);
		gpio_set(PORT_LED1_3, PIN_LED3);
		usb_endpoint_init(&usb_endpoint_bulk_out);
		rf_path_set_direction(RF_PATH_DIRECTION_TX);
	} else {
		gpio_clear(PORT_LED1_3, PIN_LED2);
		gpio_clear(PORT_LED1_3, PIN_LED3);
		rf_path_set_direction(RF_PATH_DIRECTION_OFF);
	}

	sgpio_configure(transceiver_mode, true);

	if( transceiver_mode != TRANSCEIVER_MODE_OFF ) {
		vector_table_entry_t sgpio_isr_fn = sgpio_isr_rx;
		if( transceiver_mode == TRANSCEIVER_MODE_TX ) {
			sgpio_isr_fn = sgpio_isr_tx;
		}
		vector_table.irq[NVIC_SGPIO_IRQ] = sgpio_isr_fn;
		
		nvic_set_priority(NVIC_SGPIO_IRQ, 0);
		nvic_enable_irq(NVIC_SGPIO_IRQ);
		SGPIO_SET_EN_1 = (1 << SGPIO_SLICE_A);

	    sgpio_cpld_stream_enable();
	}
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

usb_request_status_t usb_vendor_request_write_max2837(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
		if( endpoint->setup.index < MAX2837_NUM_REGS ) {
			if( endpoint->setup.value < MAX2837_DATA_REGS_MAX_VALUE ) {
				max2837_reg_write(endpoint->setup.index, endpoint->setup.value);
				usb_transfer_schedule_ack(endpoint->in);
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
			usb_transfer_schedule_block(endpoint->in, &endpoint->buffer, 2,
						    NULL, NULL);
			usb_transfer_schedule_ack(endpoint->out);
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
	const usb_transfer_stage_t stage
) {
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
		if( endpoint->setup.index < 256 ) {
			const uint8_t value = si5351c_read_single(endpoint->setup.index);
			endpoint->buffer[0] = value;
			usb_transfer_schedule_block(endpoint->in, &endpoint->buffer, 1,
						    NULL, NULL);
			usb_transfer_schedule_ack(endpoint->out);
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
			usb_transfer_schedule_ack(endpoint->in);
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
			usb_transfer_schedule_block(endpoint->in, &endpoint->buffer, 2,
						    NULL, NULL);
			usb_transfer_schedule_ack(endpoint->out);
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
		usb_transfer_schedule_ack(endpoint->in);
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
			usb_transfer_schedule_block(endpoint->out, &spiflash_buffer[0], len,
						    NULL, NULL);
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
			usb_transfer_schedule_ack(endpoint->in);
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
			u8_addr_pt = (uint8_t*)(addr + SPIFI_DATA_UNCACHED_BASE);
			for(i=0; i<len; i++)
			{
				spiflash_buffer[i] = u8_addr_pt[i];
			}
			usb_transfer_schedule_block(endpoint->in, &spiflash_buffer[0], len,
						    NULL, NULL);
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
				usb_transfer_schedule_ack(endpoint->out);
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
		usb_transfer_schedule_block(endpoint->in, &endpoint->buffer, 1, NULL, NULL);
		usb_transfer_schedule_ack(endpoint->out);
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_read_version_string(
	usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage)
{
	uint8_t length;

	if (stage == USB_TRANSFER_STAGE_SETUP) {
		length = (uint8_t)strlen(version_string);
		usb_transfer_schedule_block(endpoint->in, version_string, length, NULL, NULL);
		usb_transfer_schedule_ack(endpoint->out);
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_freq(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage) 
{
	if (stage == USB_TRANSFER_STAGE_SETUP) 
	{
		usb_transfer_schedule_block(endpoint->out, &set_freq_params, sizeof(set_freq_params_t),
					    NULL, NULL);
		return USB_REQUEST_STATUS_OK;
	} else if (stage == USB_TRANSFER_STAGE_DATA) 
	{
		if( set_freq(set_freq_params.freq_mhz, set_freq_params.freq_hz) ) 
		{
			usb_transfer_schedule_ack(endpoint->in);
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
                usb_transfer_schedule_block(endpoint->out, &set_sample_r_params, sizeof(set_sample_r_params_t),
					    NULL, NULL);
		return USB_REQUEST_STATUS_OK;
	} else if (stage == USB_TRANSFER_STAGE_DATA) 
	{
		if( sample_rate_frac_set(set_sample_r_params.freq_hz * 2, set_sample_r_params.divider ) )
		{
			usb_transfer_schedule_ack(endpoint->in);
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
			rf_path_set_lna(0);
			usb_transfer_schedule_ack(endpoint->in);
			return USB_REQUEST_STATUS_OK;
		case 1:
			rf_path_set_lna(1);
			usb_transfer_schedule_ack(endpoint->in);
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
		usb_transfer_schedule_block(endpoint->in, &read_partid_serialno, length,
					    NULL, NULL);
		usb_transfer_schedule_ack(endpoint->out);
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_lna_gain(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
			const uint8_t value = max2837_set_lna_gain(endpoint->setup.index);
			endpoint->buffer[0] = value;
			usb_transfer_schedule_block(endpoint->in, &endpoint->buffer, 1,
						    NULL, NULL);
			usb_transfer_schedule_ack(endpoint->out);
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
			usb_transfer_schedule_block(endpoint->in, &endpoint->buffer, 1,
						    NULL, NULL);
			usb_transfer_schedule_ack(endpoint->out);
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
			usb_transfer_schedule_block(endpoint->in, &endpoint->buffer, 1,
						    NULL, NULL);
			usb_transfer_schedule_ack(endpoint->out);
			return USB_REQUEST_STATUS_OK;
	}
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_set_if_freq(
	usb_endpoint_t* const endpoint,	const usb_transfer_stage_t stage
) {
	if( stage == USB_TRANSFER_STAGE_SETUP ) {
		if( set_freq_if((uint32_t)endpoint->setup.index * 1000 * 1000) ) {
			usb_transfer_schedule_ack(endpoint->in);
		} else {
			return USB_REQUEST_STATUS_STALL;
		}
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
	NULL, // used to be write_cpld
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

static void cpld_buffer_refilled(void* user_data, unsigned int length)
{
	cpld_wait = false;
}

static void refill_cpld_buffer(void)
{
	cpld_wait = true;
	usb_transfer_schedule(
		&usb_endpoint_bulk_out,
		cpld_xsvf_buffer,
		sizeof(cpld_xsvf_buffer),
		cpld_buffer_refilled,
		NULL
		);

	// Wait until transfer finishes
	while (cpld_wait);
}

static void cpld_update(void)
{
	#define WAIT_LOOP_DELAY (6000000)
	#define ALL_LEDS  (PIN_LED1|PIN_LED2|PIN_LED3)
	int i;
	int error;

	usb_queue_flush_endpoint(&usb_endpoint_bulk_in);
	usb_queue_flush_endpoint(&usb_endpoint_bulk_out);

	refill_cpld_buffer();

	error = cpld_jtag_program(sizeof(cpld_xsvf_buffer),
				  cpld_xsvf_buffer,
				  refill_cpld_buffer);
	if(error == 0)
	{
		/* blink LED1, LED2, and LED3 on success */
		while (1)
		{
			gpio_set(PORT_LED1_3, ALL_LEDS); /* LEDs on */
			for (i = 0; i < WAIT_LOOP_DELAY; i++)  /* Wait a bit. */
				__asm__("nop");
			gpio_clear(PORT_LED1_3, ALL_LEDS); /* LEDs off */
			for (i = 0; i < WAIT_LOOP_DELAY; i++)  /* Wait a bit. */
				__asm__("nop");
		}
	}else
	{
		/* LED3 (Red) steady on error */
		gpio_set(PORT_LED1_3, PIN_LED3); /* LEDs on */
		while (1);
	}
}

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
		     && transceiver_mode != TRANSCEIVER_MODE_OFF) {
			usb_transfer_schedule_block(
				(transceiver_mode == TRANSCEIVER_MODE_RX)
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
		     && transceiver_mode != TRANSCEIVER_MODE_OFF) {
			usb_transfer_schedule_block(
				(transceiver_mode == TRANSCEIVER_MODE_RX)
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
