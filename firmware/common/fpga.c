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

#include <stdbool.h>
#include "hackrf_core.h"
#include "ice40_spi.h"
#include "lz4_blk.h"
#include "m0_state.h"
#include "streaming.h"
#include "rf_path.h"
#include "selftest.h"

// FPGA bitstreams blob.
extern uint32_t _binary_fpga_bin_start;
extern uint32_t _binary_fpga_bin_end;
extern uint32_t _binary_fpga_bin_size;

// USB buffer used during selftests.
#define USB_BULK_BUFFER_SIZE 0x8000
extern uint8_t usb_bulk_buffer[USB_BULK_BUFFER_SIZE];

struct fpga_image_read_ctx {
	uint32_t addr;
	size_t next_block_sz;
	uint8_t init_flag;
	uint8_t buffer[4096 + 2];
};

static size_t fpga_image_read_block_cb(void* _ctx, uint8_t* out_buffer)
{
	// Assume out_buffer is 4KB
	struct fpga_image_read_ctx* ctx = _ctx;
	size_t block_sz = ctx->next_block_sz;

	// first iteration: read first block size
	if (ctx->init_flag == 0) {
		w25q80bv_read(&spi_flash, ctx->addr, 2, ctx->buffer);
		block_sz = ctx->buffer[0] | (ctx->buffer[1] << 8);
		ctx->addr += 2;
		ctx->init_flag = 1;
	}

	// finish at end marker
	if (block_sz == 0)
		return 0;

	// Read compressed block (and the next block size) from flash.
	w25q80bv_read(&spi_flash, ctx->addr, block_sz + 2, ctx->buffer);
	ctx->addr += block_sz + 2;
	ctx->next_block_sz = ctx->buffer[block_sz] | (ctx->buffer[block_sz + 1] << 8);

	// Decompress block.
	return lz4_blk_decompress(ctx->buffer, out_buffer, block_sz);
}

bool fpga_image_load(unsigned int index)
{
#if defined(DFU_MODE) || defined(RAM_MODE)
	return false;
#endif

	// TODO: do SPI setup and read number of bitstreams once!

	// Prepare for SPI flash access.
	spi_bus_start(spi_flash.bus, &ssp_config_w25q80bv);
	w25q80bv_setup(&spi_flash);

	// Read number of bitstreams from flash.
	// Check the bitstream exists, and extract its offset.
	uint32_t addr = (uint32_t) &_binary_fpga_bin_start;
	uint32_t num_bitstreams, bitstream_offset;
	w25q80bv_read(&spi_flash, addr, 4, (uint8_t*) &num_bitstreams);
	if (index >= num_bitstreams)
		return false;
	w25q80bv_read(&spi_flash, addr + 4 * (index + 1), 4, (uint8_t*) &bitstream_offset);

	// A callback function is used by the FPGA programmer
	// to obtain consecutive gateware chunks.
	ssp1_set_mode_ice40();
	ice40_spi_target_init(&ice40);
	struct fpga_image_read_ctx fpga_image_ctx = {
		.addr = (uint32_t) &_binary_fpga_bin_start + bitstream_offset,
	};
	const bool success = ice40_spi_syscfg_program(
		&ice40,
		fpga_image_read_block_cb,
		&fpga_image_ctx);
	ssp1_set_mode_max283x();

	return success;
}

static uint8_t lfsr_advance(uint8_t v)
{
	const uint8_t feedback = ((v >> 3) ^ (v >> 4) ^ (v >> 5) ^ (v >> 7)) & 1;
	return (v << 1) | feedback;
}

