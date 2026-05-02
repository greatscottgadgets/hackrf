/*
 * Copyright 2012-2026 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2012 Jared Boone <jared@sharebrained.com>
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
#include <string.h>

#include "clock_io.h"
#include "delay.h"
#include "platform_detect.h"
#include "selftest.h"
#include "si5351c.h"
#ifdef IS_HACKRF_ONE
	#include <libopencm3/lpc43xx/scu.h>
	#include "gpio.h"
	#include "platform_gpio.h"
	#include "platform_scu.h"
#endif

#include "si5351c_regs.def"

static enum pll_sources active_clock_source = PLL_SOURCE_UNINITIALIZED;
/* External clock output default is deactivated as it creates noise */
static bool clkout_enabled = false;

/* write to single register */
void si5351c_write_single(si5351c_driver_t* const drv, uint8_t reg, uint8_t val)
{
	const uint8_t data_tx[] = {reg, val};
	i2c_bus_transfer(drv->bus, drv->i2c_address, data_tx, 2, NULL, 0);
	if (reg < SI5351C_CACHED_REGS) {
		drv->regs[reg] = val;
		si5351c_reg_set_clean(drv, reg);
	}
}

/* read single register */
uint8_t si5351c_read_single(si5351c_driver_t* const drv, uint8_t reg)
{
	const uint8_t data_tx[] = {reg};
	uint8_t data_rx[] = {0x00};
	i2c_bus_transfer(drv->bus, drv->i2c_address, data_tx, 1, data_rx, 1);
	uint8_t val = data_rx[0];
	if (reg < SI5351C_CACHED_REGS) {
		drv->regs[reg] = val;
		si5351c_reg_set_clean(drv, reg);
	}
	return val;
}

/* Commit changes to register values. */
void si5351c_regs_commit(si5351c_driver_t* drv)
{
	for (int start = 0; start < SI5351C_CACHED_REGS; start++) {
		if (si5351c_reg_is_dirty(drv, start)) {
			if (start == SI5351C_CACHED_REGS - 1) {
				si5351c_write_single(drv, start, drv->regs[start]);
			} else {
				int end;
				for (end = start + 1; end < 256; end++) {
					if (!si5351c_reg_is_dirty(drv, end))
						break;
				}
				size_t len = 1 + (end - start);
				uint8_t data_tx[len];
				data_tx[0] = start;
				for (int i = 0; i < (end - start); i++) {
					data_tx[1 + i] = drv->regs[start + i];
				}
				i2c_bus_transfer(
					drv->bus,
					drv->i2c_address,
					data_tx,
					len,
					NULL,
					0);
				for (int i = start; i < end; i++) {
					si5351c_reg_set_clean(drv, i);
				}
				start = end;
			}
		}
	}
}

/* Disable all CLKx outputs. */
void si5351c_disable_all_outputs(si5351c_driver_t* const drv)
{
	set_all_CLK_OEB(drv, SI5351C_OUTPUT_DISABLE);
	si5351c_regs_commit(drv);
}

/* Disable all CLKx outputs using selected PLL(s). */
void si5351c_disable_pll_outputs(si5351c_driver_t* const drv, si5351c_pll_mask_t mask)
{
	/* For each CLKx output, check if it is using the specified PLL. */
	for (int op = 0; op < 8; op++) {
		int ms;
		/* First check which multisynth is used by this output. */
		switch (get_CLK_SRC(drv, op)) {
		case SI5351C_SRC_MULTISYNTH_SELF:
			ms = op;
			break;
		case SI5351C_SRC_MULTISYNTH_0_4:
			ms = (op < 4) ? 0 : 4;
			break;
		default:
			/* This output is not using any PLL */
			continue;
		}
		/* Now check which PLL is used by that multisynth. */
		if (mask & (1 << get_MS_SRC(drv, ms))) {
			/* This output depends on the specified PLL; disable it. */
			set_CLK_OEB(drv, op, SI5351C_OUTPUT_DISABLE);
		}
	}
	si5351c_regs_commit(drv);
}

/* Turn off OEB pin control for all CLKx */
void si5351c_disable_oeb_pin_control(si5351c_driver_t* const drv)
{
	set_all_OEB_MASK(drv, true);
	si5351c_regs_commit(drv);
}

