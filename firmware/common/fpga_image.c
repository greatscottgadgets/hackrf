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
#include <stddef.h>
#include <stdint.h>

#include "fpga.h"
#include "ice40_spi.h"
#include "lz4_blk.h"
#include "max283x.h"
#include "selftest.h"

struct fpga_image_read_ctx {
	struct fpga_loader_t* loader;
	uint32_t addr;
	size_t next_block_sz;
	uint8_t init_flag;
};

static size_t fpga_image_read_block_cb(void* _ctx)
{
	// Assume out_buffer is 4KB
	struct fpga_image_read_ctx* ctx = _ctx;
	struct fpga_loader_t* loader = ctx->loader;
	size_t block_sz = ctx->next_block_sz;

	uint8_t block_sz_buf[2];

	// first iteration: read first block size
	if (ctx->init_flag == 0) {
		loader->read(ctx->addr, 2, block_sz_buf);
		block_sz = block_sz_buf[0] | block_sz_buf[1] << 8;
		ctx->addr += 2;
		ctx->init_flag = 1;
	}

	// finish at end marker
	if (block_sz == 0)
		return 0;

	// Read compressed block (and the next block size) from flash.
	loader->read(ctx->addr, block_sz, loader->in_buffer);
	ctx->addr += block_sz;
	loader->read(ctx->addr, 2, block_sz_buf);
	ctx->next_block_sz = block_sz_buf[0] | block_sz_buf[1] << 8;
	ctx->addr += 2;

	// Decompress block.
	return lz4_blk_decompress(loader->in_buffer, loader->out_buffer, block_sz);
}

bool fpga_image_load(struct fpga_loader_t* loader, unsigned int index)
{
	// TODO: do SPI setup and read number of bitstreams once!
	if (loader->setup != NULL)
		loader->setup();

	// Read number of bitstreams from flash.
	// Check the bitstream exists, and extract its offset.
	uint32_t addr = loader->start_addr;
	uint32_t num_bitstreams, bitstream_offset;
	loader->read(addr, 4, (uint8_t*) &num_bitstreams);
	if (index >= num_bitstreams)
		return false;
	loader->read(addr + 4 * (index + 1), 4, (uint8_t*) &bitstream_offset);

	// A callback function is used by the FPGA programmer
	// to obtain consecutive gateware chunks.
	ice40_spi_target_init(&ice40);
	ssp1_set_mode_ice40();
	struct fpga_image_read_ctx fpga_image_ctx = {
		.loader = loader,
		.addr = loader->start_addr + bitstream_offset,
	};
	const bool success = ice40_spi_syscfg_program(
		&ice40,
		loader->out_buffer,
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