bool fpga_sgpio_selftest()
{
#if defined(DFU_MODE) || defined(RAM_MODE)
	return true;
#endif

	// Enable PRBS mode.
	ssp1_set_mode_ice40();
	ice40_spi_write(&ice40, 0x01, 0x40);
	ssp1_set_mode_max283x();

	// Stream 512 samples from the FPGA.
	sgpio_configure(&sgpio_config, SGPIO_DIRECTION_RX);
	m0_set_mode(M0_MODE_RX);
	m0_state.shortfall_limit = 0;
	baseband_streaming_enable(&sgpio_config);
	while (m0_state.m0_count < 512)
		;
	baseband_streaming_disable(&sgpio_config);
	m0_set_mode(M0_MODE_IDLE);

	// Disable PRBS mode.
	ssp1_set_mode_ice40();
	ice40_spi_write(&ice40, 0x01, 0);
	ssp1_set_mode_max283x();

	// Generate sequence from first value and compare.
	bool seq_in_sync = true;
	uint8_t seq = lfsr_advance(usb_bulk_buffer[0]);
	for (int i = 1; i < 512; ++i) {
		if (usb_bulk_buffer[i] != seq) {
			seq_in_sync = false;
			break;
		}
		seq = lfsr_advance(seq);
	}

	// Update selftest result.
	selftest.sgpio_rx_ok = seq_in_sync;
	if (!selftest.sgpio_rx_ok) {
		selftest.report.pass = false;
	}

	return selftest.sgpio_rx_ok;
}

bool fpga_if_xcvr_selftest()
{
#if defined(DFU_MODE) || defined(RAM_MODE)
	return true;
#endif

	const size_t num_samples = USB_BULK_BUFFER_SIZE / 2;

	// Set gateware features for the test.
	ssp1_set_mode_ice40();
	ice40_spi_write(&ice40, 0x01, 0x1); // RX DC block
	ice40_spi_write(&ice40, 0x05, 128); // NCO phase increment
	ice40_spi_write(&ice40, 0x03, 1);   // NCO TX enable
	ssp1_set_mode_max283x();

	// Configure RX calibration path and settle for 1ms.
	rf_path_set_direction(&rf_path, RF_PATH_DIRECTION_RX_CALIBRATION);
	delay_us_at_mhz(1000, 204);

	// Stream samples from the FPGA.
	m0_set_mode(M0_MODE_RX);
	m0_state.shortfall_limit = 0;
	baseband_streaming_enable(&sgpio_config);
	while (m0_state.m0_count < num_samples)
		;
	baseband_streaming_disable(&sgpio_config);
	m0_set_mode(M0_MODE_IDLE);

	rf_path_set_direction(&rf_path, RF_PATH_DIRECTION_OFF);

	// Gateware default settings.
	ssp1_set_mode_ice40();
	ice40_spi_write(&ice40, 0x01, 0);
	ice40_spi_write(&ice40, 0x03, 0);
	ssp1_set_mode_max283x();

	// Count zero crossings in the received samples.
	// N/2 samples/channel * 2 zcs/cycle / 8 samples/cycle = N/8 zcs/channel
	unsigned int expected_zcs = num_samples / 8;

	unsigned int zcs_i = 0;
	unsigned int zcs_q = 0;
	uint8_t last_sign_i = 0;
	uint8_t last_sign_q = 0;
	for (size_t i = 0; i < num_samples; i += 2) {
		uint8_t sign_i = (usb_bulk_buffer[i] & 0x80) ? 1 : 0;
		uint8_t sign_q = (usb_bulk_buffer[i + 1] & 0x80) ? 1 : 0;
		zcs_i += sign_i ^ last_sign_i;
		zcs_q += sign_q ^ last_sign_q;
		last_sign_i = sign_i;
		last_sign_q = sign_q;
	}

	// Allow a zero crossings counting error of +-5%.
	bool i_in_range = (zcs_i > expected_zcs * 0.95) && (zcs_i < expected_zcs * 1.05);
	bool q_in_range = (zcs_q > expected_zcs * 0.95) && (zcs_q < expected_zcs * 1.05);

	// Update selftest result.
	selftest.xcvr_loopback_ok = i_in_range && q_in_range;
	if (!selftest.xcvr_loopback_ok) {
		selftest.report.pass = false;
	}

	return selftest.xcvr_loopback_ok;
}