/* Power down all CLKx */
void si5351c_power_down_all_clocks(si5351c_driver_t* const drv)
{
	set_all_CLK_PDN(drv, true);
	si5351c_regs_commit(drv);
}

/*
 * Register 183: Crystal Internal Load Capacitance
 * Reads as 0xE4 on power-up
 * Set to 8pF based on crystal specs and HackRF One testing
 */
void si5351c_set_crystal_configuration(si5351c_driver_t* const drv)
{
	set_XTAL_CL(drv, SI5351C_XTAL_8PF);
	si5351c_regs_commit(drv);
}

/*
 * Register 187: Fanout Enable
 * Turn on XO and MultiSynth fanout only.
 */
void si5351c_enable_xo_and_ms_fanout(si5351c_driver_t* const drv)
{
	set_CLKIN_FANOUT_EN(drv, true);
	set_XO_FANOUT_EN(drv, true);
	set_MS_FANOUT_EN(drv, true);
	si5351c_regs_commit(drv);
}

/*
 * Register 15: PLL Input Source
 * CLKIN_DIV=0 (Divide by 1)
 * Set both PLLA_SRC and PLLB_SRC
 */
void si5351c_configure_pll_sources(
	si5351c_driver_t* const drv,
	const enum pll_sources source)
{
	set_CLKIN_DIV(drv, SI5351C_DIV_1);
	set_PLLA_SRC(drv, source);
	set_PLLB_SRC(drv, source);
	si5351c_regs_commit(drv);
}

/* MultiSynth NA (PLLA) and NB (PLLB) */
void si5351c_configure_pll_multisynth(
	si5351c_driver_t* const drv,
	const enum pll_sources source)
{
	for (si5351c_pll_t pll = SI5351C_PLL_A; pll <= SI5351C_PLL_B; pll++) {
		if (source == PLL_SOURCE_CLKIN) {
			/* CLKIN: 10 MHz * (0x2600 + 512) / 128 = 800 MHz, integer mode */
			set_MSN_P1(drv, pll, 0x2600);
		} else {
			/* XTAL: 25 MHz * (0x0e00 + 512) / 128 = 800 MHz, integer mode */
			set_MSN_P1(drv, pll, 0x0E00);
		}
		set_MSN_P2(drv, pll, 0);
		set_MSN_P3(drv, pll, 1);
	}
	si5351c_regs_commit(drv);
}

void si5351c_reset_plls(si5351c_driver_t* const drv, si5351c_pll_mask_t mask)
{
	si5351c_disable_pll_outputs(drv, mask);
	set_PLLA_RST(drv, (mask & SI5351C_PLL_MASK_A) ? true : false);
	set_PLLB_RST(drv, (mask & SI5351C_PLL_MASK_B) ? true : false);
	si5351c_regs_commit(drv);
	delay_us_at_mhz(2000, 204);
	si5351c_enable_clock_outputs(drv);
}

void si5351c_configure_multisynth(
	si5351c_driver_t* const drv,
	const uint_fast8_t ms_number,
	const uint32_t p1,
	const uint32_t p2,
	const uint32_t p3,
	const uint_fast8_t r_div)
{
	/*
	 * TODO: Check for p3 > 0? 0 has no meaning in fractional mode?
	 * And it makes for more jitter in integer mode.
	 */
	/*
	 * r is the r divider value encoded:
	 *   0 means divide by 1
	 *   1 means divide by 2
	 *   2 means divide by 4
	 *   ...
	 *   7 means divide by 128
	 */
	set_MS_P1(drv, ms_number, p1);
	set_MS_P2(drv, ms_number, p2);
	set_MS_P3(drv, ms_number, p3);
	set_R_DIV(drv, ms_number, r_div);
	si5351c_regs_commit(drv);
}

