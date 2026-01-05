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

#include "hackrf_core.h"
#include "lz4_blk.h"
#include "selftest.h"

// FPGA bitstreams blob.
extern uint32_t _binary_fpga_bin_start;
extern uint32_t _binary_fpga_bin_end;
extern uint32_t _binary_fpga_bin_size;

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
	selftest.fpga_image_load = SKIPPED;
	selftest.report.pass = false;
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
	ice40_spi_target_init(&ice40);
	ssp1_set_mode_ice40();
	struct fpga_image_read_ctx fpga_image_ctx = {
		.addr = (uint32_t) &_binary_fpga_bin_start + bitstream_offset,
	};
	const bool success = ice40_spi_syscfg_program(
		&ice40,
		fpga_image_read_block_cb,
		&fpga_image_ctx);
	ssp1_set_mode_max283x();

	// Update selftest result.
	selftest.fpga_image_load = success ? PASSED : FAILED;
	if (selftest.fpga_image_load != PASSED) {
		selftest.report.pass = false;
	}

	return success;
}
