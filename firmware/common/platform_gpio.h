/*
 * Copyright 2012-2026 Great Scott Gadgets <info@greatscottgadgets.com>
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

#ifndef __PLATFORM_GPIO_H
#define __PLATFORM_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gpio_lpc.h"
#include "platform_detect.h"

typedef struct {
	/* LEDs */
	gpio_t led[4];

	/* Power Supply Control */
	gpio_t gpio_1v8_enable;
#if defined(PRALINE) || defined(UNIVERSAL)
	gpio_t gpio_1v2_enable;
	gpio_t gpio_3v3aux_enable_n;
#endif

	/* MAX283x GPIO (XCVR_CTL / CS_XCVR) PinMux */
	gpio_t max283x_select;
	gpio_t max283x_enable;
	gpio_t max283x_rx_enable;
	gpio_t max283x_tx_enable;
#if defined(PRALINE) || defined(UNIVERSAL)
	gpio_t max2831_rxhp;
	gpio_t max2831_ld;
#endif

	/* MAX5864 SPI chip select (AD_CS / CS_AD) GPIO PinMux */
	gpio_t max5864_select;

	/* RF supply (VAA) control */
	gpio_t vaa_disable;
#if defined(RAD1O)
	gpio_t vaa_enable;
#endif

	/* W25Q80BV Flash */
	gpio_t w25q80bv_hold;
	gpio_t w25q80bv_wp;
	gpio_t w25q80bv_select;

	/* RF switch control */
#if defined(HACKRF_ONE) || defined(UNIVERSAL)
	gpio_t hp;
	gpio_t lp;
	gpio_t tx_mix_bp;
	gpio_t no_mix_bypass;
	gpio_t rx_mix_bp;
	gpio_t tx_amp;
	gpio_t tx;
	gpio_t mix_bypass;
	gpio_t rx;
	gpio_t no_tx_amp_pwr;
	gpio_t amp_bypass;
	gpio_t rx_amp;
	gpio_t no_rx_amp_pwr;
	gpio_t h1r9_no_ant_pwr; // HACKRF_ONE r9
	gpio_t h1r9_rx;         // HACKRF_ONE r9
#endif
#if defined(RAD1O)
	gpio_t tx_rx_n;
	gpio_t tx_rx;
	gpio_t by_mix;
	gpio_t by_mix_n;
	gpio_t by_amp;
	gpio_t by_amp_n;
	gpio_t mixer_en;
	gpio_t low_high_filt;
	gpio_t low_high_filt_n;
	gpio_t tx_amp;
	gpio_t rx_lna;
#endif
#if defined(PRALINE) || defined(UNIVERSAL)
	gpio_t tx_en;
	gpio_t mix_en_n;
	gpio_t mix_en_n_r1_0;
	gpio_t lpf_en;
	gpio_t rf_amp_en;
	gpio_t ant_bias_en_n;
#endif

	/* CPLD JTAG interface GPIO pins, FPGA config pins in Praline */
	gpio_t cpld_tck;
	gpio_t cpld_tdo;
	gpio_t cpld_tms;
	gpio_t cpld_tdi;
#if defined(HACKRF_ONE) || defined(PRALINE) || defined(UNIVERSAL)
	gpio_t cpld_pp_tms;
	gpio_t cpld_pp_tdo;
#endif
#if defined(PRALINE) || defined(UNIVERSAL)
	gpio_t fpga_cfg_creset;
	gpio_t fpga_cfg_cdone;
	gpio_t fpga_cfg_spi_cs;
#endif

	/* Other CPLD interface GPIO pins */
#if !defined(PRALINE) || defined(UNIVERSAL)
	gpio_t trigger_enable;
#endif
	gpio_t q_invert;

	/* RFFC5071 GPIO serial interface PinMux */
	gpio_t rffc5072_select;
	gpio_t rffc5072_clock;
	gpio_t rffc5072_data;
	gpio_t rffc5072_reset;
	gpio_t rffc5072_ld;   // PRALINE
	gpio_t vco_ce;        // RAD1O
	gpio_t vco_sclk;      // RAD1O
	gpio_t vco_sdata;     // RAD1O
	gpio_t vco_le;        // RAD1O
	gpio_t vco_mux;       // RAD1O
	gpio_t synt_rfout_en; // RAD1O

