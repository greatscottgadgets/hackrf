/*
 * Copyright 2012-2022 Great Scott Gadgets <info@greatscottgadgets.com>
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

#if defined(HACKRF_ONE) || defined(UNIVERSAL)
	#include <libopencm3/lpc43xx/scu.h>
#endif

#include "clkin.h"
#include "delay.h"
#include "platform_detect.h"
#include "selftest.h"
#include "si5351c.h"
#if defined(HACKRF_ONE) || defined(UNIVERSAL)
	#include "gpio.h"
	#include "platform_gpio.h"
	#include "platform_scu.h"
#endif

static enum pll_sources active_clock_source = PLL_SOURCE_UNINITIALIZED;
/* External clock output default is deactivated as it creates noise */
static bool clkout_enabled = false;
static uint8_t outputs_disabled = 0xff;

/* write to single register */
void si5351c_write_single(si5351c_driver_t* const drv, uint8_t reg, uint8_t val)
{
	const uint8_t data_tx[] = {reg, val};
	si5351c_write(drv, data_tx, 2);
}

/* read single register */
uint8_t si5351c_read_single(si5351c_driver_t* const drv, uint8_t reg)
{
	const uint8_t data_tx[] = {reg};
	uint8_t data_rx[] = {0x00};
	i2c_bus_transfer(drv->bus, drv->i2c_address, data_tx, 1, data_rx, 1);
	return data_rx[0];
}

/*
 * Write to one or more contiguous registers. data[0] should be the first
 * register number, one or more values follow.
 */
void si5351c_write(
	si5351c_driver_t* const drv,
	const uint8_t* const data,
	const size_t data_count)
{
	i2c_bus_transfer(drv->bus, drv->i2c_address, data, data_count, NULL, 0);
}

/* Disable all CLKx outputs. */
void si5351c_disable_all_outputs(si5351c_driver_t* const drv)
{
	outputs_disabled = 0xff;
	uint8_t data[] = {3, outputs_disabled};
	si5351c_write(drv, data, sizeof(data));
}

/* Disable all CLKx outputs using selected PLL. */
void si5351c_disable_pll_outputs(si5351c_driver_t* const drv, si5351c_pll_t pll)
{
	/*
	 * Bitmask defines outputs using PLL B. Other outputs are assumed to
	 * use PLL A.
	 */
	uint8_t pllb_outputs = 0x00;
	if (detected_platform() == BOARD_ID_PRALINE) {
		pllb_outputs = 0x30;
	}

	if (pll & SI5351C_PLL_A) {
		outputs_disabled |= ~pllb_outputs;
	}
	if (pll & SI5351C_PLL_B) {
		outputs_disabled |= pllb_outputs;
	}
	uint8_t data[] = {3, outputs_disabled};
	si5351c_write(drv, data, sizeof(data));
}

/* Turn off OEB pin control for all CLKx */
void si5351c_disable_oeb_pin_control(si5351c_driver_t* const drv)
{
	uint8_t data[] = {9, 0xFF};
	si5351c_write(drv, data, sizeof(data));
}

/* Power down all CLKx */
void si5351c_power_down_all_clocks(si5351c_driver_t* const drv)
{
	uint8_t data[] = {
		16,
		SI5351C_CLK_POWERDOWN,
		SI5351C_CLK_POWERDOWN,
		SI5351C_CLK_POWERDOWN,
		SI5351C_CLK_POWERDOWN,
		SI5351C_CLK_POWERDOWN,
		SI5351C_CLK_POWERDOWN,
		SI5351C_CLK_POWERDOWN | SI5351C_CLK_INT_MODE,
		SI5351C_CLK_POWERDOWN | SI5351C_CLK_INT_MODE};
	si5351c_write(drv, data, sizeof(data));
}

/*
 * Register 183: Crystal Internal Load Capacitance
 * Reads as 0xE4 on power-up
 * Set to 8pF based on crystal specs and HackRF One testing
 */
void si5351c_set_crystal_configuration(si5351c_driver_t* const drv)
{
	uint8_t data[] = {183, 0x80};
	si5351c_write(drv, data, sizeof(data));
}

/*
 * Register 187: Fanout Enable
 * Turn on XO and MultiSynth fanout only.
 */
void si5351c_enable_xo_and_ms_fanout(si5351c_driver_t* const drv)
{
	uint8_t data[] = {187, 0xD0};
	si5351c_write(drv, data, sizeof(data));
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
	uint8_t data[] = {15, 0x00};

	if (source == PLL_SOURCE_CLKIN) {
		data[1] = 0x0c;
	}
	si5351c_write(drv, data, sizeof(data));
}

