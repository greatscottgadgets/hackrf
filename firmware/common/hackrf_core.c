/*
 * Copyright 2012 Michael Ossmann
 * Copyright 2012 Jared Boone
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
#include "si5351c.h"
#include <libopencm3/lpc43xx/i2c.h>
#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/ssp.h>

void delay(uint32_t duration)
{
	uint32_t i;

	for (i = 0; i < duration; i++)
		__asm__("nop");
}

/* clock startup for Jellybean with Lemondrop attached */ 
void cpu_clock_init(void)
{
	i2c0_init();

	si5351c_disable_all_outputs();
	si5351c_disable_oeb_pin_control();
	si5351c_power_down_all_clocks();
	si5351c_set_crystal_configuration();
	si5351c_enable_xo_and_ms_fanout();
	si5351c_configure_pll_sources_for_xtal();
	si5351c_configure_pll1_multisynth();

	/* MS0/CLK0 is the source for the MAX2837 clock input. */
	si5351c_configure_multisynth(0, 2048, 0, 1, 0); /* 40MHz */

	/* MS0/CLK1 is the source for the MAX5864 codec. */
	si5351c_configure_multisynth(1, 4608, 0, 1, 2); /* 10MHz */

	/* MS0/CLK2 is the source for the CPLD codec clock (same as CLK1). */
	si5351c_configure_multisynth(2, 4608, 0, 1, 2); /* 10MHz */

	/* MS0/CLK3 is the source for the SGPIO clock. */
	si5351c_configure_multisynth(3, 4608, 0, 1, 1); /* 20MHz */

	/* MS4/CLK4 is the source for the LPC43xx microcontroller. */
	si5351c_configure_multisynth(4, 8021, 0, 3, 0); /* 12MHz */

	/* MS5/CLK5 is the source for the RFFC5071 mixer. */
	si5351c_configure_multisynth(5, 1536, 0, 1, 0); /* 50MHz */

	si5351c_configure_clock_control();
	si5351c_enable_clock_outputs();

	//FIXME disable I2C

	/*
	 * 12MHz clock is entering LPC XTAL1/OSC input now.
	 * Set up PLL1 to run from XTAL1 input.
	 */

	//FIXME a lot of the details here should be in a CGU driver

	/* configure xtal oscillator for external clock input signal */
	CGU_XTAL_OSC_CTRL |= CGU_XTAL_OSC_CTRL_BYPASS;

	/* set xtal oscillator to low frequency mode */
	CGU_XTAL_OSC_CTRL &= ~CGU_XTAL_OSC_CTRL_HF;

	/* power on the oscillator and wait until stable */
	CGU_XTAL_OSC_CTRL &= ~CGU_XTAL_OSC_CTRL_ENABLE;

	/* use XTAL_OSC as clock source for BASE_M4_CLK (CPU) */
	CGU_BASE_M4_CLK = ((CGU_SRC_XTAL << CGU_BASE_CLK_SEL_SHIFT));

	/* use XTAL_OSC as clock source for APB1 */
	CGU_BASE_APB1_CLK = (CGU_BASE_CLK_AUTOBLOCK
			| (CGU_SRC_XTAL << CGU_BASE_CLK_SEL_SHIFT));

	/* use XTAL_OSC as clock source for PLL1 */
	CGU_PLL1_CTRL = (CGU_PLL1_CTRL_AUTOBLOCK
			| (CGU_SRC_XTAL << CGU_PLL1_CTRL_CLK_SEL_SHIFT));

	/* configure PLL1 to produce 204 MHz clock from 12 MHz XTAL_OSC */
	/* not sure why, but it doesn't work without the following line */
	CGU_PLL1_CTRL &= ~(CGU_PLL1_CTRL_BYPASS
			| CGU_PLL1_CTRL_FBSEL
			| CGU_PLL1_CTRL_DIRECT
			| CGU_PLL1_CTRL_DIRECT
			| (0x3 << CGU_PLL1_CTRL_PSEL_SHIFT)
			| (0x3 << CGU_PLL1_CTRL_NSEL_SHIFT)
			| (0xFF << CGU_PLL1_CTRL_MSEL_SHIFT));
	CGU_PLL1_CTRL |= (CGU_PLL1_CTRL_FBSEL
			| CGU_PLL1_CTRL_DIRECT
			| (0 << CGU_PLL1_CTRL_PSEL_SHIFT)
			| (0 << CGU_PLL1_CTRL_NSEL_SHIFT)
			| (16 << CGU_PLL1_CTRL_MSEL_SHIFT));

	/* power on PLL1 and wait until stable */
	CGU_PLL1_CTRL &= ~CGU_PLL1_CTRL_PD;
	while (!(CGU_PLL1_STAT & CGU_PLL1_STAT_LOCK));

	/* use PLL1 as clock source for BASE_M4_CLK (CPU) */
	CGU_BASE_M4_CLK = ((CGU_SRC_PLL1 << CGU_BASE_CLK_SEL_SHIFT));

	/* use XTAL_OSC as clock source for PLL0USB */
	CGU_PLL0USB_CTRL = (CGU_PLL0USB_CTRL_PD
			| CGU_PLL0USB_CTRL_AUTOBLOCK
			| (CGU_SRC_XTAL << CGU_PLL0USB_CTRL_CLK_SEL_SHIFT));
	while (CGU_PLL0USB_STAT & CGU_PLL0USB_STAT_LOCK);

	/* configure PLL0USB to produce 480 MHz clock from 12 MHz XTAL_OSC */
	CGU_PLL0USB_MDIV = ((0x07FFA << CGU_PLL0USB_MDIV_MDEC_SHIFT)
			| (0x0B << CGU_PLL0USB_SELP_MDEC_SHIFT)
			| (0x10 << CGU_PLL0USB_SELI_MDEC_SHIFT)
			| (0x0 << CGU_PLL0USB_SELR_MDEC_SHIFT));
	CGU_PLL0USB_NP_DIV = (98 << CGU_PLL0USB_NP_DIV_PDEC_SHIFT)
			| (514 << CGU_PLL0USB_NP_DIV_NDEC_SHIFT);
	CGU_PLL0USB_CTRL |= (CGU_PLL0USB_CTRL_PD
			| CGU_PLL0USB_CTRL_DIRECTI
			| CGU_PLL0USB_CTRL_DIRECTO
			| CGU_PLL0USB_CTRL_CLKEN);

	/* power on PLL0USB and wait until stable */
	CGU_PLL0USB_CTRL &= ~CGU_PLL0USB_CTRL_PD;
	while (!(CGU_PLL0USB_STAT & CGU_PLL0USB_STAT_LOCK));

	/* use PLL0USB as clock source for USB0 */
	CGU_BASE_USB0_CLK = (CGU_BASE_CLK_AUTOBLOCK
			| (CGU_SRC_PLL0USB << CGU_BASE_CLK_SEL_SHIFT));
}