void si5351c_configure_clock_control(si5351c_driver_t* const drv)
{
	si5351c_clk_ctrl_t clkout = {
		.mode = SI5351C_MODE_INT,
		.pll = SI5351C_PLL_A,
		.source = SI5351C_SRC_MULTISYNTH_SELF,
		.drive = SI5351C_DRIVE_8MA,
	};

	si5351c_clk_ctrl_t powered_down = {
		.power_down = true,
		.mode = SI5351C_MODE_INT,
	};

	if (!clkout_enabled) {
		clkout = powered_down;
	}

	si5351c_clk_ctrl_t clk[8];

	/* Clock to CPU is deactivated as it is not used and creates noise */
	/* External clock output is kept in current state */

	/* CLK0: MAX5864/CPLD */
	clk[0] = (si5351c_clk_ctrl_t){
		.mode = SI5351C_MODE_FRAC,
		.pll = SI5351C_PLL_A,
		.source = SI5351C_SRC_MULTISYNTH_SELF,
		.drive = SI5351C_DRIVE_8MA,
	};
	/* CLK1: CPLD */
	clk[1] = (si5351c_clk_ctrl_t){
		.mode = SI5351C_MODE_INT,
		.pll = SI5351C_PLL_A,
		.source = SI5351C_SRC_MULTISYNTH_0_4,
		.drive = SI5351C_DRIVE_2MA,
		.invert = true,
	};
	/* CLK2: SGPIO */
	clk[2] = (si5351c_clk_ctrl_t){
		.mode = SI5351C_MODE_INT,
		.pll = SI5351C_PLL_A,
		.source = SI5351C_SRC_MULTISYNTH_0_4,
		.drive = SI5351C_DRIVE_2MA,
	};
	/* CLK3: CLKOUT */
	clk[3] = clkout;
	/* CLK4: RFFC5072 (MAX2837 on rad1o) */
	clk[4] = (si5351c_clk_ctrl_t){
		.mode = SI5351C_MODE_INT,
		.pll = SI5351C_PLL_A,
		.source = SI5351C_SRC_MULTISYNTH_SELF,
		.drive = SI5351C_DRIVE_6MA,
		.invert = true,
	};
	/* CLK5: MAX2837 (MAX2871 on rad1o) */
	clk[5] = (si5351c_clk_ctrl_t){
		.mode = SI5351C_MODE_INT,
		.pll = SI5351C_PLL_A,
		.source = SI5351C_SRC_MULTISYNTH_SELF,
		.drive = SI5351C_DRIVE_6MA,
		.invert = true,
	};
	/* CLK6: none */
	clk[6] = powered_down;
	/* CLK7: LPC43xx */
	clk[7] = powered_down;

#ifdef IS_H1_R9
	if (IS_H1_R9) {
		/* CLK0: MAX5864/CPLD/SGPIO (sample clocks) */
		clk[0] = (si5351c_clk_ctrl_t){
			.mode = SI5351C_MODE_INT,
			.pll = SI5351C_PLL_A,
			.source = SI5351C_SRC_MULTISYNTH_SELF,
			.drive = SI5351C_DRIVE_6MA,
		};
		/* CLK1: RFFC5072/MAX2839 */
		clk[1] = (si5351c_clk_ctrl_t){
			.mode = SI5351C_MODE_FRAC,
			.pll = SI5351C_PLL_A,
			.source = SI5351C_SRC_MULTISYNTH_SELF,
			.drive = SI5351C_DRIVE_4MA,
		};
		/* CLK2: CLKOUT/LPC4320 */
		clk[2] = clkout;
		/* Other outputs not present on Si5351A */
		clk[3] = powered_down;
		clk[4] = powered_down;
		clk[5] = powered_down;
		clk[6] = powered_down;
		clk[7] = powered_down;
	}
#endif
#ifdef IS_PRALINE
	if (IS_PRALINE) {
		/* CLK0: AFE_CLK */
		clk[0] = (si5351c_clk_ctrl_t){
			.mode = SI5351C_MODE_FRAC,
			.pll = SI5351C_PLL_A,
			.source = SI5351C_SRC_MULTISYNTH_SELF,
			.drive = SI5351C_DRIVE_4MA,
		};
		/* CLK1: SCT_CLK and FPGA_CLK */
		clk[1] = (si5351c_clk_ctrl_t){
			.mode = SI5351C_MODE_FRAC,
			.pll = SI5351C_PLL_A,
			.source = SI5351C_SRC_MULTISYNTH_SELF,
			.drive = SI5351C_DRIVE_2MA,
		};
		/* CLK3: CLKOUT */
		clkout.pll = SI5351C_PLL_B;
		clk[3] = clkout;
		/* CLK4: XCVR_CLK */
		clk[4] = (si5351c_clk_ctrl_t){
			.mode = SI5351C_MODE_INT,
			.pll = SI5351C_PLL_B,
			.source = SI5351C_SRC_MULTISYNTH_SELF,
			.drive = SI5351C_DRIVE_4MA,
			.invert = true,
		};
		/* CLK5: MIX_CLK */
		clk[5] = (si5351c_clk_ctrl_t){
			.mode = SI5351C_MODE_INT,
			.pll = SI5351C_PLL_B,
			.source = SI5351C_SRC_MULTISYNTH_SELF,
			.drive = SI5351C_DRIVE_4MA,
		};
		if ((detected_revision() & ~BOARD_REV_GSG) < BOARD_REV_PRALINE_R1_1) {
			/* CLK2: FPGA_CLK (not shared with SCT_CLK on older boards) */
			clk[2] = (si5351c_clk_ctrl_t){
				.mode = SI5351C_MODE_FRAC,
				.pll = SI5351C_PLL_A,
				.source = SI5351C_SRC_MULTISYNTH_SELF,
				.drive = SI5351C_DRIVE_2MA,
			};
		} else {
			/* CLK2: MCU_CLK */
			clk[2] = powered_down;
		}
	}
#endif

	for (int i = 0; i < 8; i++) {
		set_CLK_PDN(drv, i, clk[i].power_down);
		set_MS_INT(drv, i, clk[i].mode);
		set_MS_SRC(drv, i, clk[i].pll);
		set_CLK_SRC(drv, i, clk[i].source);
		set_CLK_IDRV(drv, i, clk[i].drive);
		set_CLK_INV(drv, i, clk[i].invert);
	}

	si5351c_regs_commit(drv);
}