/* MultiSynth NA (PLLA) and NB (PLLB) */
void si5351c_configure_pll_multisynth(
	si5351c_driver_t* const drv,
	const enum pll_sources source)
{
	/* XTAL: 25 MHz * (0x0e00 + 512) / 128 = 800 MHz, integer mode */
	uint8_t data[] = {26, 0x00, 0x01, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00};
	if (source == PLL_SOURCE_CLKIN) {
		/* CLKIN: 10 MHz * (0x2600 + 512) / 128 = 800 MHz, integer mode */
		data[4] = 0x26;
	}
	si5351c_write(drv, data, sizeof(data));

	/* Apply same configuration to PLL B. */
	data[0] = 34;
	si5351c_write(drv, data, sizeof(data));
}

void si5351c_reset_pll(si5351c_driver_t* const drv, si5351c_pll_t pll)
{
	uint8_t value = 0;

	if (pll & SI5351C_PLL_A) {
		value |= 0x20;
	}
	if (pll & SI5351C_PLL_B) {
		value |= 0x80;
	}

	si5351c_disable_pll_outputs(drv, pll);
	uint8_t data[] = {177, value};
	si5351c_write(drv, data, sizeof(data));
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
	const uint_fast8_t register_number = 42 + (ms_number * 8);
	uint8_t data[] = {
		register_number,
		(p3 >> 8) & 0xFF,
		(p3 >> 0) & 0xFF,
		(r_div << 4) | (0 << 2) | ((p1 >> 16) & 0x3),
		(p1 >> 8) & 0xFF,
		(p1 >> 0) & 0xFF,
		(((p3 >> 16) & 0xF) << 4) | (((p2 >> 16) & 0xF) << 0),
		(p2 >> 8) & 0xFF,
		(p2 >> 0) & 0xFF};
	si5351c_write(drv, data, sizeof(data));
}