#if defined(HACKRF_ONE) || defined(UNIVERSAL)
	/* HackRF One r9 clock control */
	gpio_t h1r9_clkin_en;
	gpio_t h1r9_clkout_en;
	gpio_t h1r9_mcu_clk_en;

	gpio_t h1r9_1v8_enable;
	gpio_t h1r9_vaa_disable;
	gpio_t h1r9_trigger_enable;
#endif

#if defined(PRALINE) || defined(UNIVERSAL)
	gpio_t p2_ctrl0;
	gpio_t p2_ctrl1;
	gpio_t p1_ctrl0;
	gpio_t p1_ctrl1;
	gpio_t p1_ctrl2;
	gpio_t clkin_ctrl;
	gpio_t aa_en;
	gpio_t trigger_in;
	gpio_t trigger_out;
	gpio_t pps_out;
#endif
} platform_gpio_t;

// Detects and returns the global platform gpio instance of the active board id and revision.
const platform_gpio_t* platform_gpio(void);

/* LPC43xx GPIO pre-declarations - we use these instead of the GPIO()
   macro so that we can assign them at runtime. */
static const struct gpio GPIO0_0 = GPIO(0, 0);
static const struct gpio GPIO0_1 = GPIO(0, 1);
static const struct gpio GPIO0_2 = GPIO(0, 2);
static const struct gpio GPIO0_3 = GPIO(0, 3);
static const struct gpio GPIO0_4 = GPIO(0, 4);
static const struct gpio GPIO0_5 = GPIO(0, 5);
static const struct gpio GPIO0_6 = GPIO(0, 6);
static const struct gpio GPIO0_7 = GPIO(0, 7);
static const struct gpio GPIO0_8 = GPIO(0, 8);
static const struct gpio GPIO0_9 = GPIO(0, 9);
static const struct gpio GPIO0_10 = GPIO(0, 10);
static const struct gpio GPIO0_11 = GPIO(0, 11);
static const struct gpio GPIO0_12 = GPIO(0, 12);
static const struct gpio GPIO0_13 = GPIO(0, 13);
static const struct gpio GPIO0_14 = GPIO(0, 14);
static const struct gpio GPIO0_15 = GPIO(0, 15);

static const struct gpio GPIO1_0 = GPIO(1, 0);
static const struct gpio GPIO1_1 = GPIO(1, 1);
static const struct gpio GPIO1_2 = GPIO(1, 2);
static const struct gpio GPIO1_3 = GPIO(1, 3);
static const struct gpio GPIO1_4 = GPIO(1, 4);
static const struct gpio GPIO1_5 = GPIO(1, 5);
static const struct gpio GPIO1_6 = GPIO(1, 6);
static const struct gpio GPIO1_7 = GPIO(1, 7);
static const struct gpio GPIO1_8 = GPIO(1, 8);
static const struct gpio GPIO1_9 = GPIO(1, 9);
static const struct gpio GPIO1_10 = GPIO(1, 10);
static const struct gpio GPIO1_11 = GPIO(1, 11);
static const struct gpio GPIO1_12 = GPIO(1, 12);
static const struct gpio GPIO1_13 = GPIO(1, 13);
static const struct gpio GPIO1_14 = GPIO(1, 14);
static const struct gpio GPIO1_15 = GPIO(1, 15);

static const struct gpio GPIO2_0 = GPIO(2, 0);
static const struct gpio GPIO2_1 = GPIO(2, 1);
static const struct gpio GPIO2_2 = GPIO(2, 2);
static const struct gpio GPIO2_3 = GPIO(2, 3);
static const struct gpio GPIO2_4 = GPIO(2, 4);
static const struct gpio GPIO2_5 = GPIO(2, 5);
static const struct gpio GPIO2_6 = GPIO(2, 6);
static const struct gpio GPIO2_7 = GPIO(2, 7);
static const struct gpio GPIO2_8 = GPIO(2, 8);
static const struct gpio GPIO2_9 = GPIO(2, 9);
static const struct gpio GPIO2_10 = GPIO(2, 10);
static const struct gpio GPIO2_11 = GPIO(2, 11);
static const struct gpio GPIO2_12 = GPIO(2, 12);
static const struct gpio GPIO2_13 = GPIO(2, 13);
static const struct gpio GPIO2_14 = GPIO(2, 14);
static const struct gpio GPIO2_15 = GPIO(2, 15);

