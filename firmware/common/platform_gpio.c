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

#include <stddef.h>
#include "platform_gpio.h"

// clang-format off

const platform_gpio_t* platform_gpio(void)
{
	static const platform_gpio_t* _platform_gpio = NULL;
	if (_platform_gpio != NULL) {
		return _platform_gpio;
	}

	board_id_t board_id = detected_platform();
	static platform_gpio_t gpio;

	/* LEDs */
	gpio.led[0] = &GPIO2_1;
	gpio.led[1] = &GPIO2_2;
	gpio.led[2] = &GPIO2_8;
	switch (board_id) {
	case BOARD_ID_RAD1O:
#ifdef RAD1O
		gpio.led[3] = &GPIO5_26;
#endif
		break;
	case BOARD_ID_PRALINE:
#ifdef PRALINE
		gpio.led[3] = &GPIO4_6;
#endif
		break;
	default:
		break;
	}

	/* Power Supply Control */
	switch (board_id) {
	case BOARD_ID_PRALINE:
#ifdef PRALINE
		gpio.gpio_1v2_enable      = &GPIO4_7;
		gpio.gpio_3v3aux_enable_n = &GPIO5_15;
#endif
		break;
	default:
		gpio.gpio_1v8_enable      = &GPIO3_6;
		break;
	}

	/* MAX283x GPIO (XCVR_CTL / CS_XCVR) PinMux */
	switch (board_id) {
	case BOARD_ID_PRALINE:
#ifdef PRALINE
		gpio.max283x_select    = &GPIO6_28;
		gpio.max283x_enable    = &GPIO7_1;
		gpio.max283x_rx_enable = &GPIO7_2;
		gpio.max2831_rxhp      = &GPIO6_29;
		gpio.max2831_ld        = &GPIO4_11;
#endif
		break;
	default:
		gpio.max283x_select    = &GPIO0_15;
		gpio.max283x_enable    = &GPIO2_6;
		gpio.max283x_rx_enable = &GPIO2_5;
		gpio.max283x_tx_enable = &GPIO2_4;
		break;
	}

	/* MAX5864 SPI chip select (AD_CS / CS_AD) GPIO PinMux */
	switch (board_id) {
	case BOARD_ID_PRALINE:
#ifdef PRALINE
		gpio.max5864_select    = &GPIO6_30;
#endif
		break;
	default:
		gpio.max5864_select    = &GPIO2_7;
		break;
	}

	/* RF supply (VAA) control */
	switch (board_id) {
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
		gpio.vaa_disable       = &GPIO2_9;
        break;
    case BOARD_ID_PRALINE:
#ifdef PRALINE
		gpio.vaa_disable       = &GPIO4_1;
#endif
		break;
	case BOARD_ID_RAD1O:
#ifdef RAD1O
		gpio.vaa_enable        = &GPIO2_9;
#endif
		break;
	default:
		break;
	}

	/* W25Q80BV Flash */
	gpio.w25q80bv_hold     = &GPIO1_14;
	gpio.w25q80bv_wp       = &GPIO1_15;
	gpio.w25q80bv_select   = &GPIO5_11;

	/* RF switch control */
	switch (board_id) {
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
		gpio.hp              = &GPIO2_0;
		gpio.lp              = &GPIO2_10;
		gpio.tx_mix_bp       = &GPIO2_11;
		gpio.no_mix_bypass   = &GPIO1_0;
		gpio.rx_mix_bp       = &GPIO2_12;
		gpio.tx_amp          = &GPIO2_15;
		gpio.tx              = &GPIO5_15;
		gpio.mix_bypass      = &GPIO5_16;
		gpio.rx              = &GPIO5_5;
		gpio.no_tx_amp_pwr   = &GPIO3_5;
		gpio.amp_bypass      = &GPIO0_14;
		gpio.rx_amp          = &GPIO1_11;
		gpio.no_rx_amp_pwr   = &GPIO1_12;
        // HackRF One rev 9
		gpio.h1r9_rx         = &GPIO0_7;
		gpio.h1r9_no_ant_pwr = &GPIO2_4;
        break;
	case BOARD_ID_RAD1O:
#ifdef RAD1O
		gpio.tx_rx_n         = &GPIO1_11;
		gpio.tx_rx           = &GPIO0_14;
		gpio.by_mix          = &GPIO1_12;
		gpio.by_mix_n        = &GPIO2_10;
		gpio.by_amp          = &GPIO1_0;
		gpio.by_amp_n        = &GPIO5_5;
		gpio.mixer_en        = &GPIO5_16;
		gpio.low_high_filt   = &GPIO2_11;
		gpio.low_high_filt_n = &GPIO2_12;
		gpio.tx_amp          = &GPIO2_15;
		gpio.rx_lna          = &GPIO5_15;
#endif
        break;
    case BOARD_ID_PRALINE:
#ifdef PRALINE
		gpio.tx_en           = &GPIO3_4;
		gpio.mix_en_n        = &GPIO3_2;
		gpio.mix_en_n_r1_0   = &GPIO5_6;
		gpio.lpf_en          = &GPIO4_8;
		gpio.rf_amp_en       = &GPIO4_9;
		gpio.ant_bias_en_n   = &GPIO1_12;
#endif
		break;
	case BOARD_ID_JELLYBEAN:
	case BOARD_ID_JAWBREAKER:
		break;
	default:
		break;
	}

	/* CPLD JTAG interface GPIO pins_FPGA config pins in Praline */
	gpio.cpld_tck                  = &GPIO3_0;
	switch (board_id) {
    case BOARD_ID_PRALINE:
#ifdef PRALINE
		gpio.fpga_cfg_creset       = &GPIO2_11;
		gpio.fpga_cfg_cdone        = &GPIO5_14;
		gpio.fpga_cfg_spi_cs       = &GPIO2_10;
#endif
		break;
	default:
		gpio.cpld_tdo              = &GPIO5_18;
		switch (board_id) {
		case BOARD_ID_HACKRF1_OG:
		case BOARD_ID_HACKRF1_R9:
		case BOARD_ID_RAD1O:
			gpio.cpld_tms          = &GPIO3_4;
			gpio.cpld_tdi          = &GPIO3_1;
			break;
		default:
			gpio.cpld_tms          = &GPIO3_1;
			gpio.cpld_tdi          = &GPIO3_4;
			break;
			break;
		}
	}
	switch (board_id) {
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
	case BOARD_ID_PRALINE:
		gpio.cpld_pp_tms       = &GPIO1_1;
		gpio.cpld_pp_tdo       = &GPIO1_8;
		break;
	default:
		break;
	}

	/* Other CPLD interface GPIO pins */
	switch (board_id) {
	case BOARD_ID_PRALINE:
		break;
	default:
		gpio.trigger_enable = &GPIO5_12;
		break;
	}
	gpio.q_invert           = &GPIO0_13;

	/* RFFC5071 GPIO serial interface PinMux */
	switch (board_id) {
	case BOARD_ID_JAWBREAKER:
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
		gpio.rffc5072_select = &GPIO2_13;
		gpio.rffc5072_clock  = &GPIO5_6;
		gpio.rffc5072_data   = &GPIO3_3;
		gpio.rffc5072_reset  = &GPIO2_14;
		break;
	case BOARD_ID_RAD1O:
#ifdef RAD1O
		gpio.vco_ce        = &GPIO2_13;
		gpio.vco_sclk      = &GPIO5_6;
		gpio.vco_sdata     = &GPIO3_3;
		gpio.vco_le        = &GPIO2_14;
		gpio.vco_mux       = &GPIO5_25;
		gpio.synt_rfout_en = &GPIO3_5;
#endif
		break;
	case BOARD_ID_PRALINE:
#ifdef PRALINE
		gpio.rffc5072_select = &GPIO2_13;
		gpio.rffc5072_clock  = &GPIO5_18;
		gpio.rffc5072_data   = &GPIO4_14;
		gpio.rffc5072_reset  = &GPIO2_14;
		gpio.rffc5072_ld     = &GPIO6_25;
#endif
		break;
	default:
		break;
	}

	/* HackRF One r9 clock control */
	switch (board_id) {
	case BOARD_ID_HACKRF1_R9:
		gpio.h1r9_clkin_en   = &GPIO5_15;
		gpio.h1r9_clkout_en  = &GPIO0_9;
		gpio.h1r9_mcu_clk_en = &GPIO0_8;
		break;
	default:
		break;
	}

	/* HackRF One r9 */
	switch (board_id) {
	case BOARD_ID_HACKRF1_R9:
		gpio.h1r9_1v8_enable     = &GPIO2_9;
		gpio.h1r9_vaa_disable    = &GPIO3_6;
		gpio.h1r9_trigger_enable = &GPIO5_5;
		break;
	default:
		break;
	}

	/* Praline */
	switch (board_id) {
    case BOARD_ID_PRALINE:
#ifdef PRALINE
		gpio.p2_ctrl0     = &GPIO7_3;
		gpio.p2_ctrl1     = &GPIO7_4;
		gpio.p1_ctrl0     = &GPIO0_14;
		gpio.p1_ctrl1     = &GPIO5_16;
		gpio.p1_ctrl2     = &GPIO3_5;
		gpio.clkin_ctrl   = &GPIO0_15;
		gpio.aa_en        = &GPIO1_7;
		gpio.trigger_in   = &GPIO6_26;
		gpio.trigger_out  = &GPIO5_6;
		gpio.pps_out      = &GPIO5_5;
#endif
		break;
	default:
		break;
	}

	_platform_gpio = &gpio;

	return _platform_gpio;
}

// clang-format on