void si5351c_configure_clock_control(si5351c_driver_t* const drv)
{
	uint8_t clkout_ctrl;
	board_id_t board_id = detected_platform();

	if (clkout_enabled) {
		clkout_ctrl = SI5351C_CLK_INT_MODE | SI5351C_CLK_PLL_SRC(SI5351C_PLL_A) |
			SI5351C_CLK_SRC(SI5351C_CLK_SRC_MULTISYNTH_SELF) |
			SI5351C_CLK_IDRV(SI5351C_CLK_IDRV_8MA);
	} else {
		clkout_ctrl = SI5351C_CLK_POWERDOWN | SI5351C_CLK_INT_MODE;
	}

	/* Clock to CPU is deactivated as it is not used and creates noise */
	/* External clock output is kept in current state */
	uint8_t data[] = {
		16,
		SI5351C_CLK_FRAC_MODE | SI5351C_CLK_PLL_SRC(SI5351C_PLL_A) |
			SI5351C_CLK_SRC(SI5351C_CLK_SRC_MULTISYNTH_SELF) |
			SI5351C_CLK_IDRV(SI5351C_CLK_IDRV_8MA),
		SI5351C_CLK_INT_MODE | SI5351C_CLK_PLL_SRC(SI5351C_PLL_A) |
			SI5351C_CLK_SRC(SI5351C_CLK_SRC_MULTISYNTH_0_4) |
			SI5351C_CLK_IDRV(SI5351C_CLK_IDRV_2MA) | SI5351C_CLK_INV,
		SI5351C_CLK_INT_MODE | SI5351C_CLK_PLL_SRC(SI5351C_PLL_A) |
			SI5351C_CLK_SRC(SI5351C_CLK_SRC_MULTISYNTH_0_4) |
			SI5351C_CLK_IDRV(SI5351C_CLK_IDRV_2MA),
		clkout_ctrl,
		SI5351C_CLK_INT_MODE | SI5351C_CLK_PLL_SRC(SI5351C_PLL_A) |
			SI5351C_CLK_SRC(SI5351C_CLK_SRC_MULTISYNTH_SELF) |
			SI5351C_CLK_IDRV(SI5351C_CLK_IDRV_6MA) | SI5351C_CLK_INV,
		SI5351C_CLK_INT_MODE | SI5351C_CLK_PLL_SRC(SI5351C_PLL_A) |
			SI5351C_CLK_SRC(SI5351C_CLK_SRC_MULTISYNTH_SELF) |
			SI5351C_CLK_IDRV(SI5351C_CLK_IDRV_4MA),
		SI5351C_CLK_POWERDOWN |
			SI5351C_CLK_INT_MODE, /* not connected, but: PLL A int mode */
		SI5351C_CLK_POWERDOWN |
			SI5351C_CLK_INT_MODE /* not connected, but: PLL B int mode */
	};
	if (board_id == BOARD_ID_HACKRF1_R9) {
#if defined(HACKRF_ONE) || defined(UNIVERSAL)
		data[1] = SI5351C_CLK_INT_MODE | SI5351C_CLK_PLL_SRC(SI5351C_PLL_A) |
			SI5351C_CLK_SRC(SI5351C_CLK_SRC_MULTISYNTH_SELF) |
			SI5351C_CLK_IDRV(SI5351C_CLK_IDRV_6MA);
		data[2] = SI5351C_CLK_FRAC_MODE | SI5351C_CLK_PLL_SRC(SI5351C_PLL_A) |
			SI5351C_CLK_SRC(SI5351C_CLK_SRC_MULTISYNTH_SELF) |
			SI5351C_CLK_IDRV(SI5351C_CLK_IDRV_4MA);
		data[3] = clkout_ctrl;
		data[4] = SI5351C_CLK_POWERDOWN;
		data[5] = SI5351C_CLK_POWERDOWN;
		data[6] = SI5351C_CLK_POWERDOWN;
#endif
	}
	if (board_id == BOARD_ID_PRALINE) {
#if defined(PRALINE) || defined(UNIVERSAL)
		/* CLK0: AFE_CLK */
		data[1] = SI5351C_CLK_FRAC_MODE | SI5351C_CLK_PLL_SRC(SI5351C_PLL_A) |
			SI5351C_CLK_SRC(SI5351C_CLK_SRC_MULTISYNTH_SELF) |
			SI5351C_CLK_IDRV(SI5351C_CLK_IDRV_4MA);
		/* CLK1: SCT_CLK and FPGA_CLK */
		data[2] = SI5351C_CLK_FRAC_MODE | SI5351C_CLK_PLL_SRC(SI5351C_PLL_A) |
			SI5351C_CLK_SRC(SI5351C_CLK_SRC_MULTISYNTH_SELF) |
			SI5351C_CLK_IDRV(SI5351C_CLK_IDRV_2MA);
		/* CLK3: CLKOUT */
		clkout_ctrl = SI5351C_CLK_INT_MODE | SI5351C_CLK_PLL_SRC(SI5351C_PLL_B) |
			SI5351C_CLK_SRC(SI5351C_CLK_SRC_MULTISYNTH_SELF) |
			SI5351C_CLK_IDRV(SI5351C_CLK_IDRV_8MA);
		/* CLK4: XCVR_CLK */
		data[5] = SI5351C_CLK_INT_MODE | SI5351C_CLK_PLL_SRC(SI5351C_PLL_B) |
			SI5351C_CLK_SRC(SI5351C_CLK_SRC_MULTISYNTH_SELF) |
			SI5351C_CLK_IDRV(SI5351C_CLK_IDRV_4MA) | SI5351C_CLK_INV;
		data[6] = SI5351C_CLK_INT_MODE | SI5351C_CLK_PLL_SRC(SI5351C_PLL_B) |
			SI5351C_CLK_SRC(SI5351C_CLK_SRC_MULTISYNTH_SELF) |
			SI5351C_CLK_IDRV(SI5351C_CLK_IDRV_4MA);
		if ((detected_revision() & ~BOARD_REV_GSG) < BOARD_REV_PRALINE_R1_1) {
			/* CLK2: FPGA_CLK (not shared with SCT_CLK on older boards) */
			data[3] = SI5351C_CLK_FRAC_MODE | SI5351C_CLK_PLL_SRC(SI5351C_PLL_A) |
				SI5351C_CLK_SRC(SI5351C_CLK_SRC_MULTISYNTH_SELF) |
				SI5351C_CLK_IDRV(SI5351C_CLK_IDRV_2MA);
		} else {
			/* CLK2: MCU_CLK */
			data[3] = SI5351C_CLK_POWERDOWN;
		}
#endif
	}
	si5351c_write(drv, data, sizeof(data));
}

#define SI5351C_CLK_ENABLE(x)  (0 << x)
#define SI5351C_CLK_DISABLE(x) (1 << x)
#define SI5351C_REG_OUTPUT_EN  (3)

