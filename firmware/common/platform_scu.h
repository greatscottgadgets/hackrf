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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <libopencm3/lpc43xx/scu.h>

/*
 * SCU PinMux
 */

typedef struct {
	/* GPIO Output PinMux */
	scu_grp_pin_t PINMUX_LED1;
	scu_grp_pin_t PINMUX_LED2;
	scu_grp_pin_t PINMUX_LED3;
#ifdef IS_FOUR_LEDS
	scu_grp_pin_t PINMUX_LED4;
#endif

	scu_grp_pin_t PINMUX_EN1V8;
	scu_grp_pin_t PINMUX_EN1V2;
#ifdef IS_PRALINE
	scu_grp_pin_t PINMUX_EN3V3_AUX_N;
	scu_grp_pin_t PINMUX_EN3V3_OC_N;
#endif

	/* GPIO Input PinMux */
	scu_grp_pin_t PINMUX_BOOT0;
	scu_grp_pin_t PINMUX_BOOT1;
#ifdef IS_NOT_HACKRF_ONE
	scu_grp_pin_t PINMUX_BOOT2;
	scu_grp_pin_t PINMUX_BOOT3;
#endif
	scu_grp_pin_t PINMUX_PP_LCD_TE;
	scu_grp_pin_t PINMUX_PP_LCD_RDX;
	scu_grp_pin_t PINMUX_PP_UNUSED;
	scu_grp_pin_t PINMUX_PP_LCD_WRX;
	scu_grp_pin_t PINMUX_PP_DIR;

	/* USB peripheral */
#ifdef IS_JAWBREAKER
	scu_grp_pin_t PINMUX_USB_LED0;
	scu_grp_pin_t PINMUX_USB_LED1;
#endif

	/* SSP1 Peripheral PinMux */
	scu_grp_pin_t SSP1_CIPO;
	scu_grp_pin_t SSP1_COPI;
	scu_grp_pin_t SSP1_SCK;
	scu_grp_pin_t SSP1_CS;

	/* CPLD JTAG interface */
#ifdef IS_PRALINE
	scu_grp_pin_t PINMUX_FPGA_CRESET;
	scu_grp_pin_t PINMUX_FPGA_CDONE;
	scu_grp_pin_t PINMUX_FPGA_SPI_CS;
#endif
	scu_grp_pin_t PINMUX_CPLD_TDO;
	scu_grp_pin_t PINMUX_CPLD_TCK;
	scu_grp_pin_t PINMUX_CPLD_TMS;
	scu_grp_pin_t PINMUX_CPLD_TDI;

	/* CPLD SGPIO interface */
	scu_grp_pin_t PINMUX_SGPIO0;
	scu_grp_pin_t PINMUX_SGPIO1;
	scu_grp_pin_t PINMUX_SGPIO2;
	scu_grp_pin_t PINMUX_SGPIO3;
	scu_grp_pin_t PINMUX_SGPIO4;
	scu_grp_pin_t PINMUX_SGPIO5;
	scu_grp_pin_t PINMUX_SGPIO6;
	scu_grp_pin_t PINMUX_SGPIO7;
	scu_grp_pin_t PINMUX_SGPIO8;
	scu_grp_pin_t PINMUX_SGPIO9;
	scu_grp_pin_t PINMUX_SGPIO10;
	scu_grp_pin_t PINMUX_SGPIO11;
	scu_grp_pin_t PINMUX_SGPIO12;
	scu_grp_pin_t PINMUX_SGPIO14;
	scu_grp_pin_t PINMUX_SGPIO15;
	scu_grp_pin_t PINMUX_SGPIO0_PINCFG;
	scu_grp_pin_t PINMUX_SGPIO1_PINCFG;
	scu_grp_pin_t PINMUX_SGPIO2_PINCFG;
	scu_grp_pin_t PINMUX_SGPIO3_PINCFG;
	scu_grp_pin_t PINMUX_SGPIO4_PINCFG;
	scu_grp_pin_t PINMUX_SGPIO5_PINCFG;
	scu_grp_pin_t PINMUX_SGPIO6_PINCFG;
	scu_grp_pin_t PINMUX_SGPIO7_PINCFG;
	scu_grp_pin_t PINMUX_SGPIO8_PINCFG;
	scu_grp_pin_t PINMUX_SGPIO9_PINCFG;
	scu_grp_pin_t PINMUX_SGPIO10_PINCFG;
	scu_grp_pin_t PINMUX_SGPIO11_PINCFG;
	scu_grp_pin_t PINMUX_SGPIO12_PINCFG;
	scu_grp_pin_t PINMUX_SGPIO14_PINCFG;
	scu_grp_pin_t PINMUX_SGPIO15_PINCFG;
	scu_grp_pin_t TRIGGER_EN;

	/* MAX283x GPIO (XCVR_CTL) PinMux */
	scu_grp_pin_t XCVR_ENABLE;
	scu_grp_pin_t XCVR_RXENABLE;
	scu_grp_pin_t XCVR_TXENABLE;
	scu_grp_pin_t XCVR_CS;
	uint32_t XCVR_ENABLE_PINCFG;
	uint32_t XCVR_RXENABLE_PINCFG;
	uint32_t XCVR_TXENABLE_PINCFG;
	uint32_t XCVR_CS_PINCFG;
#ifdef IS_PRALINE
	scu_grp_pin_t XCVR_LD;
	uint32_t XCVR_LD_PINCFG;
	uint32_t XCVR_RXHP_PINCFG;
#endif
#if defined(IS_RAD1O) || defined(IS_PRALINE)
	scu_grp_pin_t XCVR_RXHP;
#endif
#ifdef IS_RAD1O
	scu_grp_pin_t XCVR_B6;
	scu_grp_pin_t XCVR_B7;
#endif

	/* MAX5864 SPI chip select (AD_CS) GPIO PinMux */
	scu_grp_pin_t AD_CS;
	scu_grp_pin_t AD_CS_PINCFG;

	/* RFFC5071 GPIO serial interface PinMux */
#ifdef IS_NOT_RAD1O
	scu_grp_pin_t MIXER_ENX;
	scu_grp_pin_t MIXER_SCLK;
	scu_grp_pin_t MIXER_SDATA;
	scu_grp_pin_t MIXER_RESETX;
	uint32_t MIXER_SCLK_PINCFG;
	uint32_t MIXER_SDATA_PINCFG;
#endif
#ifdef IS_PRALINE
	scu_grp_pin_t MIXER_LD;
	uint32_t MIXER_LD_PINCFG;
#endif
#ifdef IS_RAD1O
	scu_grp_pin_t VCO_CE;
	scu_grp_pin_t VCO_SCLK;
	scu_grp_pin_t VCO_SDATA;
	scu_grp_pin_t VCO_LE;
	scu_grp_pin_t VCO_MUX;
	scu_grp_pin_t MIXER_EN;
	scu_grp_pin_t SYNT_RFOUT_EN;
#endif

	/* RF LDO control */
#ifdef IS_JAWBREAKER
	scu_grp_pin_t RF_LDO_ENABLE;
#endif

	/* RF supply (VAA) control */
#if defined(IS_PRALINE) || defined(IS_HACKRF_ONE)
	scu_grp_pin_t NO_VAA_ENABLE;
#endif
#ifdef IS_RAD1O
	scu_grp_pin_t VAA_ENABLE;
#endif

	/* SPI flash */
	scu_grp_pin_t SSP0_CIPO;
	scu_grp_pin_t SSP0_COPI;
	scu_grp_pin_t SSP0_SCK;
	scu_grp_pin_t SSP0_CS;
	scu_grp_pin_t FLASH_HOLD;
	scu_grp_pin_t FLASH_WP;

	/* RF switch control */
#ifdef IS_PRALINE
	scu_grp_pin_t TX_EN;
	scu_grp_pin_t MIX_EN_N;
	scu_grp_pin_t MIX_EN_N_R1_0;
	scu_grp_pin_t LPF_EN;
	scu_grp_pin_t RF_AMP_EN;
	scu_grp_pin_t ANT_BIAS_EN_N;
	scu_grp_pin_t ANT_BIAS_OC_N;
#endif
#ifdef IS_HACKRF_ONE
	scu_grp_pin_t HP;
	scu_grp_pin_t LP;
	scu_grp_pin_t TX_MIX_BP;
	scu_grp_pin_t NO_MIX_BYPASS;
	scu_grp_pin_t RX_MIX_BP;
	scu_grp_pin_t TX_AMP;
	scu_grp_pin_t TX;
	scu_grp_pin_t MIX_BYPASS;
	scu_grp_pin_t RX;
	scu_grp_pin_t NO_TX_AMP_PWR;
	scu_grp_pin_t AMP_BYPASS;
	scu_grp_pin_t RX_AMP;
	scu_grp_pin_t NO_RX_AMP_PWR;
#endif
#ifdef IS_RAD1O
	scu_grp_pin_t BY_AMP;
	scu_grp_pin_t BY_AMP_N;
	scu_grp_pin_t TX_RX;
	scu_grp_pin_t TX_RX_N;
	scu_grp_pin_t BY_MIX;
	scu_grp_pin_t BY_MIX_N;
	scu_grp_pin_t LOW_HIGH_FILT;
	scu_grp_pin_t LOW_HIGH_FILT_N;
	scu_grp_pin_t TX_AMP;
	scu_grp_pin_t RX_LNA;
#endif

	/* Praline */
#ifdef IS_PRALINE
	scu_grp_pin_t P2_CTRL0;
	scu_grp_pin_t P2_CTRL1;
	scu_grp_pin_t P1_CTRL0;
	scu_grp_pin_t P1_CTRL1;
	scu_grp_pin_t P1_CTRL2;
	scu_grp_pin_t CLKIN_CTRL;
	scu_grp_pin_t AA_EN;
	scu_grp_pin_t TRIGGER_IN;
	scu_grp_pin_t TRIGGER_OUT;
	scu_grp_pin_t PPS_OUT;

	scu_grp_pin_t P2_CTRL0_PINCFG;
	scu_grp_pin_t P2_CTRL1_PINCFG;
	scu_grp_pin_t P1_CTRL0_PINCFG;
	scu_grp_pin_t P1_CTRL1_PINCFG;
	scu_grp_pin_t P1_CTRL2_PINCFG;
	scu_grp_pin_t CLKIN_CTRL_PINCFG;
	scu_grp_pin_t AA_EN_PINCFG;
	scu_grp_pin_t TRIGGER_IN_PINCFG;
	scu_grp_pin_t TRIGGER_OUT_PINCFG;
	scu_grp_pin_t PPS_OUT_PINCFG;
#endif

	/* HackRF One r9 */
#ifdef IS_HACKRF_ONE
	scu_grp_pin_t H1R9_CLKIN_EN;
	scu_grp_pin_t H1R9_CLKOUT_EN;
	scu_grp_pin_t H1R9_MCU_CLK_EN;
	scu_grp_pin_t H1R9_RX;
	scu_grp_pin_t H1R9_NO_ANT_PWR;
	scu_grp_pin_t H1R9_EN1V8;
	scu_grp_pin_t H1R9_NO_VAA_EN;
	scu_grp_pin_t H1R9_TRIGGER_EN;
#endif

	/* Miscellaneous */
	scu_grp_pin_t PINMUX_PP_D0;
	scu_grp_pin_t PINMUX_PP_D1;
	scu_grp_pin_t PINMUX_PP_D2;
	scu_grp_pin_t PINMUX_PP_D3;
	scu_grp_pin_t PINMUX_PP_D4;
	scu_grp_pin_t PINMUX_PP_D5;
	scu_grp_pin_t PINMUX_PP_D6;
	scu_grp_pin_t PINMUX_PP_D7;
	/* TODO add other Pins */
	scu_grp_pin_t PINMUX_GPIO3_8;
	scu_grp_pin_t PINMUX_GPIO3_9;
	scu_grp_pin_t PINMUX_GPIO3_10;
	scu_grp_pin_t PINMUX_GPIO3_11;
	scu_grp_pin_t PINMUX_GPIO3_12;
	scu_grp_pin_t PINMUX_GPIO3_13;
	scu_grp_pin_t PINMUX_GPIO3_14;
	scu_grp_pin_t PINMUX_GPIO3_15;

	scu_grp_pin_t PINMUX_PP_TDO;
	scu_grp_pin_t PINMUX_SD_POW;
	scu_grp_pin_t PINMUX_SD_CMD;
	scu_grp_pin_t PINMUX_PP_TMS;
	scu_grp_pin_t PINMUX_SD_VOLT0;
	scu_grp_pin_t PINMUX_SD_DAT0;
	scu_grp_pin_t PINMUX_SD_DAT1;
	scu_grp_pin_t PINMUX_SD_DAT2;
	scu_grp_pin_t PINMUX_SD_DAT3;
	scu_grp_pin_t PINMUX_SD_CD;

	scu_grp_pin_t PINMUX_PP_IO_STBX;
	scu_grp_pin_t PINMUX_PP_ADDR;
	scu_grp_pin_t PINMUX_U0_TXD;
	scu_grp_pin_t PINMUX_U0_RXD;

	scu_grp_pin_t PINMUX_ISP;

	scu_grp_pin_t PINMUX_GP_CLKIN;
} platform_scu_t;

// Detects and returns the global platform scu instance of the active board id and revision.
const platform_scu_t* platform_scu(void);

// Platform detection needs these for error indication.
#define SCU_PINMUX_LED1 (P4_1)  /* GPIO2[1] on P4_1 */
#define SCU_PINMUX_LED2 (P4_2)  /* GPIO2[2] on P4_2 */
#define SCU_PINMUX_LED3 (P6_12) /* GPIO2[8] on P6_12 */

#ifdef __cplusplus
}
#endif
