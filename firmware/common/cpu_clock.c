/*
 * Copyright 2026 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include "cpu_clock.h"

#include <stdint.h>

#include <libopencm3/lpc43xx/cgu.h>
#if defined(IS_JAWBREAKER) || defined(IS_HACKRF_ONE) || defined(IS_PRALINE)
	#include <libopencm3/lpc43xx/ccu.h>
#endif

#include "delay.h"
#include "hackrf_core.h"
#include "i2c_bus.h"
#ifdef IS_NOT_RAD1O
	#include "platform_detect.h"
#endif

/*
Configure PLL1 (Main MCU Clock) to max speed (204MHz).
Note: PLL1 clock is used by M4/M0 core, Peripheral, APB1.
This function shall be called after cpu_clock_init().
*/
static void cpu_clock_pll1_max_speed(void)
{
	uint32_t reg_val;

	/* This function implements the sequence recommended in:
	 * UM10503 Rev 2.4 (Aug 2018), section 13.2.1.1, page 167. */

	/* 1. Select the IRC as BASE_M4_CLK source. */
	reg_val = CGU_BASE_M4_CLK;
	reg_val &= ~CGU_BASE_M4_CLK_CLK_SEL_MASK;
	reg_val |= CGU_BASE_M4_CLK_CLK_SEL(CGU_SRC_IRC) | CGU_BASE_M4_CLK_AUTOBLOCK(1);
	CGU_BASE_M4_CLK = reg_val;

	/* 2. Enable the crystal oscillator. */
	CGU_XTAL_OSC_CTRL &= ~CGU_XTAL_OSC_CTRL_ENABLE_MASK;

	/* 3. Wait 250us. */
	delay_us_at_mhz(250, 12);

	/* 4. Set the AUTOBLOCK bit. */
	CGU_PLL1_CTRL |= CGU_PLL1_CTRL_AUTOBLOCK(1);

	/* 5. Reconfigure PLL1 to produce the final output frequency, with the
	 *    crystal oscillator as clock source. */
	reg_val = CGU_PLL1_CTRL;
	// clang-format off
	reg_val &= ~( CGU_PLL1_CTRL_CLK_SEL_MASK |
	              CGU_PLL1_CTRL_PD_MASK |
	              CGU_PLL1_CTRL_FBSEL_MASK |
	              CGU_PLL1_CTRL_BYPASS_MASK |
	              CGU_PLL1_CTRL_DIRECT_MASK |
	              CGU_PLL1_CTRL_PSEL_MASK |
	              CGU_PLL1_CTRL_MSEL_MASK |
	              CGU_PLL1_CTRL_NSEL_MASK );
	/* Set PLL1 up to 12MHz * 17 = 204MHz.
	 * Direct mode: FCLKOUT = FCCO = M*(FCLKIN/N) */
	reg_val |= CGU_PLL1_CTRL_CLK_SEL(CGU_SRC_XTAL) |
	           CGU_PLL1_CTRL_PSEL(0) |
	           CGU_PLL1_CTRL_NSEL(0) |
	           CGU_PLL1_CTRL_MSEL(16) |
	           CGU_PLL1_CTRL_FBSEL(0) |
	           CGU_PLL1_CTRL_DIRECT(1);
	// clang-format on
	CGU_PLL1_CTRL = reg_val;

	/* 6. Wait for PLL1 to lock. */
	while (!(CGU_PLL1_STAT & CGU_PLL1_STAT_LOCK_MASK)) {}

	/* 7. Set the PLL1 P-divider to divide by 2 (DIRECT=0, PSEL=0). */
	CGU_PLL1_CTRL &= ~CGU_PLL1_CTRL_DIRECT_MASK;

	/* 8. Select PLL1 as BASE_M4_CLK source. */
	reg_val = CGU_BASE_M4_CLK;
	reg_val &= ~CGU_BASE_M4_CLK_CLK_SEL_MASK;
	reg_val |= CGU_BASE_M4_CLK_CLK_SEL(CGU_SRC_PLL1);
	CGU_BASE_M4_CLK = reg_val;

	/* 9. Wait 50us. */
	delay_us_at_mhz(50, 102);

	/* 10. Set the PLL1 P-divider to direct output mode (DIRECT=1). */
	CGU_PLL1_CTRL |= CGU_PLL1_CTRL_DIRECT_MASK;
}