static const struct gpio GPIO3_0 = GPIO(3, 0);
static const struct gpio GPIO3_1 = GPIO(3, 1);
static const struct gpio GPIO3_2 = GPIO(3, 2);
static const struct gpio GPIO3_3 = GPIO(3, 3);
static const struct gpio GPIO3_4 = GPIO(3, 4);
static const struct gpio GPIO3_5 = GPIO(3, 5);
static const struct gpio GPIO3_6 = GPIO(3, 6);
static const struct gpio GPIO3_7 = GPIO(3, 7);
static const struct gpio GPIO3_8 = GPIO(3, 8);
static const struct gpio GPIO3_9 = GPIO(3, 9);
static const struct gpio GPIO3_10 = GPIO(3, 10);
static const struct gpio GPIO3_11 = GPIO(3, 11);
static const struct gpio GPIO3_12 = GPIO(3, 12);
static const struct gpio GPIO3_13 = GPIO(3, 13);
static const struct gpio GPIO3_14 = GPIO(3, 14);
static const struct gpio GPIO3_15 = GPIO(3, 15);

static const struct gpio GPIO4_0 = GPIO(4, 0);
static const struct gpio GPIO4_1 = GPIO(4, 1);
static const struct gpio GPIO4_2 = GPIO(4, 2);
static const struct gpio GPIO4_3 = GPIO(4, 3);
static const struct gpio GPIO4_4 = GPIO(4, 4);
static const struct gpio GPIO4_5 = GPIO(4, 5);
static const struct gpio GPIO4_6 = GPIO(4, 6);
static const struct gpio GPIO4_7 = GPIO(4, 7);
static const struct gpio GPIO4_8 = GPIO(4, 8);
static const struct gpio GPIO4_9 = GPIO(4, 9);
static const struct gpio GPIO4_10 = GPIO(4, 10);
static const struct gpio GPIO4_11 = GPIO(4, 11);
static const struct gpio GPIO4_12 = GPIO(4, 12);
static const struct gpio GPIO4_13 = GPIO(4, 13);
static const struct gpio GPIO4_14 = GPIO(4, 14);
static const struct gpio GPIO4_15 = GPIO(4, 15);

static const struct gpio GPIO5_0 = GPIO(5, 0);
static const struct gpio GPIO5_1 = GPIO(5, 1);
static const struct gpio GPIO5_2 = GPIO(5, 2);
static const struct gpio GPIO5_3 = GPIO(5, 3);
static const struct gpio GPIO5_4 = GPIO(5, 4);
static const struct gpio GPIO5_5 = GPIO(5, 5);
static const struct gpio GPIO5_6 = GPIO(5, 6);
static const struct gpio GPIO5_7 = GPIO(5, 7);
static const struct gpio GPIO5_8 = GPIO(5, 8);
static const struct gpio GPIO5_9 = GPIO(5, 9);
static const struct gpio GPIO5_10 = GPIO(5, 10);
static const struct gpio GPIO5_11 = GPIO(5, 11);
static const struct gpio GPIO5_12 = GPIO(5, 12);
static const struct gpio GPIO5_13 = GPIO(5, 13);
static const struct gpio GPIO5_14 = GPIO(5, 14);
static const struct gpio GPIO5_15 = GPIO(5, 15);
static const struct gpio GPIO5_16 = GPIO(5, 16);
static const struct gpio GPIO5_17 = GPIO(5, 17);
static const struct gpio GPIO5_18 = GPIO(5, 18);
static const struct gpio GPIO5_19 = GPIO(5, 19);
static const struct gpio GPIO5_20 = GPIO(5, 20);
static const struct gpio GPIO5_21 = GPIO(5, 21);
static const struct gpio GPIO5_22 = GPIO(5, 22);
static const struct gpio GPIO5_23 = GPIO(5, 23);
static const struct gpio GPIO5_24 = GPIO(5, 24);
static const struct gpio GPIO5_25 = GPIO(5, 25);
static const struct gpio GPIO5_26 = GPIO(5, 26);

