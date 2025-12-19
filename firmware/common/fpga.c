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
#include "fpga.h"
#include "fpga_regs.def"

/* Set up all registers according to the loaded bitstream's defaults. */
void fpga_init(fpga_driver_t* const drv)
{
	// Standard bitstream default register values.
	set_FPGA_STANDARD_CTRL(drv, 0);
	set_FPGA_STANDARD_TX_CTRL(drv, 0);

	// TODO support the other bitstreams

	fpga_regs_commit(drv);
}

void fpga_setup(fpga_driver_t* const drv)
{
	// TODO implement fpga_target.c

	fpga_init(drv);
}

uint8_t fpga_reg_read(fpga_driver_t* const drv, uint8_t r)
{
	uint8_t v;
	ssp1_set_mode_ice40();
	v = ice40_spi_read(drv->bus, r);
	ssp1_set_mode_max283x();
	drv->regs[r] = v;
	return v;
}

void fpga_reg_write(fpga_driver_t* const drv, uint8_t r, uint8_t v)
{
	drv->regs[r] = v;
	ssp1_set_mode_ice40();
	ice40_spi_write(drv->bus, r, v);
	ssp1_set_mode_max283x();
	FPGA_REG_SET_CLEAN(drv, r);
}

static inline void fpga_reg_commit(fpga_driver_t* const drv, uint8_t r)
{
	fpga_reg_write(drv, r, drv->regs[r]);
}

void fpga_regs_commit(fpga_driver_t* const drv)
{
	int r;
	for (r = 1; r < FPGA_NUM_REGS; r++) {
		if ((drv->regs_dirty >> r) & 0x1) {
			fpga_reg_commit(drv, r);
		}
	}
}

void fpga_set_hw_sync_enable(fpga_driver_t* const drv, const hw_sync_mode_t hw_sync_mode)
{
	fpga_reg_read(drv, FPGA_STANDARD_CTRL);
	set_FPGA_STANDARD_CTRL_TRIGGER_EN(drv, hw_sync_mode == 1);
	fpga_regs_commit(drv);
}

void fpga_set_rx_dc_block_enable(fpga_driver_t* const drv, const bool enable)
{
	set_FPGA_STANDARD_CTRL_DC_BLOCK(drv, enable);
	fpga_regs_commit(drv);
}

void fpga_set_rx_decimation_ratio(fpga_driver_t* const drv, const uint8_t value)
{
	set_FPGA_STANDARD_RX_DECIM(drv, value & 0b111);
	fpga_regs_commit(drv);
}

void fpga_set_rx_quarter_shift_mode(
	fpga_driver_t* const drv,
	const fpga_quarter_shift_mode_t mode)
{
	set_FPGA_STANDARD_CTRL_QUARTER_SHIFT_EN(drv, (mode >> 0) & 0b1);
	set_FPGA_STANDARD_CTRL_QUARTER_SHIFT_UP(drv, (mode >> 1) & 0b1);
	fpga_regs_commit(drv);
}

void fpga_set_tx_interpolation_ratio(fpga_driver_t* const drv, const uint8_t value)
{
	set_FPGA_STANDARD_TX_INTRP(drv, value & 0b111);
	fpga_regs_commit(drv);
}

void fpga_set_prbs_enable(fpga_driver_t* const drv, const bool enable)
{
	set_FPGA_STANDARD_CTRL(drv, 0);
	if (enable) {
		set_FPGA_STANDARD_CTRL_PRBS(drv, enable);
	}
	fpga_regs_commit(drv);
}

void fpga_set_tx_nco_enable(fpga_driver_t* const drv, const bool enable)
{
	set_FPGA_STANDARD_TX_CTRL_NCO_EN(drv, enable);
	fpga_regs_commit(drv);
}

void fpga_set_tx_nco_pstep(fpga_driver_t* const drv, const uint8_t phase_increment)
{
	set_FPGA_STANDARD_TX_PSTEP(drv, phase_increment);
	fpga_regs_commit(drv);
}