void si5351c_enable_clock_outputs(si5351c_driver_t* const drv)
{
	/* Enable CLK outputs 0, 1, 2, 4, 5 only. */
	/* Praline: enable 0, 1, 4, 5 only. */
	/* 7: Clock to CPU is deactivated as it is not used and creates noise */
	/* 3: External clock output is deactivated by default */

	board_id_t board_id = detected_platform();
	uint8_t clkout = 3;
	uint8_t value;

	if (board_id == BOARD_ID_PRALINE) {
		value = SI5351C_CLK_ENABLE(0) | SI5351C_CLK_ENABLE(1) |
			SI5351C_CLK_ENABLE(4) | SI5351C_CLK_ENABLE(5) |
			SI5351C_CLK_DISABLE(6) | SI5351C_CLK_DISABLE(7);
		if ((detected_revision() & ~BOARD_REV_GSG) < BOARD_REV_PRALINE_R1_1) {
			/* CLK2: FPGA_CLK (not shared with SCT_CLK on older boards) */
			value |= SI5351C_CLK_ENABLE(2);
		} else {
			value |= SI5351C_CLK_DISABLE(2);
		}
	} else {
		value = SI5351C_CLK_ENABLE(0) | SI5351C_CLK_ENABLE(1) |
			SI5351C_CLK_ENABLE(2) | SI5351C_CLK_ENABLE(4) |
			SI5351C_CLK_ENABLE(5) | SI5351C_CLK_DISABLE(6) |
			SI5351C_CLK_DISABLE(7);

		/* HackRF One r9 has only three clock generator outputs. */
		if (board_id == BOARD_ID_HACKRF1_R9) {
			clkout = 2;
			value = SI5351C_CLK_ENABLE(0) | SI5351C_CLK_ENABLE(1) |
				SI5351C_CLK_DISABLE(3) | SI5351C_CLK_DISABLE(4) |
				SI5351C_CLK_DISABLE(5) | SI5351C_CLK_DISABLE(6) |
				SI5351C_CLK_DISABLE(7);
		}
	}

	value |= (clkout_enabled) ? SI5351C_CLK_ENABLE(clkout) :
				    SI5351C_CLK_DISABLE(clkout);
	uint8_t data[] = {SI5351C_REG_OUTPUT_EN, value};
	si5351c_write(drv, data, sizeof(data));
	outputs_disabled = value;

	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
#if defined(HACKRF_ONE) || defined(UNIVERSAL)
		const platform_gpio_t* gpio = platform_gpio();
		if (clkout_enabled) {
			gpio_set(gpio->h1r9_clkout_en);
		} else {
			gpio_clear(gpio->h1r9_clkout_en);
		}
#endif
	}
}

void si5351c_set_int_mode(
	si5351c_driver_t* const drv,
	const uint_fast8_t ms_number,
	const uint_fast8_t on)
{
	uint8_t data[] = {16, 0};

	if (ms_number < 8) {
		data[0] = 16 + ms_number;
		data[1] = si5351c_read_single(drv, data[0]);

		if (on) {
			data[1] |= SI5351C_CLK_INT_MODE;
		} else {
			data[1] &= ~(SI5351C_CLK_INT_MODE);
		}

		si5351c_write(drv, data, 2);
	}
}

void si5351c_set_clock_source(si5351c_driver_t* const drv, const enum pll_sources source)
{
	if (source == active_clock_source) {
		return;
	}
	si5351c_disable_all_outputs(drv);
	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
#if defined(HACKRF_ONE)
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
#endif
	} else {
		si5351c_configure_pll_sources(drv, source);
	}
	si5351c_configure_pll_multisynth(drv, source);
	active_clock_source = source;
	si5351c_reset_pll(drv, SI5351C_PLL_BOTH);
}

bool si5351c_clkin_signal_valid(si5351c_driver_t* const drv)
{
	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
		uint32_t f = clkin_frequency();
		return (f > 9000000) && (f < 11000000);
	} else {
		return (si5351c_read_single(drv, 0) & SI5351C_LOS) == 0;
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
	selftest.si5351_rev_id = si5351c_read_single(drv, 0) & SI5351C_REVID;

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

	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
#if defined(HACKRF_ONE) || defined(UNIVERSAL)
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
#endif
	}
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
	const uint8_t address = 165 + ms_number;
	if (ms_number < 8) {
		si5351c_write_single(drv, address, offset & 0x7f);
	}
}