static const struct gpio GPIO6_0 = GPIO(6, 0);
static const struct gpio GPIO6_1 = GPIO(6, 1);
static const struct gpio GPIO6_2 = GPIO(6, 2);
static const struct gpio GPIO6_3 = GPIO(6, 3);
static const struct gpio GPIO6_4 = GPIO(6, 4);
static const struct gpio GPIO6_5 = GPIO(6, 5);
static const struct gpio GPIO6_6 = GPIO(6, 6);
static const struct gpio GPIO6_7 = GPIO(6, 7);
static const struct gpio GPIO6_8 = GPIO(6, 8);
static const struct gpio GPIO6_9 = GPIO(6, 9);
static const struct gpio GPIO6_10 = GPIO(6, 10);
static const struct gpio GPIO6_11 = GPIO(6, 11);
static const struct gpio GPIO6_12 = GPIO(6, 12);
static const struct gpio GPIO6_13 = GPIO(6, 13);
static const struct gpio GPIO6_14 = GPIO(6, 14);
static const struct gpio GPIO6_15 = GPIO(6, 15);
static const struct gpio GPIO6_16 = GPIO(6, 16);
static const struct gpio GPIO6_17 = GPIO(6, 17);
static const struct gpio GPIO6_18 = GPIO(6, 18);
static const struct gpio GPIO6_19 = GPIO(6, 19);
static const struct gpio GPIO6_20 = GPIO(6, 20);
static const struct gpio GPIO6_21 = GPIO(6, 21);
static const struct gpio GPIO6_22 = GPIO(6, 22);
static const struct gpio GPIO6_23 = GPIO(6, 23);
static const struct gpio GPIO6_24 = GPIO(6, 24);
static const struct gpio GPIO6_25 = GPIO(6, 25);
static const struct gpio GPIO6_26 = GPIO(6, 26);
static const struct gpio GPIO6_27 = GPIO(6, 27);
static const struct gpio GPIO6_28 = GPIO(6, 28);
static const struct gpio GPIO6_29 = GPIO(6, 29);
static const struct gpio GPIO6_30 = GPIO(6, 30);

static const struct gpio GPIO7_0 = GPIO(7, 0);
static const struct gpio GPIO7_1 = GPIO(7, 1);
static const struct gpio GPIO7_2 = GPIO(7, 2);
static const struct gpio GPIO7_3 = GPIO(7, 3);
static const struct gpio GPIO7_4 = GPIO(7, 4);
static const struct gpio GPIO7_5 = GPIO(7, 5);
static const struct gpio GPIO7_6 = GPIO(7, 6);
static const struct gpio GPIO7_7 = GPIO(7, 7);
static const struct gpio GPIO7_8 = GPIO(7, 8);
static const struct gpio GPIO7_9 = GPIO(7, 9);
static const struct gpio GPIO7_10 = GPIO(7, 10);
static const struct gpio GPIO7_11 = GPIO(7, 11);
static const struct gpio GPIO7_12 = GPIO(7, 12);
static const struct gpio GPIO7_13 = GPIO(7, 13);
static const struct gpio GPIO7_14 = GPIO(7, 14);
static const struct gpio GPIO7_15 = GPIO(7, 15);
static const struct gpio GPIO7_16 = GPIO(7, 16);
static const struct gpio GPIO7_17 = GPIO(7, 17);
static const struct gpio GPIO7_18 = GPIO(7, 18);
static const struct gpio GPIO7_19 = GPIO(7, 19);
static const struct gpio GPIO7_20 = GPIO(7, 20);
static const struct gpio GPIO7_21 = GPIO(7, 21);
static const struct gpio GPIO7_22 = GPIO(7, 22);
static const struct gpio GPIO7_23 = GPIO(7, 23);
static const struct gpio GPIO7_24 = GPIO(7, 24);
static const struct gpio GPIO7_25 = GPIO(7, 25);

#ifdef __cplusplus
}
#endif

#endif /* __PLATFORM_GPIO_H */