/* clock startup for LPC4320 configure PLL1 to max speed (204MHz).
Note: PLL1 clock is used by M4/M0 core, Peripheral, APB1. */
void cpu_clock_init(void)
{
	/* use IRC as clock source for APB1 (including I2C0) */
	CGU_BASE_APB1_CLK = CGU_BASE_APB1_CLK_CLK_SEL(CGU_SRC_IRC);

	/* use IRC as clock source for APB3 */
	CGU_BASE_APB3_CLK = CGU_BASE_APB3_CLK_CLK_SEL(CGU_SRC_IRC);

	//FIXME disable I2C
	/* Kick I2C0 down to 400kHz when we switch over to APB1 clock = 204MHz */
	i2c_bus_start(clock_gen.bus, &i2c_config_si5351c_fast_clock);

	/*
	 * 12MHz clock is entering LPC XTAL1/OSC input now.
	 * On HackRF One and Jawbreaker, there is a 12 MHz crystal at the LPC.
	 * Set up PLL1 to run from XTAL1 input.
	 */

	//FIXME a lot of the details here should be in a CGU driver

	/* set xtal oscillator to low frequency mode */
	CGU_XTAL_OSC_CTRL &= ~CGU_XTAL_OSC_CTRL_HF_MASK;

	cpu_clock_pll1_max_speed();

	/* use XTAL_OSC as clock source for APB1 */
	CGU_BASE_APB1_CLK =
		CGU_BASE_APB1_CLK_AUTOBLOCK(1) | CGU_BASE_APB1_CLK_CLK_SEL(CGU_SRC_XTAL);

	/* use XTAL_OSC as clock source for APB3 */
	CGU_BASE_APB3_CLK =
		CGU_BASE_APB3_CLK_AUTOBLOCK(1) | CGU_BASE_APB3_CLK_CLK_SEL(CGU_SRC_XTAL);

	/* use XTAL_OSC as clock source for PLL0USB */
	CGU_PLL0USB_CTRL = CGU_PLL0USB_CTRL_PD(1) | CGU_PLL0USB_CTRL_AUTOBLOCK(1) |
		CGU_PLL0USB_CTRL_CLK_SEL(CGU_SRC_XTAL);
	while (CGU_PLL0USB_STAT & CGU_PLL0USB_STAT_LOCK_MASK) {}

	/* configure PLL0USB to produce 480 MHz clock from 12 MHz XTAL_OSC */
	/* Values from User Manual v1.4 Table 94, for 12MHz oscillator. */
	CGU_PLL0USB_MDIV = 0x06167FFA;
	CGU_PLL0USB_NP_DIV = 0x00302062;
	CGU_PLL0USB_CTRL |=
		(CGU_PLL0USB_CTRL_PD(1) | CGU_PLL0USB_CTRL_DIRECTI(1) |
		 CGU_PLL0USB_CTRL_DIRECTO(1) | CGU_PLL0USB_CTRL_CLKEN(1));

	/* power on PLL0USB and wait until stable */
	CGU_PLL0USB_CTRL &= ~CGU_PLL0USB_CTRL_PD_MASK;
	while (!(CGU_PLL0USB_STAT & CGU_PLL0USB_STAT_LOCK_MASK)) {}

	/* use PLL0USB as clock source for USB0 */
	CGU_BASE_USB0_CLK = CGU_BASE_USB0_CLK_AUTOBLOCK(1) |
		CGU_BASE_USB0_CLK_CLK_SEL(CGU_SRC_PLL0USB);

	/* Switch peripheral clock over to use PLL1 (204MHz) */
	CGU_BASE_PERIPH_CLK = CGU_BASE_PERIPH_CLK_AUTOBLOCK(1) |
		CGU_BASE_PERIPH_CLK_CLK_SEL(CGU_SRC_PLL1);

	/* Switch APB1 clock over to use PLL1 (204MHz) */
	CGU_BASE_APB1_CLK =
		CGU_BASE_APB1_CLK_AUTOBLOCK(1) | CGU_BASE_APB1_CLK_CLK_SEL(CGU_SRC_PLL1);

	/* Switch APB3 clock over to use PLL1 (204MHz) */
	CGU_BASE_APB3_CLK =
		CGU_BASE_APB3_CLK_AUTOBLOCK(1) | CGU_BASE_APB3_CLK_CLK_SEL(CGU_SRC_PLL1);

	CGU_BASE_SSP0_CLK =
		CGU_BASE_SSP0_CLK_AUTOBLOCK(1) | CGU_BASE_SSP0_CLK_CLK_SEL(CGU_SRC_PLL1);

	CGU_BASE_SSP1_CLK =
		CGU_BASE_SSP1_CLK_AUTOBLOCK(1) | CGU_BASE_SSP1_CLK_CLK_SEL(CGU_SRC_PLL1);

#ifdef IS_NOT_RAD1O
	if (IS_NOT_RAD1O) {
		/* Disable unused clocks */
		/* Start with PLLs */
		CGU_PLL0AUDIO_CTRL = CGU_PLL0AUDIO_CTRL_PD(1);

		/* Dividers */
		CGU_IDIVA_CTRL = CGU_IDIVA_CTRL_PD(1);
		CGU_IDIVB_CTRL = CGU_IDIVB_CTRL_PD(1);
		CGU_IDIVC_CTRL = CGU_IDIVC_CTRL_PD(1);
		CGU_IDIVD_CTRL = CGU_IDIVD_CTRL_PD(1);
		CGU_IDIVE_CTRL = CGU_IDIVE_CTRL_PD(1);

		/* Base clocks */
		CGU_BASE_SPIFI_CLK =
			CGU_BASE_SPIFI_CLK_PD(1); /* SPIFI is only used at boot */
		CGU_BASE_USB1_CLK =
			CGU_BASE_USB1_CLK_PD(1); /* USB1 is not exposed on HackRF */
		CGU_BASE_PHY_RX_CLK = CGU_BASE_PHY_RX_CLK_PD(1);
		CGU_BASE_PHY_TX_CLK = CGU_BASE_PHY_TX_CLK_PD(1);
		CGU_BASE_LCD_CLK = CGU_BASE_LCD_CLK_PD(1);
		CGU_BASE_VADC_CLK = CGU_BASE_VADC_CLK_PD(1);
		CGU_BASE_SDIO_CLK = CGU_BASE_SDIO_CLK_PD(1);
		CGU_BASE_UART0_CLK = CGU_BASE_UART0_CLK_PD(1);
		CGU_BASE_UART1_CLK = CGU_BASE_UART1_CLK_PD(1);
		CGU_BASE_UART2_CLK = CGU_BASE_UART2_CLK_PD(1);
		CGU_BASE_UART3_CLK = CGU_BASE_UART3_CLK_PD(1);
		CGU_BASE_OUT_CLK = CGU_BASE_OUT_CLK_PD(1);
		CGU_BASE_AUDIO_CLK = CGU_BASE_AUDIO_CLK_PD(1);
		CGU_BASE_CGU_OUT0_CLK = CGU_BASE_CGU_OUT0_CLK_PD(1);
		CGU_BASE_CGU_OUT1_CLK = CGU_BASE_CGU_OUT1_CLK_PD(1);

		/* Disable unused peripheral clocks */
		CCU1_CLK_APB1_CAN1_CFG = 0;
		CCU1_CLK_APB1_I2S_CFG = 0;
		CCU1_CLK_APB1_MOTOCONPWM_CFG = 0;
		//CCU1_CLK_APB3_ADC0_CFG = 0;
		CCU1_CLK_APB3_ADC1_CFG = 0;
		CCU1_CLK_APB3_CAN0_CFG = 0;
		CCU1_CLK_APB3_DAC_CFG = 0;
		//CCU1_CLK_M4_DMA_CFG = 0;
		CCU1_CLK_M4_EMC_CFG = 0;
		CCU1_CLK_M4_EMCDIV_CFG = 0;
		CCU1_CLK_M4_ETHERNET_CFG = 0;
		CCU1_CLK_M4_LCD_CFG = 0;
		CCU1_CLK_M4_QEI_CFG = 0;
		CCU1_CLK_M4_RITIMER_CFG = 0;
		// CCU1_CLK_M4_SCT_CFG = 0;
		CCU1_CLK_M4_SDIO_CFG = 0;
		CCU1_CLK_M4_SPIFI_CFG = 0;
		CCU1_CLK_M4_TIMER0_CFG = 0;
		//CCU1_CLK_M4_TIMER1_CFG = 0;
		//CCU1_CLK_M4_TIMER2_CFG = 0;
		CCU1_CLK_M4_TIMER3_CFG = 0;
		CCU1_CLK_M4_UART1_CFG = 0;
		CCU1_CLK_M4_USART0_CFG = 0;
		CCU1_CLK_M4_USART2_CFG = 0;
		CCU1_CLK_M4_USART3_CFG = 0;
		CCU1_CLK_M4_USB1_CFG = 0;
		CCU1_CLK_M4_VADC_CFG = 0;
		// CCU1_CLK_SPIFI_CFG = 0;
		// CCU1_CLK_USB1_CFG = 0;
		// CCU1_CLK_VADC_CFG = 0;
		// CCU2_CLK_APB0_UART1_CFG = 0;
		// CCU2_CLK_APB0_USART0_CFG = 0;
		// CCU2_CLK_APB2_USART2_CFG = 0;
		// CCU2_CLK_APB2_USART3_CFG = 0;
		// CCU2_CLK_APLL_CFG = 0;
		// CCU2_CLK_SDIO_CFG = 0;
	}
#endif
}
