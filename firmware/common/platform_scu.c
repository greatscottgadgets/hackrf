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

const platform_scu_t* platform_scu(void)
{
	static const platform_scu_t* _platform_scu = NULL;
	if (_platform_scu != NULL) {
		return _platform_scu;
	}

	board_id_t board_id = detected_platform();
	static platform_scu_t scu;

	/* GPIO Output PinMux */
	scu.PINMUX_LED1     = SCU_PINMUX_LED1;  /* GPIO2[1] on P4_1 */
	scu.PINMUX_LED2     = SCU_PINMUX_LED2;  /* GPIO2[2] on P4_2 */
	scu.PINMUX_LED3     = SCU_PINMUX_LED3;  /* GPIO2[8] on P6_12 */
	switch (board_id) {
	case BOARD_ID_RAD1O:
#if defined(RAD1O)
		scu.PINMUX_LED4 = (PB_6);  /* GPIO5[26] on PB_6 */
#endif
		break;
	case BOARD_ID_PRALINE:
#if defined(PRALINE) || defined(HACKRF_ALL)
		scu.PINMUX_LED4 = (P8_6);  /* GPIO4[6] on P8_6 */
#endif
		break;
	default:
		break;
	}

	scu.PINMUX_EN1V8           = (P6_10); /* GPIO3[6] on P6_10 */
	scu.PINMUX_EN1V2           = (P8_7);  /* GPIO4[7] on P8_7 */
	switch (board_id) {
	case BOARD_ID_PRALINE:
#if defined(PRALINE) || defined(HACKRF_ALL)
		scu.PINMUX_EN3V3_AUX_N = (P6_7);  /* GPIO5[15] on P6_7 */
		scu.PINMUX_EN3V3_OC_N  = (P6_11); /* GPIO3[7] on P6_11 */
#endif
		break;
	default:
		break;
	}

	/* GPIO Input PinMux */
	scu.PINMUX_BOOT0      = (P1_1); /* GPIO0[8] on P1_1 */
	scu.PINMUX_BOOT1      = (P1_2); /* GPIO0[9] on P1_2 */
	switch (board_id) {
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
		break;
	default:
#if !defined(HACKRF_ONE) || defined(HACKRF_ALL)
		scu.PINMUX_BOOT2  = (P2_8); /* GPIO5[7] on P2_8 */
		scu.PINMUX_BOOT3  = (P2_9); /* GPIO1[10] on P2_9 */
#endif
		break;
	}
	scu.PINMUX_PP_LCD_TE  = (P2_3);  /* GPIO5[3] on P2_3 */
	scu.PINMUX_PP_LCD_RDX = (P2_4);  /* GPIO5[4] on P2_4 */
	scu.PINMUX_PP_UNUSED  = (P2_8);  /* GPIO5[7] on P2_8 */
	scu.PINMUX_PP_LCD_WRX = (P2_9);  /* GPIO1[10] on P2_9 */
	scu.PINMUX_PP_DIR     = (P2_13); /* GPIO1[13] on P2_13 */

	/* USB peripheral */
	switch (board_id) {
	case BOARD_ID_JAWBREAKER:
#if defined(JAWBREAKER)
		scu.PINMUX_USB_LED0 = (P6_8);
		scu.PINMUX_USB_LED1 = (P6_7);
#endif
		break;
	default:
		break;
	}

	/* SSP1 Peripheral PinMux */
	scu.SSP1_CIPO = (P1_3);  /* P1_3 */
	scu.SSP1_COPI = (P1_4);  /* P1_4 */
	scu.SSP1_SCK  = (P1_19); /* P1_19 */
	scu.SSP1_CS   = (P1_20); /* P1_20 */

	/* CPLD JTAG interface */
	switch (board_id) {
	case BOARD_ID_PRALINE:
#if defined(PRALINE) || defined(HACKRF_ALL)
		scu.PINMUX_FPGA_CRESET = (P5_2);  /* GPIO2[11] on P5_2 */
		scu.PINMUX_FPGA_CDONE  = (P4_10); /* GPIO5[14] */
		scu.PINMUX_FPGA_SPI_CS = (P5_1);  /* GPIO2[10] */
#endif
		break;
	default:
		break;
	}
	scu.PINMUX_CPLD_TDO        = (P9_5); /* GPIO5[18] */
	scu.PINMUX_CPLD_TCK        = (P6_1); /* GPIO3[ 0] */
	switch (board_id) {
	case BOARD_ID_RAD1O:
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
	case BOARD_ID_PRALINE:
		scu.PINMUX_CPLD_TMS    = (P6_5); /* GPIO3[ 4] */
		scu.PINMUX_CPLD_TDI    = (P6_2); /* GPIO3[ 1] */
		break;
	default:
		scu.PINMUX_CPLD_TMS    = (P6_2); /* GPIO3[ 1] */
		scu.PINMUX_CPLD_TDI    = (P6_5); /* GPIO3[ 4] */
		break;
	}

	/* CPLD SGPIO interface */
	switch (board_id) {
	case BOARD_ID_PRALINE:
#if defined(PRALINE) || defined(HACKRF_ALL)
		scu.PINMUX_SGPIO0  = (P0_0);
		scu.PINMUX_SGPIO1  = (P0_1);
		scu.PINMUX_SGPIO2  = (P1_15);
		scu.PINMUX_SGPIO3  = (P1_16);
		scu.PINMUX_SGPIO4  = (P9_4);
		scu.PINMUX_SGPIO5  = (P6_6);
		scu.PINMUX_SGPIO6  = (P2_2);
		scu.PINMUX_SGPIO7  = (P1_0);
		scu.PINMUX_SGPIO8  = (P8_0);
		scu.PINMUX_SGPIO9  = (P9_3);
		scu.PINMUX_SGPIO10 = (P8_2);
		scu.PINMUX_SGPIO11 = (P1_17);
		scu.PINMUX_SGPIO12 = (P1_18);
		scu.PINMUX_SGPIO14 = (P1_18);
		scu.PINMUX_SGPIO15 = (P1_18);

		scu.PINMUX_SGPIO0_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION3);
		scu.PINMUX_SGPIO1_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION3);
		scu.PINMUX_SGPIO2_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION2);
		scu.PINMUX_SGPIO3_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION2);
		scu.PINMUX_SGPIO4_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION6);
		scu.PINMUX_SGPIO5_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION2);
		scu.PINMUX_SGPIO6_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		scu.PINMUX_SGPIO7_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION6);
		scu.PINMUX_SGPIO8_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.PINMUX_SGPIO9_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION6);
		scu.PINMUX_SGPIO10_PINCFG = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.PINMUX_SGPIO11_PINCFG = (SCU_GPIO_FAST | SCU_CONF_FUNCTION6);
		scu.PINMUX_SGPIO12_PINCFG = (SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		scu.PINMUX_SGPIO14_PINCFG = (SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		scu.PINMUX_SGPIO15_PINCFG = (SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
#endif
		break;
	default:
#if !defined(PRALINE) || defined(HACKRF_ALL)
		scu.PINMUX_SGPIO0 = (P0_0);
		scu.PINMUX_SGPIO1 = (P0_1);
		scu.PINMUX_SGPIO2 = (P1_15);
		scu.PINMUX_SGPIO3 = (P1_16);
		scu.PINMUX_SGPIO4 = (P6_3);
		scu.PINMUX_SGPIO5 = (P6_6);
		scu.PINMUX_SGPIO6 = (P2_2);
		scu.PINMUX_SGPIO7 = (P1_0);
		scu.PINMUX_SGPIO8 = (P9_6);
		scu.PINMUX_SGPIO9  = (P4_3);
		scu.PINMUX_SGPIO10 = (P1_14);
		scu.PINMUX_SGPIO11 = (P1_17);
		scu.PINMUX_SGPIO12 = (P1_18);
		scu.PINMUX_SGPIO14 = (P4_9);
		scu.PINMUX_SGPIO15 = (P4_10);

		scu.PINMUX_SGPIO0_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION3);
		scu.PINMUX_SGPIO1_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION3);
		scu.PINMUX_SGPIO2_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION2);
		scu.PINMUX_SGPIO3_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION2);
		scu.PINMUX_SGPIO4_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION2);
		scu.PINMUX_SGPIO5_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION2);
		scu.PINMUX_SGPIO6_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		scu.PINMUX_SGPIO7_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION6);
		scu.PINMUX_SGPIO8_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION6);
		scu.PINMUX_SGPIO9_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION7);
		scu.PINMUX_SGPIO10_PINCFG = (SCU_GPIO_FAST | SCU_CONF_FUNCTION6);
		scu.PINMUX_SGPIO11_PINCFG = (SCU_GPIO_FAST | SCU_CONF_FUNCTION6);
		scu.PINMUX_SGPIO12_PINCFG = (SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		scu.PINMUX_SGPIO14_PINCFG = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.PINMUX_SGPIO15_PINCFG = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
#endif
		break;
	}
	scu.TRIGGER_EN = (P4_8); /* GPIO5[12] on P4_8 */

	/* MAX283x GPIO (XCVR_CTL) PinMux */
	switch (board_id) {
	case BOARD_ID_RAD1O:
#if defined(RAD1O)
		scu.XCVR_RXHP = P8_1; /* GPIO[] on P8_1 */
		scu.XCVR_B6   = P8_2; /* GPIO[] on P8_2 */
		scu.XCVR_B7   = P9_3; /* GPIO[] on P9_3 */
#endif
		break;
	case BOARD_ID_PRALINE:
#if defined(PRALINE) || defined(HACKRF_ALL)
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
#endif
		break;
	case BOARD_ID_JELLYBEAN:
	case BOARD_ID_JAWBREAKER:
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
#if defined(JELLYBEAN) || defined(JAWBREAKER) || defined(HACKRF_ONE) || defined(HACKRF_ALL)
		scu.XCVR_ENABLE   = P4_6;  /* GPIO2[6]  on P4_6 */
		scu.XCVR_RXENABLE = P4_5;  /* GPIO2[5]  on P4_5 */
		scu.XCVR_TXENABLE = P4_4;  /* GPIO2[4]  on P4_4 */
		scu.XCVR_CS       = P1_20; /* GPIO0[15] on P1_20 */

		scu.XCVR_ENABLE_PINCFG   = (SCU_GPIO_FAST);
		scu.XCVR_RXENABLE_PINCFG = (SCU_GPIO_FAST);
		scu.XCVR_TXENABLE_PINCFG = (SCU_GPIO_FAST);
		scu.XCVR_CS_PINCFG       = (SCU_GPIO_FAST);
#endif
		break;
	default:
		break;
	}

	/* MAX5864 SPI chip select (AD_CS) GPIO PinMux */
	switch (board_id) {
	case BOARD_ID_PRALINE:
#if defined(PRALINE) || defined(HACKRF_ALL)
		scu.AD_CS        = (PD_16); /* GPIO6[30] on PD_16 */
		scu.AD_CS_PINCFG = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
#endif
		break;
	default:
		scu.AD_CS        = (P5_7); /* GPIO2[7] on P5_7 */
		scu.AD_CS_PINCFG = (SCU_GPIO_FAST);
		break;
	}

	/* RFFC5071 GPIO serial interface PinMux */
	switch (board_id) {
	case BOARD_ID_JAWBREAKER:
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
#if defined(JAWBREAKER) || defined(HACKRF_ONE) || defined(HACKRF_ALL)
		scu.MIXER_ENX    = (P5_4); /* GPIO2[13] on P5_4 */
		scu.MIXER_SCLK   = (P2_6); /* GPIO5[6] on P2_6 */
		scu.MIXER_SDATA  = (P6_4); /* GPIO3[3] on P6_4 */
		scu.MIXER_RESETX = (P5_5); /* GPIO2[14] on P5_5 */

		scu.MIXER_SCLK_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.MIXER_SDATA_PINCFG = (SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
#endif
		break;
	case BOARD_ID_PRALINE:
#if defined(PRALINE) || defined(HACKRF_ALL)
		scu.MIXER_ENX    = (P5_4);  /* GPIO2[13] on P5_4 */
		scu.MIXER_SCLK   = (P9_5);  /* GPIO5[18] on P9_5 */
		scu.MIXER_SDATA  = (P9_2);  /* GPIO4[14] on P9_2 */
		scu.MIXER_RESETX = (P5_5);  /* GPIO2[14] on P5_5 */
		scu.MIXER_LD     = (PD_11); /* GPIO6[25] on PD_11 */

		scu.MIXER_SCLK_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.MIXER_SDATA_PINCFG = (SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		scu.MIXER_LD_PINCFG    = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
#endif
		break;
	case BOARD_ID_RAD1O:
#if defined(RAD1O)
		scu.VCO_CE        = (P5_4); /* GPIO2[13] on P5_4 */
		scu.VCO_SCLK      = (P2_6); /* GPIO5[6] on P2_6 */
		scu.VCO_SDATA     = (P6_4); /* GPIO3[3] on P6_4 */
		scu.VCO_LE        = (P5_5); /* GPIO2[14] on P5_5 */
		scu.VCO_MUX       = (PB_5); /* GPIO5[25] on PB_5 */
		scu.MIXER_EN      = (P6_8); /* GPIO5[16] on P6_8 */
		scu.SYNT_RFOUT_EN = (P6_9); /* GPIO3[5] on P6_9 */
#endif
		break;
	default:
		break;
	}

	/* RF LDO control */
	switch (board_id) {
	case BOARD_ID_JAWBREAKER:
#if defined(JAWBREAKER)
		scu.RF_LDO_ENABLE = (P5_0); /* GPIO2[9] on P5_0 */
#endif
		break;
	default:
		break;
	}

	/* RF supply (VAA) control */
	switch (board_id) {
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
#if defined(HACKRF_ONE) || defined(HACKRF_ALL)
		scu.NO_VAA_ENABLE = P5_0; /* GPIO2[9] on P5_0 */
#endif
		break;
	case BOARD_ID_PRALINE:
#if defined(PRALINE) || defined(HACKRF_ALL)
		scu.NO_VAA_ENABLE = P8_1; /* GPIO4[1] on P8_1 */
#endif
		break;
	case BOARD_ID_RAD1O:
#if defined(RAD1O)
		scu.VAA_ENABLE    = P5_0; /* GPIO2[9] on P5_0 */
#endif
		break;
	default:
		break;
	}

	/* SPI flash */
	scu.SSP0_CIPO  = (P3_6);
	scu.SSP0_COPI  = (P3_7);
	scu.SSP0_SCK   = (P3_3);
	scu.SSP0_CS    = (P3_8); /* GPIO5[11] on P3_8 */
	scu.FLASH_HOLD = (P3_4); /* GPIO1[14] on P3_4 */
	scu.FLASH_WP   = (P3_5); /* GPIO1[15] on P3_5 */

	/* RF switch control */
	switch (board_id) {
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
#if defined(HACKRF_ONE) || defined(HACKRF_ALL)
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
#endif
		break;
	case BOARD_ID_RAD1O:
#if defined(RAD1O)
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
#endif
		break;
	case BOARD_ID_PRALINE:
#if defined(PRALINE) || defined(HACKRF_ALL)
		scu.TX_EN         = P6_5;  /* GPIO3[4] on P6_5 */
		scu.MIX_EN_N      = P6_3;  /* GPIO3[2] on P6_3 */
		scu.MIX_EN_N_R1_0 = P2_6;  /* GPIO5[6] on P2_6 */
		scu.LPF_EN        = PA_1;  /* GPIO4[8] on PA_1 */
		scu.RF_AMP_EN     = PA_2;  /* GPIO4[9] on PA_2 */
		scu.ANT_BIAS_EN_N = P2_12; /* GPIO1[12] on P2_12 */
		scu.ANT_BIAS_OC_N = P2_11; /* GPIO1[11] on P2_11 */
#endif
		break;
	default:
		break;
	}

	/* Praline */
#if defined(PRALINE) || defined(HACKRF_ALL)
	switch (board_id) {
	case BOARD_ID_PRALINE:
		scu.P2_CTRL0    = (PE_3);  /* GPIO7[3] on PE_3 */
		scu.P2_CTRL1    = (PE_4);  /* GPIO7[4] on PE_4 */
		scu.P1_CTRL0    = (P2_10); /* GPIO0[14] on P2_10 */
		scu.P1_CTRL1    = (P6_8);  /* GPIO5[16] on P6_8 */
		scu.P1_CTRL2    = (P6_9);  /* GPIO3[5] on P6_9 */
		scu.CLKIN_CTRL  = (P1_20); /* GPIO0[15] on P1_20 */
		scu.AA_EN       = (P1_14); /* GPIO1[7] on P1_14 */
		scu.TRIGGER_IN  = (PD_12); /* GPIO6[26] on PD_12 */
		scu.TRIGGER_OUT = (P2_6);  /* GPIO5[6] on P2_6 */
		scu.PPS_OUT     = (P2_5);  /* GPIO5[5] on P2_5 */

		scu.P2_CTRL0_PINCFG    = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.P2_CTRL1_PINCFG    = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.P1_CTRL0_PINCFG    = (SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		scu.P1_CTRL1_PINCFG    = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.P1_CTRL2_PINCFG    = (SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		scu.CLKIN_CTRL_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		scu.AA_EN_PINCFG       = (SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		scu.TRIGGER_IN_PINCFG  = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.TRIGGER_OUT_PINCFG = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.PPS_OUT_PINCFG     = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		break;
	default:
		break;
	}
#endif

	/* Miscellaneous */
	scu.PINMUX_PP_D0 = (P7_0); /* GPIO3[8] */
	scu.PINMUX_PP_D1 = (P7_1); /* GPIO3[9] */
	scu.PINMUX_PP_D2 = (P7_2); /* GPIO3[10] */
	scu.PINMUX_PP_D3 = (P7_3); /* GPIO3[11] */
	scu.PINMUX_PP_D4 = (P7_4); /* GPIO3[12] */
	scu.PINMUX_PP_D5 = (P7_5); /* GPIO3[13] */
	scu.PINMUX_PP_D6 = (P7_6); /* GPIO3[14] */
	scu.PINMUX_PP_D7 = (P7_7); /* GPIO3[15] */
	/* TODO add other Pins */
	scu.PINMUX_GPIO3_8  = (P7_0); /* GPIO3[8] */
	scu.PINMUX_GPIO3_9  = (P7_1); /* GPIO3[9] */
	scu.PINMUX_GPIO3_10 = (P7_2); /* GPIO3[10] */
	scu.PINMUX_GPIO3_11 = (P7_3); /* GPIO3[11] */
	scu.PINMUX_GPIO3_12 = (P7_4); /* GPIO3[12] */
	scu.PINMUX_GPIO3_13 = (P7_5); /* GPIO3[13] */
	scu.PINMUX_GPIO3_14 = (P7_6); /* GPIO3[14] */
	scu.PINMUX_GPIO3_15 = (P7_7); /* GPIO3[15] */

	scu.PINMUX_PP_TDO   = (P1_5);  /* GPIO1[8] */
	scu.PINMUX_SD_POW   = (P1_5);  /* GPIO1[8] */
	scu.PINMUX_SD_CMD   = (P1_6);  /* GPIO1[9] */
	scu.PINMUX_PP_TMS   = (P1_8);  /* GPIO1[1] */
	scu.PINMUX_SD_VOLT0 = (P1_8);  /* GPIO1[1] */
	scu.PINMUX_SD_DAT0  = (P1_9);  /* GPIO1[2] */
	scu.PINMUX_SD_DAT1  = (P1_10); /* GPIO1[3] */
	scu.PINMUX_SD_DAT2  = (P1_11); /* GPIO1[4] */
	scu.PINMUX_SD_DAT3  = (P1_12); /* GPIO1[5] */
	scu.PINMUX_SD_CD    = (P1_13); /* GPIO1[6] */

	scu.PINMUX_PP_IO_STBX = (P2_0); /* GPIO5[0] */
	scu.PINMUX_PP_ADDR    = (P2_1); /* GPIO5[1] */
	scu.PINMUX_U0_TXD     = (P2_0); /* GPIO5[0] */
	scu.PINMUX_U0_RXD     = (P2_1); /* GPIO5[1] */

	scu.PINMUX_ISP = (P2_7); /* GPIO0[7] */

	scu.PINMUX_GP_CLKIN = (P4_7);

	/* HackRF One r9 */
#if defined(HACKRF_ONE) || defined(HACKRF_ALL)
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
#endif

	_platform_scu = &scu;

	return _platform_scu;
}

// clang-format on
