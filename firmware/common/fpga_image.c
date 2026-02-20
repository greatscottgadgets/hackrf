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
#include "lz4_buf.h"
#include "selftest.h"

// FPGA bitstreams blob.
extern uint32_t _binary_fpga_bin_start;
extern uint32_t _binary_fpga_bin_end;
extern uint32_t _binary_fpga_bin_size;

struct fpga_image_read_ctx {
	uint32_t addr;
	size_t next_block_sz;
	uint8_t init_flag;
};

struct spifi_fpga_read_ctx {
	const uint8_t* mem_ptr;
	size_t next_block_sz;
	uint8_t init_flag;
	uint8_t buffer[4096 + 2];
};

static size_t fpga_image_read_block_cb(void* _ctx, uint8_t* out_buffer);

// @brief Add FPGA image loading from SPIFI for pp -> hackrf mode RAM boot method
// @param _ctx
// @param out_buffer
// @return
static size_t spifi_fpga_read_block_cb(void* _ctx, uint8_t* out_buffer)
{
	struct spifi_fpga_read_ctx* ctx = (struct spifi_fpga_read_ctx*) _ctx;
	size_t block_sz = ctx->next_block_sz;

	// First iteration: read first block size from SPIFI memory
	if (ctx->init_flag == 0) {
		block_sz = ctx->mem_ptr[0] | (ctx->mem_ptr[1] << 8);
		ctx->mem_ptr += 2;
		ctx->init_flag = 1;
	}

	// Finish at end marker (block_sz == 0)
	if (block_sz == 0)
		return 0;

	// Read compressed block from SPIFI memory
	memcpy(ctx->buffer, ctx->mem_ptr, block_sz + 2);
	ctx->mem_ptr += block_sz + 2;

	// Extract next block size
	ctx->next_block_sz = ctx->buffer[block_sz] | (ctx->buffer[block_sz + 1] << 8);

	// Decompress block using LZ4
	return lz4_blk_decompress(ctx->buffer, out_buffer, block_sz);
}

// @brief Load FPGA bitstream from SPIFI flash memory and program the FPGA.
// @param index Index of the FPGA bitstream to load from flash.
// @return true on success, false on failure.
static bool fpga_image_load_from_spifi(unsigned int index)
{
// PRALINE: FPGA bitstream is in flash at 0x380000 (SPIFI mapped at 0x14380000)
#define FPGA_BITSTREAM_FLASH_ADDR 0x380000

	spi_bus_start(spi_flash.bus, &ssp_config_w25q80bv);
	w25q80bv_setup(&spi_flash);

	uint32_t num_bitstreams;
	w25q80bv_read(
		&spi_flash,
		FPGA_BITSTREAM_FLASH_ADDR,
		4,
		(uint8_t*) &num_bitstreams);

	// Check if header looks valid
	if (num_bitstreams == 0 || num_bitstreams > 16 || num_bitstreams == 0xFFFFFFFF) {
		while (1)
			;
		return false;
	}

	if (index >= num_bitstreams) {
		return false;
	}

	uint32_t bitstream_offset;
	w25q80bv_read(
		&spi_flash,
		FPGA_BITSTREAM_FLASH_ADDR + 4 + index * 4,
		4,
		(uint8_t*) &bitstream_offset);

	// Initialize SSP1 for iCE40 programming
	ice40_spi_target_init(&ice40);
	ssp1_set_mode_ice40();

	delay_us_at_mhz(2000, 204);

	// Calculate start address of bitstream in flash
	uint32_t bitstream_addr = FPGA_BITSTREAM_FLASH_ADDR + bitstream_offset;

	struct fpga_image_read_ctx fpga_image_ctx = {
		.addr = bitstream_addr,
	};

	const bool success = ice40_spi_syscfg_program(
		&ice40,
		fpga_image_read_block_cb,
		&fpga_image_ctx);

	ssp1_set_mode_max283x();

	return success;
}

static size_t fpga_image_read_block_cb(void* _ctx, uint8_t* out_buffer)
{
	// Assume out_buffer is 4KB
	struct fpga_image_read_ctx* ctx = _ctx;
	size_t block_sz = ctx->next_block_sz;

	uint8_t block_sz_buf[2];

	// first iteration: read first block size
	if (ctx->init_flag == 0) {
		w25q80bv_read(&spi_flash, ctx->addr, 2, block_sz_buf);
		block_sz = block_sz_buf[0] | block_sz_buf[1] << 8;
		ctx->addr += 2;
		ctx->init_flag = 1;
	}

	// finish at end marker
	if (block_sz == 0)
		return 0;

	// Read compressed block (and the next block size) from flash.
	w25q80bv_read(&spi_flash, ctx->addr, block_sz, lz4_in_buf);
	ctx->addr += block_sz;
	w25q80bv_read(&spi_flash, ctx->addr, 2, block_sz_buf);
	ctx->next_block_sz = block_sz_buf[0] | block_sz_buf[1] << 8;
	ctx->addr += 2;

	// Decompress block.
	return lz4_blk_decompress(lz4_in_buf, out_buffer, block_sz);
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

// @brief Interface for loading HACKRF from PP mode
// @param index
// @return
bool fpga_image_load_for_pp(unsigned int index)
{
	// Updated to return the correct result
	bool success = fpga_image_load_from_spifi(index);

	selftest.fpga_image_load = success ? PASSED : FAILED;
	// selftest.fpga_image_load = PASSED;

	if (selftest.fpga_image_load != PASSED) {
		selftest.report.pass = false;
	}

	return success;
}