void si5351c_enable_clock_outputs(si5351c_driver_t* const drv)
{
	/* Enable CLK outputs 0, 1, 2, 4, 5 only. */
	/* Praline: enable 0, 1, 4, 5 only. */
	/* 7: Clock to CPU is deactivated as it is not used and creates noise */
	/* 3: External clock output is deactivated by default */

	uint8_t clkout = 3;

	bool enable[8];

#ifdef IS_PRALINE
	if (IS_PRALINE) {
		enable[0] = true;
		enable[1] = true;
		if ((detected_revision() & ~BOARD_REV_GSG) < BOARD_REV_PRALINE_R1_1) {
			/* CLK2: FPGA_CLK (not shared with SCT_CLK on older boards) */
			enable[2] = true;
		} else {
			enable[2] = false;
		}
		enable[3] = false;
		enable[4] = true;
		enable[5] = true;
		enable[6] = false;
		enable[7] = false;
	}
#endif
#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
		enable[0] = true;
		enable[1] = true;
		enable[2] = true;
		enable[3] = false;
		enable[4] = true;
		enable[5] = true;
		enable[6] = false;
		enable[7] = false;

		/* HackRF One r9 has only three clock generator outputs. */
	#ifdef IS_H1_R9
		if (IS_H1_R9) {
			clkout = 2;
			enable[0] = true;
			enable[1] = true;
			enable[3] = false;
			enable[4] = false;
			enable[5] = false;
			enable[6] = false;
			enable[7] = false;
		}
	#endif
	}
#endif
	enable[clkout] = clkout_enabled;

	for (int i = 0; i < 8; i++) {
		set_CLK_OEB(
			drv,
			i,
			enable[i] ? SI5351C_OUTPUT_ENABLE : SI5351C_OUTPUT_DISABLE);
	}
	si5351c_regs_commit(drv);

#ifdef IS_H1_R9
	if (IS_H1_R9) {
		const platform_gpio_t* gpio = platform_gpio();
		if (clkout_enabled) {
			gpio_set(gpio->h1r9_clkout_en);
		} else {
			gpio_clear(gpio->h1r9_clkout_en);
		}
	}
#endif
}

void si5351c_set_int_mode(
	si5351c_driver_t* const drv,
	const uint_fast8_t ms_number,
	const uint_fast8_t on)
{
	set_MS_INT(drv, ms_number, on);
	si5351c_regs_commit(drv);
}

