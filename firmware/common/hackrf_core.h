/*
 * Copyright 2012 Michael Ossmann <mike@ossmann.com>
 * Copyright 2012 Benjamin Vernoux <titanmkd@gmail.com>
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

#ifndef __HACKRF_CORE_H
#define __HACKRF_CORE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

/* hardware identification number */
#define BOARD_ID_JELLYBEAN  0
#define BOARD_ID_JAWBREAKER 1

#ifdef JELLYBEAN
#define BOARD_ID BOARD_ID_JELLYBEAN
#endif

#ifdef JAWBREAKER
#define BOARD_ID BOARD_ID_JAWBREAKER
#endif

/*
 * SCU PinMux
 */

/* GPIO Output PinMux */
#define SCU_PINMUX_LED1     (P4_1)  /* GPIO2[1] on P4_1 */
#define SCU_PINMUX_LED2     (P4_2)  /* GPIO2[2] on P4_2 */
#define SCU_PINMUX_LED3     (P6_12) /* GPIO2[8] on P6_12 */

#define SCU_PINMUX_EN1V8    (P6_10) /* GPIO3[6] on P6_10 */

/* GPIO Input PinMux */
#define SCU_PINMUX_BOOT0    (P1_1)  /* GPIO0[8] on P1_1 */
#define SCU_PINMUX_BOOT1    (P1_2)  /* GPIO0[9] on P1_2 */
#define SCU_PINMUX_BOOT2    (P2_8)  /* GPIO5[7] on P2_8 */
#define SCU_PINMUX_BOOT3    (P2_9)  /* GPIO1[10] on P2_9 */

/* USB peripheral */
#define SCU_PINMUX_USB_LED0 (P6_8)
#define SCU_PINMUX_USB_LED1 (P6_7)

/* SSP1 Peripheral PinMux */
#define SCU_SSP1_MISO       (P1_3)  /* P1_3 */
#define SCU_SSP1_MOSI       (P1_4)  /* P1_4 */
#define SCU_SSP1_SCK        (P1_19) /* P1_19 */
#define SCU_SSP1_SSEL       (P1_20) /* P1_20 */

/* CPLD JTAG interface */
#define SCU_PINMUX_CPLD_TDO (P9_5)  /* GPIO5[18] */
#define SCU_PINMUX_CPLD_TCK (P6_1)  /* GPIO3[ 0] */
#define SCU_PINMUX_CPLD_TMS (P6_2)  /* GPIO3[ 1] */
#define SCU_PINMUX_CPLD_TDI (P6_5)  /* GPIO3[ 4] */

/* CPLD SGPIO interface */
#define SCU_PINMUX_SGPIO0   (P0_0)
#define SCU_PINMUX_SGPIO1   (P0_1)
#define SCU_PINMUX_SGPIO2   (P1_15)
#define SCU_PINMUX_SGPIO3   (P1_16)
#define SCU_PINMUX_SGPIO4   (P6_3)
#define SCU_PINMUX_SGPIO5   (P6_6)
#define SCU_PINMUX_SGPIO6   (P2_2)
#define SCU_PINMUX_SGPIO7   (P1_0)
#ifdef JELLYBEAN
#define SCU_PINMUX_SGPIO8   (P1_12)
#endif
#ifdef JAWBREAKER
#define SCU_PINMUX_SGPIO8   (P9_6)
#endif
#define SCU_PINMUX_SGPIO9   (P4_3)
#define SCU_PINMUX_SGPIO10  (P1_14)
#define SCU_PINMUX_SGPIO11  (P1_17)
#define SCU_PINMUX_SGPIO12  (P1_18)
#define SCU_PINMUX_SGPIO13  (P4_8)
#define SCU_PINMUX_SGPIO14  (P4_9)
#define SCU_PINMUX_SGPIO15  (P4_10)

/* MAX2837 GPIO (XCVR_CTL) PinMux */
#ifdef JELLYBEAN
#define SCU_XCVR_RXHP		(P4_0)	/* GPIO2[0] on P4_0 */
#define SCU_XCVR_B1			(P5_0)	/* GPIO2[9] on P5_0 */
#define SCU_XCVR_B2			(P5_1)	/* GPIO2[10] on P5_1 */
#define SCU_XCVR_B3			(P5_2)	/* GPIO2[11] on P5_2 */
#define SCU_XCVR_B4			(P5_3)	/* GPIO2[12] on P5_3 */
#define SCU_XCVR_B5			(P5_4)	/* GPIO2[13] on P5_4 */
#define SCU_XCVR_B6			(P5_5)	/* GPIO2[14] on P5_5 */
#define SCU_XCVR_B7			(P5_6)	/* GPIO2[15] on P5_6 */
#endif
#define SCU_XCVR_ENABLE     (P4_6)  /* GPIO2[6] on P4_6 */
#define SCU_XCVR_RXENABLE   (P4_5)  /* GPIO2[5] on P4_5 */
#define SCU_XCVR_TXENABLE   (P4_4)  /* GPIO2[4] on P4_4 */
#define SCU_XCVR_CS         (P1_20) /* GPIO0[15] on P1_20 */

