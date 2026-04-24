/*
 * Copyright 2012-2026 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2012 Jared Boone <jared@sharebrained.com>
 * Copyright 2013 Benjamin Vernoux <titanmkd@gmail.com>
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

#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/ssp.h>

#include "drivers.h"
#include "gpio.h"
#include "leds.h"
#include "mixer.h"
#include "platform_detect.h"
#include "platform_gpio.h"
#include "platform_scu.h"
#include "power.h"
#include "rf_path.h"
#include "sgpio.h"
#if defined(IS_PRALINE)
	#include "clock_io.h"
#endif

void pin_shutdown(void)
{
	/* Configure all GPIO as Input (safe state) */
	gpio_init();

	/* Detect Platform */
	const platform_gpio_t* gpio = platform_gpio();
	const platform_scu_t* scu = platform_scu();

	/* TDI and TMS pull-ups are required in all JTAG-compliant devices.
	 *
	 * The HackRF CPLD is always present, so let the CPLD pull up its TDI and TMS.
	 *
	 * The PortaPack may not be present, so pull up the PortaPack TMS pin from the
	 * microcontroller.
	 *
	 * TCK is recommended to be held low, so use microcontroller pull-down.
	 *
	 * TDO is undriven except when in Shift-IR or Shift-DR phases.
	 * Use the microcontroller to pull down to keep from floating.
	 *
	 * LPC43xx pull-up and pull-down resistors are approximately 53K.
	 */
#ifdef IS_EXPANSION_COMPATIBLE
	if (IS_EXPANSION_COMPATIBLE) {
		scu_pinmux(scu->PINMUX_PP_TMS, SCU_GPIO_PUP | SCU_CONF_FUNCTION0);
		scu_pinmux(scu->PINMUX_PP_TDO, SCU_GPIO_PDN | SCU_CONF_FUNCTION0);
	}
#endif
	scu_pinmux(scu->PINMUX_CPLD_TCK, SCU_GPIO_PDN | SCU_CONF_FUNCTION0);