void si5351c_set_clock_source(si5351c_driver_t* const drv, const enum pll_sources source)
{
	if (source == active_clock_source) {
		return;
	}
	si5351c_disable_all_outputs(drv);
#ifdef IS_H1_R9
	if (IS_H1_R9) {
		/*
		 * HackRF One r9 always uses PLL A on the XTAL input
		 * but externally switches that input to CLKIN.
		 */
		si5351c_configure_pll_sources(drv, PLL_SOURCE_XTAL);
		if (source == PLL_SOURCE_CLKIN) {
			gpio_set(platform_gpio()->h1r9_clkin_en);
		} else {
			gpio_clear(platform_gpio()->h1r9_clkin_en);
		}
	}
#endif
#ifdef IS_NOT_H1_R9
	if (IS_NOT_H1_R9) {
		si5351c_configure_pll_sources(drv, source);
	}
#endif
	si5351c_configure_pll_multisynth(drv, source);
	active_clock_source = source;
	si5351c_reset_plls(drv, SI5351C_PLL_MASK_BOTH);
}

bool si5351c_clkin_signal_valid(si5351c_driver_t* const drv)
{
	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
		uint32_t f = clkin_frequency();
		return (f > 9000000) && (f < 11000000);
	} else {
		si5351c_read_single(drv, LOS_CLKIN);
		bool los_clkin = get_LOS_CLKIN(drv);
		return !los_clkin;
	}
}

void si5351c_clkout_enable(si5351c_driver_t* const drv, uint8_t enable)
{
	clkout_enabled = (enable > 0);

	uint8_t clkout = 3;

	/* HackRF One r9 has only three clock generator outputs. */
	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
		clkout = 2;
	}
	/* Configure clock to 10MHz */
	si5351c_configure_multisynth(drv, clkout, 80 * 128 - 512, 0, 1, 0);

	si5351c_configure_clock_control(drv);
	si5351c_enable_clock_outputs(drv);
}

void si5351c_init(si5351c_driver_t* const drv)
{
	/* Read revision ID */
	si5351c_read_single(drv, REVID);
	selftest.si5351_rev_id = get_REVID(drv);

	/* Read back interrupt status mask register, flip the mask bits and verify. */
	uint8_t int_mask = si5351c_read_single(drv, 2);
	int_mask ^= 0xF8;
	si5351c_write_single(drv, 2, int_mask);
	selftest.si5351_readback_ok = (si5351c_read_single(drv, 2) == int_mask);
	if (!selftest.si5351_readback_ok) {
		selftest.report.pass = false;
	}

	/* Do the same with them flipped back. */
	int_mask ^= 0xF8;
	si5351c_write_single(drv, 2, int_mask);
	selftest.si5351_readback_ok &= (si5351c_read_single(drv, 2) == int_mask);
	if (!selftest.si5351_readback_ok) {
		selftest.report.pass = false;
	}

	/* Cache all current register values. */
	uint8_t data_tx[] = {0};
	i2c_bus_transfer(
		drv->bus,
		drv->i2c_address,
		data_tx,
		1,
		drv->regs,
		sizeof(drv->regs));
	memset(drv->regs_dirty, 0, sizeof(drv->regs_dirty));

#ifdef IS_H1_R9
	if (IS_H1_R9) {
		const platform_gpio_t* gpio = platform_gpio();
		const platform_scu_t* scu = platform_scu();

		/* CLKIN_EN */
		scu_pinmux(scu->H1R9_CLKIN_EN, SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		gpio_clear(gpio->h1r9_clkin_en);
		gpio_output(gpio->h1r9_clkin_en);

		/* CLKOUT_EN */
		scu_pinmux(scu->H1R9_CLKOUT_EN, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		gpio_clear(gpio->h1r9_clkout_en);
		gpio_output(gpio->h1r9_clkout_en);

		/* MCU_CLK_EN */
		scu_pinmux(scu->H1R9_MCU_CLK_EN, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		gpio_clear(gpio->h1r9_mcu_clk_en);
		gpio_output(gpio->h1r9_mcu_clk_en);
	}
#endif
}

/*
 * Set initial phase offset of output multisynth. AN619 associates this setting
 * with outputs, but it seems to really be a multisynth setting.
 *
 * After changing this setting, you must call si5351c_reset_pll() to
 * synchronize outputs with the new phase offset.
 */
void si5351c_set_phase(
	si5351c_driver_t* const drv,
	const uint8_t ms_number,
	const uint8_t offset)
{
	set_CLK_PHOFF(drv, ms_number, offset);
	si5351c_regs_commit(drv);
}
