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
#include "platform_scu.h"

// clang-format off

const platform_scu_t* platform_scu()
{
	static const platform_scu_t* _platform_scu = NULL;
	if (_platform_scu != NULL) {
		return _platform_scu;
	}

	board_id_t board_id = detected_platform();
	board_rev_t board_rev = detected_revision();
	static platform_scu_t scu;
	(void) board_rev; // TODO silence warning until we use this

	/* RFFC5071 GPIO serial interface PinMux */
	switch (board_id) {
	case BOARD_ID_JAWBREAKER:
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
		scu.MIXER_ENX    = (P5_4); /* GPIO2[13] on P5_4 */
		scu.MIXER_SCLK   = (P2_6); /* GPIO5[6] on P2_6 */
		scu.MIXER_SDATA  = (P6_4); /* GPIO3[3] on P6_4 */
		scu.MIXER_RESETX = (P5_5); /* GPIO2[14] on P5_5 */

		scu.MIXER_SCLK_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.MIXER_SDATA_PINCFG = (SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		break;
	case BOARD_ID_PRALINE:
		scu.MIXER_ENX    = (P5_4);  /* GPIO2[13] on P5_4 */
		scu.MIXER_SCLK   = (P9_5);  /* GPIO5[18] on P9_5 */
		scu.MIXER_SDATA  = (P9_2);  /* GPIO4[14] on P9_2 */
		scu.MIXER_RESETX = (P5_5);  /* GPIO2[14] on P5_5 */
		scu.MIXER_LD     = (PD_11); /* GPIO6[25] on PD_11 */

		scu.MIXER_SCLK_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.MIXER_SDATA_PINCFG = (SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		scu.MIXER_LD_PINCFG    = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		break;
	case BOARD_ID_RAD1O:
		scu.VCO_CE        = (P5_4); /* GPIO2[13] on P5_4 */
		scu.VCO_SCLK      = (P2_6); /* GPIO5[6] on P2_6 */
		scu.VCO_SDATA     = (P6_4); /* GPIO3[3] on P6_4 */
		scu.VCO_LE        = (P5_5); /* GPIO2[14] on P5_5 */
		scu.VCO_MUX       = (PB_5); /* GPIO5[25] on PB_5 */
		scu.MIXER_EN      = (P6_8); /* GPIO5[16] on P6_8 */
		scu.SYNT_RFOUT_EN = (P6_9); /* GPIO3[5] on P6_9 */
		break;
	default:
		break;
	}

	/* RF supply (VAA) control */
	switch (board_id) {
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
		scu.NO_VAA_ENABLE = P5_0; /* GPIO2[9] on P5_0 */
		break;
	case BOARD_ID_PRALINE:
		scu.NO_VAA_ENABLE = P8_1; /* GPIO4[1] on P8_1 */
		break;
	case BOARD_ID_RAD1O:
		scu.VAA_ENABLE    = P5_0; /* GPIO2[9] on P5_0 */
		break;
	default:
		break;
	}

	/* RF switch control */
	switch (board_id) {
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
		scu.HP            = P4_0;  /* GPIO2[0] on P4_0 */
		scu.LP            = P5_1;  /* GPIO2[10] on P5_1 */
		scu.TX_MIX_BP     = P5_2;  /* GPIO2[11] on P5_2 */
		scu.NO_MIX_BYPASS = P1_7;  /* GPIO1[0] on P1_7 */
		scu.RX_MIX_BP     = P5_3;  /* GPIO2[12] on P5_3 */
		scu.TX_AMP        = P5_6;  /* GPIO2[15] on P5_6 */
		scu.TX            = P6_7;  /* GPIO5[15] on P6_7 */
		scu.MIX_BYPASS    = P6_8;  /* GPIO5[16] on P6_8 */
		scu.RX            = P2_5;  /* GPIO5[5] on P2_5 */
		scu.NO_TX_AMP_PWR = P6_9;  /* GPIO3[5] on P6_9 */
		scu.AMP_BYPASS    = P2_10; /* GPIO0[14] on P2_10 */
		scu.RX_AMP        = P2_11; /* GPIO1[11] on P2_11 */
		scu.NO_RX_AMP_PWR = P2_12; /* GPIO1[12] on P2_12 */
		break;
	case BOARD_ID_RAD1O:
		scu.BY_AMP          = P1_7;  /* GPIO1[0] on P1_7 */
		scu.BY_AMP_N        = P2_5;  /* GPIO5[5] on P2_5 */
		scu.TX_RX           = P2_10; /* GPIO0[14] on P2_10 */
		scu.TX_RX_N         = P2_11; /* GPIO1[11] on P2_11 */
		scu.BY_MIX          = P2_12; /* GPIO1[12] on P2_12 */
		scu.BY_MIX_N        = P5_1;  /* GPIO2[10] on P5_1 */
		scu.LOW_HIGH_FILT   = P5_2;  /* GPIO2[11] on P5_2 */
		scu.LOW_HIGH_FILT_N = P5_3;  /* GPIO2[12] on P5_3 */
		scu.TX_AMP          = P5_6;  /* GPIO2[15] on P5_6 */
		scu.RX_LNA          = P6_7;  /* GPIO5[15] on P6_7 */
		break;
	case BOARD_ID_PRALINE:
		scu.TX_EN         = P6_5;  /* GPIO3[4] on P6_5 */
		scu.MIX_EN_N      = P6_3;  /* GPIO3[2] on P6_3 */
		scu.MIX_EN_N_R1_0 = P2_6;  /* GPIO5[6] on P2_6 */
		scu.LPF_EN        = PA_1;  /* GPIO4[8] on PA_1 */
		scu.RF_AMP_EN     = PA_2;  /* GPIO4[9] on PA_2 */
		scu.ANT_BIAS_EN_N = P2_12; /* GPIO1[12] on P2_12 */
		scu.ANT_BIAS_OC_N = P2_11; /* GPIO1[11] on P2_11 */
		break;
	default:
		break;
	}

	/* MAX283x GPIO (XCVR_CTL) PinMux */
	switch (board_id) {
	case BOARD_ID_RAD1O:
		scu.XCVR_RXHP = P8_1; /* GPIO[] on P8_1 */
		scu.XCVR_B6   = P8_2; /* GPIO[] on P8_2 */
		scu.XCVR_B7   = P9_3; /* GPIO[] on P9_3 */
		break;
	case BOARD_ID_PRALINE:
		scu.XCVR_ENABLE	  = PE_1;  /* GPIO7[1]  on PE_1 */
		scu.XCVR_RXENABLE = PE_2;  /* GPIO7[2]  on PE_2 */
		scu.XCVR_CS	      = PD_14; /* GPIO6[28] on PD_14 */
		scu.XCVR_RXHP     = PD_15; /* GPIO6[29] on PD_15 */
		scu.XCVR_LD       = P9_6;  /* GPIO4[11] on P9_6 */

		scu.XCVR_ENABLE_PINCFG   = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.XCVR_RXENABLE_PINCFG = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.XCVR_CS_PINCFG       = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.XCVR_RXHP_PINCFG     = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.XCVR_LD_PINCFG       = (SCU_GPIO_FAST | SCU_CONF_FUNCTION0 |
                                    SCU_CONF_EPD_EN_PULLDOWN | SCU_CONF_EPUN_DIS_PULLUP);
		break;
	case BOARD_ID_JELLYBEAN:
	case BOARD_ID_JAWBREAKER:
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
		scu.XCVR_ENABLE   = P4_6;  /* GPIO2[6]  on P4_6 */
		scu.XCVR_RXENABLE = P4_5;  /* GPIO2[5]  on P4_5 */
		scu.XCVR_TXENABLE = P4_4;  /* GPIO2[4]  on P4_4 */
		scu.XCVR_CS       = P1_20; /* GPIO0[15] on P1_20 */

		scu.XCVR_ENABLE_PINCFG   = (SCU_GPIO_FAST);
		scu.XCVR_RXENABLE_PINCFG = (SCU_GPIO_FAST);
		scu.XCVR_TXENABLE_PINCFG = (SCU_GPIO_FAST);
		scu.XCVR_CS_PINCFG       = (SCU_GPIO_FAST);
		break;
	default:
		break;
	}

	/* HackRF One r9 */
	switch (board_id) {
	case BOARD_ID_HACKRF1_R9:
		scu.H1R9_CLKIN_EN    = P6_7;  /* GPIO5[15] on P6_7 */
		scu.H1R9_CLKOUT_EN   = P1_2;  /* GPIO0[9] on P1_2  = has boot pull-down; */
		scu.H1R9_MCU_CLK_EN  = P1_1;  /* GPIO0[8] on P1_1  = has boot pull-up; */
		scu.H1R9_RX          = P2_7;  /* GPIO0[7] on P4_4  = has boot pull-up; */
		scu.H1R9_NO_ANT_PWR  = P4_4;  /* GPIO2[4] on P4_4 */
		scu.H1R9_EN1V8       = P5_0;  /* GPIO2[9] on P5_0 */
		scu.H1R9_NO_VAA_EN   = P6_10; /* GPIO3[6] on P6_10 */
		scu.H1R9_TRIGGER_EN  = P2_5;  /* GPIO5[5] on P2_5 */
		break;
	default:
		break;
	}

	_platform_scu = &scu;

	return _platform_scu;
}

// clang-format on