/* MAX5864 SPI chip select (AD_CS) GPIO PinMux */
#define SCU_AD_CS           (P5_7)  /* GPIO2[7] on P5_7 */

/* RFFC5071 GPIO serial interface PinMux */
#ifdef JELLYBEAN
#define SCU_MIXER_ENX       (P7_0)  /* GPIO3[8] on P7_0 */
#define SCU_MIXER_SCLK      (P7_1)  /* GPIO3[9] on P7_1 */
#define SCU_MIXER_SDATA     (P7_2)  /* GPIO3[10] on P7_2 */
#define SCU_MIXER_RESETX    (P7_3)  /* GPIO3[11] on P7_3 */
#endif
#ifdef JAWBREAKER
#define SCU_MIXER_ENX       (P5_4)  /* GPIO2[13] on P5_4 */
#define SCU_MIXER_SCLK      (P2_6)  /* GPIO5[6] on P2_6 */
#define SCU_MIXER_SDATA     (P6_4)  /* GPIO3[3] on P6_4 */
#define SCU_MIXER_RESETX    (P5_5)  /* GPIO2[14] on P5_5 */
#endif

/* RF LDO control */
#ifdef JAWBREAKER
#define RF_LDO_ENABLE       (P5_0)  /* GPIO2[9] on P5_0 */
#endif

/* SPI flash */
#define SCU_SSP0_MISO       (P3_6)
#define SCU_SSP0_MOSI       (P3_7)
#define SCU_SSP0_SCK        (P3_3)
#define SCU_SSP0_SSEL       (P3_8) /* GPIO5[11] on P3_8 */
#define SCU_FLASH_HOLD      (P3_4) /* GPIO1[14] on P3_4 */
#define SCU_FLASH_WP        (P3_5) /* GPIO1[15] on P3_5 */

/* TODO add other Pins */

/*
 * GPIO Pins
 */

/* GPIO Output */
#define PIN_LED1    (BIT1) /* GPIO2[1] on P4_1 */
#define PIN_LED2    (BIT2) /* GPIO2[2] on P4_2 */
#define PIN_LED3    (BIT8) /* GPIO2[8] on P6_12 */
#define PORT_LED1_3 (GPIO2) /* PORT for LED1, 2 & 3 */

#define PIN_EN1V8   (BIT6) /* GPIO3[6] on P6_10 */
#define PORT_EN1V8  (GPIO3)

#define PIN_XCVR_CS       (BIT15) /* GPIO0[15] on P1_20 */
#define PORT_XCVR_CS      (GPIO0) /* PORT for CS */
#define PIN_XCVR_ENABLE   (BIT6)  /* GPIO2[6] on P4_6 */
#define PIN_XCVR_RXENABLE (BIT5)  /* GPIO2[5] on P4_5 */
#define PIN_XCVR_TXENABLE (BIT4)  /* GPIO2[4] on P4_4 */
#define PORT_XCVR_ENABLE  (GPIO2) /* PORT for ENABLE, TXENABLE, RXENABLE */
#ifdef JELLYBEAN
#define PIN_XCVR_RXHP     (BIT0)  /* GPIO2[0] on P4_0 */
#define PORT_XCVR_RXHP	  (GPIO2)
#define PIN_XCVR_B1		  (BIT9)  /* GPIO2[9] on P5_0 */
#define PIN_XCVR_B2		  (BIT10) /* GPIO2[10] on P5_1 */
#define PIN_XCVR_B3		  (BIT11) /* GPIO2[11] on P5_2 */
#define PIN_XCVR_B4		  (BIT12) /* GPIO2[12] on P5_3 */
#define PIN_XCVR_B5		  (BIT13) /* GPIO2[13] on P5_4 */
#define PIN_XCVR_B6		  (BIT14) /* GPIO2[14] on P5_5 */
#define PIN_XCVR_B7		  (BIT15) /* GPIO2[15] on P5_6 */
#define PORT_XCVR_B	  	  (GPIO2)
#endif

#define PIN_AD_CS  (BIT7)  /* GPIO2[7] on P5_7 */
#define PORT_AD_CS (GPIO2) /* PORT for AD_CS */