void ssp1_init(void)
{
	/*
	 * Configure CS_AD pin to keep the MAX5864 SPI disabled while we use the
	 * SPI bus for the MAX2837. FIXME: this should probably be somewhere else.
	 */
	scu_pinmux(SCU_AD_CS, SCU_GPIO_FAST);
	GPIO_SET(PORT_AD_CS) = PIN_AD_CS;
	GPIO_DIR(PORT_AD_CS) |= PIN_AD_CS;

	scu_pinmux(SCU_XCVR_CS, SCU_GPIO_FAST);
	GPIO_SET(PORT_XCVR_CS) = PIN_XCVR_CS;
	GPIO_DIR(PORT_XCVR_CS) |= PIN_XCVR_CS;
	
	/* Configure SSP1 Peripheral (to be moved later in SSP driver) */
	scu_pinmux(SCU_SSP1_MISO, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP1_MOSI, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP1_SCK,  (SCU_SSP_IO | SCU_CONF_FUNCTION1));
}

void ssp1_set_mode_max2837(void)
{
	/* FIXME speed up once everything is working reliably */
	const uint8_t serial_clock_rate = 32;
	const uint8_t clock_prescale_rate = 128;

	ssp_init(SSP1_NUM,
		SSP_DATA_16BITS,
		SSP_FRAME_SPI,
		SSP_CPOL_0_CPHA_0,
		serial_clock_rate,
		clock_prescale_rate,
		SSP_MODE_NORMAL,
		SSP_MASTER,
		SSP_SLAVE_OUT_ENABLE);
}

void ssp1_set_mode_max5864(void)
{
	/* FIXME speed up once everything is working reliably */
	const uint8_t serial_clock_rate = 32;
	const uint8_t clock_prescale_rate = 128;

	ssp_init(SSP1_NUM,
		SSP_DATA_8BITS,
		SSP_FRAME_SPI,
		SSP_CPOL_0_CPHA_0,
		serial_clock_rate,
		clock_prescale_rate,
		SSP_MODE_NORMAL,
		SSP_MASTER,
		SSP_SLAVE_OUT_ENABLE);
}