#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
		scu_pinmux(scu->PINMUX_CPLD_TMS, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
		scu_pinmux(scu->PINMUX_CPLD_TDI, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
		scu_pinmux(scu->PINMUX_CPLD_TDO, SCU_GPIO_PDN | SCU_CONF_FUNCTION4);
	}
#endif

	/* Configure SCU Pin Mux as GPIO */
	scu_pinmux(scu->PINMUX_LED1, SCU_GPIO_NOPULL);
	scu_pinmux(scu->PINMUX_LED2, SCU_GPIO_NOPULL);
	scu_pinmux(scu->PINMUX_LED3, SCU_GPIO_NOPULL);
#ifdef IS_RAD1O
	if (IS_RAD1O) {
		scu_pinmux(scu->PINMUX_LED4, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION4);
	}
#endif
#ifdef IS_PRALINE
	if (IS_PRALINE) {
		scu_pinmux(scu->PINMUX_LED4, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
	}
#endif

	/* Configure USB indicators */
#ifdef IS_JAWBREAKER
	if (IS_JAWBREAKER) {
		scu_pinmux(scu->PINMUX_USB_LED0, SCU_CONF_FUNCTION3);
		scu_pinmux(scu->PINMUX_USB_LED1, SCU_CONF_FUNCTION3);
	}
#endif

#ifdef IS_PRALINE
	if (IS_PRALINE) {
		disable_1v2_power();
		disable_3v3aux_power();
		gpio_output(gpio->gpio_1v2_enable);
		gpio_output(gpio->gpio_3v3aux_enable_n);
		scu_pinmux(scu->PINMUX_EN1V2, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		scu_pinmux(scu->PINMUX_EN3V3_AUX_N, SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
	}
#endif

#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
		disable_1v8_power();
	#ifdef IS_H1_R9
		if (IS_H1_R9) {
			gpio_output(gpio->h1r9_1v8_enable);
			scu_pinmux(scu->H1R9_EN1V8, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		}
	#endif
	#ifdef IS_NOT_H1_R9
		if (IS_NOT_H1_R9) {
			gpio_output(gpio->gpio_1v8_enable);
			scu_pinmux(scu->PINMUX_EN1V8, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		}
	#endif
	}
#endif

#ifdef IS_H1_OR_PRALINE
	if (IS_H1_OR_PRALINE) {
		/* Safe state: start with VAA turned off: */
		disable_rf_power();

		/* Configure RF power supply (VAA) switch control signal as output */
	#ifdef IS_H1_R9
		if (IS_H1_R9) {
			gpio_output(gpio->h1r9_vaa_disable);
		}
	#endif
	#ifdef IS_NOT_H1_R9
		if (IS_NOT_H1_R9) {
			gpio_output(gpio->vaa_disable);
		}
	#endif
	}
#endif

#ifdef IS_RAD1O
	if (IS_RAD1O) {
		/* Safe state: start with VAA turned off: */
		disable_rf_power();

		/* Configure RF power supply (VAA) switch control signal as output */
		gpio_output(gpio->vaa_enable);

		/* Disable unused clock outputs. They generate noise. */
		scu_pinmux(CLK0, SCU_CLK_IN | SCU_CONF_FUNCTION7);
		scu_pinmux(CLK2, SCU_CLK_IN | SCU_CONF_FUNCTION7);

		scu_pinmux(scu->PINMUX_GPIO3_10, SCU_GPIO_PDN | SCU_CONF_FUNCTION0);
		scu_pinmux(scu->PINMUX_GPIO3_11, SCU_GPIO_PDN | SCU_CONF_FUNCTION0);
	}
#endif

#ifdef IS_PRALINE
	if (IS_PRALINE) {
		scu_pinmux(scu->P2_CTRL0, scu->P2_CTRL0_PINCFG);
		scu_pinmux(scu->P2_CTRL1, scu->P2_CTRL1_PINCFG);
		scu_pinmux(scu->P1_CTRL0, scu->P1_CTRL0_PINCFG);
		scu_pinmux(scu->P1_CTRL1, scu->P1_CTRL1_PINCFG);
		scu_pinmux(scu->P1_CTRL2, scu->P1_CTRL2_PINCFG);
		scu_pinmux(scu->CLKIN_CTRL, scu->CLKIN_CTRL_PINCFG);
		scu_pinmux(scu->AA_EN, scu->AA_EN_PINCFG);
		scu_pinmux(scu->TRIGGER_IN, scu->TRIGGER_IN_PINCFG);
		scu_pinmux(scu->TRIGGER_OUT, scu->TRIGGER_OUT_PINCFG);
		scu_pinmux(scu->PPS_OUT, scu->PPS_OUT_PINCFG);
		scu_pinmux(scu->PINMUX_FPGA_CRESET, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
		scu_pinmux(scu->PINMUX_FPGA_CDONE, SCU_GPIO_PUP | SCU_CONF_FUNCTION4);
		scu_pinmux(scu->PINMUX_FPGA_SPI_CS, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);

		p2_ctrl_set(P2_SIGNAL_CLK3);
		p1_ctrl_set(P1_SIGNAL_CLKIN);
		narrowband_filter_set(0);
		clkin_ctrl_set(CLKIN_SIGNAL_P1);

		gpio_output(gpio->p2_ctrl0);
		gpio_output(gpio->p2_ctrl1);
		gpio_output(gpio->p1_ctrl0);
		gpio_output(gpio->p1_ctrl1);
		gpio_output(gpio->p1_ctrl2);
		gpio_output(gpio->clkin_ctrl);
		gpio_output(gpio->pps_out);
		gpio_output(gpio->aa_en);
		gpio_input(gpio->trigger_in);
		gpio_input(gpio->trigger_out);
		gpio_clear(gpio->fpga_cfg_spi_cs);
		gpio_output(gpio->fpga_cfg_spi_cs);
		gpio_clear(gpio->fpga_cfg_creset);
		gpio_output(gpio->fpga_cfg_creset);
		gpio_input(gpio->fpga_cfg_cdone);
	}
#endif

	/* enable input on SCL and SDA pins */
	SCU_SFSI2C0 = SCU_I2C0_NOMINAL;
}

/* Run after pin_shutdown() and prior to enabling power supplies. */
void pin_setup(void)
{
	/* Detect Platform */
	const platform_gpio_t* gpio = platform_gpio();
	const platform_scu_t* scu = platform_scu();

	/* Configure LEDs */
	led_off(0);
	led_off(1);
	led_off(2);
#ifdef IS_FOUR_LEDS
	if (IS_FOUR_LEDS) {
		led_off(3);
	}
#endif

	gpio_output(gpio->led[0]);
	gpio_output(gpio->led[1]);
	gpio_output(gpio->led[2]);

#ifdef IS_FOUR_LEDS
	if (IS_FOUR_LEDS) {
		gpio_output(gpio->led[3]);
	}
#endif

	/* Configure drivers and driver pins */
	ssp_config_max283x.gpio_select = gpio->max283x_select;
#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
		ssp_config_max283x.data_bits = SSP_DATA_16BITS;
	}
#endif
#ifdef IS_PRALINE
	if (IS_PRALINE) {
		ssp_config_max283x.data_bits = SSP_DATA_9BITS; // send 2 words
	}
#endif

	ssp_config_max5864.gpio_select = gpio->max5864_select;

	ssp_config_w25q80bv.gpio_select = gpio->w25q80bv_select;
	spi_flash.gpio_hold = gpio->w25q80bv_hold;
	spi_flash.gpio_wp = gpio->w25q80bv_wp;

	sgpio_config.gpio_q_invert = gpio->q_invert;

#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
		sgpio_config.gpio_trigger_enable = gpio->trigger_enable;
	}
#endif

#ifdef IS_PRALINE
	if (IS_PRALINE) {
		ssp_config_ice40_fpga.gpio_select = gpio->fpga_cfg_spi_cs;
		ice40.gpio_select = gpio->fpga_cfg_spi_cs;
		ice40.gpio_creset = gpio->fpga_cfg_creset;
		ice40.gpio_cdone = gpio->fpga_cfg_cdone;
	}
#endif

	jtag_gpio_cpld.gpio_tck = gpio->cpld_tck;

#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
		jtag_gpio_cpld.gpio_tms = gpio->cpld_tms;
		jtag_gpio_cpld.gpio_tdi = gpio->cpld_tdi;
		jtag_gpio_cpld.gpio_tdo = gpio->cpld_tdo;
	}
#endif

#ifdef IS_EXPANSION_COMPATIBLE
	if (IS_EXPANSION_COMPATIBLE) {
		jtag_gpio_cpld.gpio_pp_tms = gpio->cpld_pp_tms;
		jtag_gpio_cpld.gpio_pp_tdo = gpio->cpld_pp_tdo;
	}
#endif

	ssp1_set_mode_max283x();

	mixer_bus_setup(&mixer);

#ifdef IS_H1_R9
	if (IS_H1_R9) {
		sgpio_config.gpio_trigger_enable = gpio->h1r9_trigger_enable;
	}
#endif

	// initialize rf_path struct and assign gpio's
#ifdef IS_HACKRF_ONE
	if (IS_HACKRF_ONE) {
		rf_path = (rf_path_t){
			.switchctrl = 0,
			.gpio_hp = gpio->hp,
			.gpio_lp = gpio->lp,
			.gpio_tx_mix_bp = gpio->tx_mix_bp,
			.gpio_no_mix_bypass = gpio->no_mix_bypass,
			.gpio_rx_mix_bp = gpio->rx_mix_bp,
			.gpio_tx_amp = gpio->tx_amp,
			.gpio_tx = gpio->tx,
			.gpio_mix_bypass = gpio->mix_bypass,
			.gpio_rx = gpio->rx,
			.gpio_no_tx_amp_pwr = gpio->no_tx_amp_pwr,
			.gpio_amp_bypass = gpio->amp_bypass,
			.gpio_rx_amp = gpio->rx_amp,
			.gpio_no_rx_amp_pwr = gpio->no_rx_amp_pwr,
		};
	#ifdef IS_H1_R9
		if (IS_H1_R9) {
			rf_path.gpio_rx = gpio->h1r9_rx;
			rf_path.gpio_h1r9_no_ant_pwr = gpio->h1r9_no_ant_pwr;
		}
	#endif
	}
#endif

#ifdef IS_RAD1O
	if (IS_RAD1O) {
		rf_path = (rf_path_t){
			.switchctrl = 0,
			.gpio_tx_rx_n = gpio->tx_rx_n,
			.gpio_tx_rx = gpio->tx_rx,
			.gpio_by_mix = gpio->by_mix,
			.gpio_by_mix_n = gpio->by_mix_n,
			.gpio_by_amp = gpio->by_amp,
			.gpio_by_amp_n = gpio->by_amp_n,
			.gpio_mixer_en = gpio->mixer_en,
			.gpio_low_high_filt = gpio->low_high_filt,
			.gpio_low_high_filt_n = gpio->low_high_filt_n,
			.gpio_tx_amp = gpio->tx_amp,
			.gpio_rx_lna = gpio->rx_lna,
		};
	}
#endif

#ifdef IS_PRALINE
	if (IS_PRALINE) {
		rf_path = (rf_path_t){
			.switchctrl = 0,
			.gpio_tx_en = gpio->tx_en,
			.gpio_mix_en_n = gpio->mix_en_n,
			.gpio_lpf_en = gpio->lpf_en,
			.gpio_rf_amp_en = gpio->rf_amp_en,
			.gpio_ant_bias_en_n = gpio->ant_bias_en_n,
		};
		if ((detected_revision() == BOARD_REV_PRALINE_R1_0) ||
		    (detected_revision() == BOARD_REV_GSG_PRALINE_R1_0)) {
			rf_path.gpio_mix_en_n = gpio->mix_en_n_r1_0;
		}
	}
#endif

	rf_path_pin_setup(&rf_path);

	/* Configure external clock in */
	scu_pinmux(scu->PINMUX_GP_CLKIN, SCU_CLK_IN | SCU_CONF_FUNCTION1);

	sgpio_configure_pin_functions(&sgpio_config);
}