#ifdef JELLYBEAN
#define PIN_MIXER_ENX     (BIT8)  /* GPIO3[8] on P7_0 */
#define PORT_MIXER_ENX    (GPIO3)
#define PIN_MIXER_SCLK    (BIT9)  /* GPIO3[9] on P7_1 */
#define PORT_MIXER_SCLK   (GPIO3)
#define PIN_MIXER_SDATA   (BIT10) /* GPIO3[10] on P7_2 */
#define PORT_MIXER_SDATA  (GPIO3)
#define PIN_MIXER_RESETX  (BIT11) /* GPIO3[11] on P7_3 */
#define PORT_MIXER_RESETX (GPIO3)
#endif
#ifdef JAWBREAKER
#define PIN_MIXER_ENX     (BIT13) /* GPIO2[13] on P5_4 */
#define PORT_MIXER_ENX    (GPIO2)
#define PIN_MIXER_SCLK    (BIT6)  /* GPIO5[6] on P2_6 */
#define PORT_MIXER_SCLK   (GPIO5)
#define PIN_MIXER_SDATA   (BIT3)  /* GPIO3[3] on P6_4 */
#define PORT_MIXER_SDATA  (GPIO3)
#define PIN_MIXER_RESETX  (BIT14) /* GPIO2[14] on P5_5 */
#define PORT_MIXER_RESETX (GPIO2)
#endif

#ifdef JAWBREAKER
#define PIN_RF_LDO_ENABLE  (BIT9)  /* GPIO2[9] on P5_0 */
#define PORT_RF_LDO_ENABLE (GPIO2) /* PORT for RF_LDO_ENABLE */
#endif

#define PIN_FLASH_HOLD (BIT14) /* GPIO1[14] on P3_4 */
#define PIN_FLASH_WP   (BIT15) /* GPIO1[15] on P3_5 */
#define PORT_FLASH     (GPIO1)
#define PIN_SSP0_SSEL  (BIT11) /* GPIO5[11] on P3_8 */
#define PORT_SSP0_SSEL (GPIO5)

/* GPIO Input */
#define PIN_BOOT0   (BIT8)  /* GPIO0[8] on P1_1 */
#define PIN_BOOT1   (BIT9)  /* GPIO0[9] on P1_2 */
#define PIN_BOOT2   (BIT7)  /* GPIO5[7] on P2_8 */
#define PIN_BOOT3   (BIT10) /* GPIO1[10] on P2_9 */

/* CPLD JTAG interface GPIO pins */
#define PIN_CPLD_TDO    (GPIOPIN18)
#define PORT_CPLD_TDO   (GPIO5)
#define PIN_CPLD_TCK    (GPIOPIN0)
#define PORT_CPLD_TCK   (GPIO3)
#define PIN_CPLD_TMS    (GPIOPIN1)
#define PORT_CPLD_TMS   (GPIO3)
#define PIN_CPLD_TDI    (GPIOPIN4)
#define PORT_CPLD_TDI   (GPIO3)

/* Read GPIO Pin */
#define GPIO_STATE(port, pin) ((GPIO_PIN(port) & (pin)) == (pin))
#define BOOT0_STATE       GPIO_STATE(GPIO0, PIN_BOOT0)
#define BOOT1_STATE       GPIO_STATE(GPIO0, PIN_BOOT1)
#define BOOT2_STATE       GPIO_STATE(GPIO5, PIN_BOOT2)
#define BOOT3_STATE       GPIO_STATE(GPIO1, PIN_BOOT3)
#define MIXER_SDATA_STATE GPIO_STATE(PORT_MIXER_SDATA, PIN_MIXER_SDATA)
#define CPLD_TDO_STATE    GPIO_STATE(PORT_CPLD_TDO, PIN_CPLD_TDO)

/* TODO add other Pins */

typedef enum {
	TRANSCEIVER_MODE_OFF = 0,
	TRANSCEIVER_MODE_RX = 1,
	TRANSCEIVER_MODE_TX = 2
} transceiver_mode_t;

void delay(uint32_t duration);

void cpu_clock_init(void);
void cpu_clock_pll1_low_speed(void);
void cpu_clock_pll1_max_speed(void);
void ssp1_init(void);
void ssp1_set_mode_max2837(void);
void ssp1_set_mode_max5864(void);

void pin_setup(void);

void enable_1v8_power(void);

bool sample_rate_frac_set(uint32_t rate_num, uint32_t rate_denom);
bool sample_rate_set(const uint32_t sampling_rate_hz);
bool baseband_filter_bandwidth_set(const uint32_t bandwidth_hz);

#ifdef __cplusplus
}
#endif

#endif /* __HACKRF_CORE_H */
