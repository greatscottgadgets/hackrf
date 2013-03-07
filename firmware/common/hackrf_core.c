/*
 * Copyright 2012 Michael Ossmann <mike@ossmann.com>
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
#include "si5351c.h"
#include "max2837.h"
#include "rffc5071.h"
#include <libopencm3/lpc43xx/i2c.h>
#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/ssp.h>

/* Define for set_tune_freq() */
#define FREQ_ONE_MHZ	 (1000*1000)

#define MIN_LP_FREQ_MHZ (30)
#define MAX_LP_FREQ_MHZ (2300)

#define MIN_BYPASS_FREQ_MHZ (2300)
#define MAX_BYPASS_FREQ_MHZ (2700)

#define MIN_HP_FREQ_MHZ (2700)
#define MAX_HP_FREQ_MHZ (6000)

#define MAX2837_FREQ_NOMINAL_HZ (2600000000)
#define MAX2837_FREQ_NOMINAL_MHZ (MAX2837_FREQ_NOMINAL_HZ / FREQ_ONE_MHZ)

void delay(uint32_t duration)
{
	uint32_t i;

	for (i = 0; i < duration; i++)
		__asm__("nop");
}

bool sample_rate_set(const uint32_t sample_rate_hz) {
#ifdef JELLYBEAN
	/* Due to design issues, Jellybean/Lemondrop frequency plan is limited.
	 * Long version of the story: The MAX2837 reference frequency
	 * originates from the same PLL as the sample clocks, and in order to
	 * keep the sample clocks in phase and keep jitter noise down, the MAX2837
	 * and sample clocks must be integer-related.
	 */
	uint32_t r_div_sample = 2;
	uint32_t r_div_sgpio = 1;
	
	switch( sample_rate_hz ) {
	case 5000000:
		r_div_sample = 3;	/* 800 MHz / 20 / 8 =  5 MHz */
		r_div_sgpio = 2;	/* 800 MHz / 20 / 4 = 10 MHz */
		break;
		
	case 10000000:
		r_div_sample = 2;	/* 800 MHz / 20 / 4 = 10 MHz */
		r_div_sgpio = 1;	/* 800 MHz / 20 / 2 = 20 MHz */
		break;
		
	case 20000000:
		r_div_sample = 1;	/* 800 MHz / 20 / 2 = 20 MHz */
		r_div_sgpio = 0;	/* 800 MHz / 20 / 1 = 40 MHz */
		break;
		
	default:
		return false;
	}
	
	/* NOTE: Because MS1, 2, 3 outputs are slaved to PLLA, the p1, p2, p3
	 * values are irrelevant. */
	
	/* MS0/CLK1 is the source for the MAX5864 codec. */
	si5351c_configure_multisynth(1, 4608, 0, 1, r_div_sample);

	/* MS0/CLK2 is the source for the CPLD codec clock (same as CLK1). */
	si5351c_configure_multisynth(2, 4608, 0, 1, r_div_sample);

	/* MS0/CLK3 is the source for the SGPIO clock. */
	si5351c_configure_multisynth(3, 4608, 0, 1, r_div_sgpio);
	
	return true;
#endif
	
#ifdef JAWBREAKER
	uint32_t p1 = 4608;
	
	switch(sample_rate_hz) {
	case 5000000:
		p1 = 9728;	// 800MHz / 80 = 10 MHz (SGPIO), 5 MHz (codec)
		break;
	
	case 10000000:
		p1 = 4608;	// 800MHz / 40 = 20 MHz (SGPIO), 10 MHz (codec)
		break;

	case 12500000:
		p1 = 3584;	// 800MHz / 32 = 25 MHz (SGPIO), 12.5 MHz (codec)
		break;

	case 16000000:
		p1 = 2688;	// 800MHz / 25 = 32 MHz (SGPIO), 16 MHz (codec)
		break;
	
	case 20000000:
		p1 = 2048;	// 800MHz / 20 = 40 MHz (SGPIO), 20 MHz (codec)
		break;
	
	default:
		return false;
	}
	
	/* MS0/CLK0 is the source for the MAX5864/CPLD (CODEC_CLK). */
	si5351c_configure_multisynth(0, p1, 0, 1, 1);

	/* MS0/CLK1 is the source for the CPLD (CODEC_X2_CLK). */
	si5351c_configure_multisynth(1, p1, 0, 1, 0);

	/* MS0/CLK2 is the source for SGPIO (CODEC_X2_CLK) */
	si5351c_configure_multisynth(2, p1, 0, 1, 0);

	/* MS0/CLK3 is the source for the external clock output. */
	si5351c_configure_multisynth(3, p1, 0, 1, 0);

	return true;
#endif
}

bool baseband_filter_bandwidth_set(const uint32_t bandwidth_hz) {
	return max2837_set_lpf_bandwidth(bandwidth_hz);
}

/* clock startup for Jellybean with Lemondrop attached */ 
void cpu_clock_init(void)
{
	/* use IRC as clock source for APB1 (including I2C0) */
	CGU_BASE_APB1_CLK = CGU_BASE_APB1_CLK_CLK_SEL(CGU_SRC_IRC);

	i2c0_init(15);

	si5351c_disable_all_outputs();
	si5351c_disable_oeb_pin_control();
	si5351c_power_down_all_clocks();
	si5351c_set_crystal_configuration();
	si5351c_enable_xo_and_ms_fanout();
	si5351c_configure_pll_sources_for_xtal();
	si5351c_configure_pll1_multisynth();

#ifdef JELLYBEAN
	/*
	 * Jellybean/Lemondrop clocks:
	 *   CLK0 -> MAX2837
	 *   CLK1 -> MAX5864/CPLD.GCLK0
	 *   CLK2 -> CPLD.GCLK1
	 *   CLK3 -> CPLD.GCLK2
	 *   CLK4 -> LPC4330
	 *   CLK5 -> RFFC5072
	 *   CLK6 -> extra
	 *   CLK7 -> extra
	 */

	/* MS0/CLK0 is the source for the MAX2837 clock input. */
	si5351c_configure_multisynth(0, 2048, 0, 1, 0); /* 40MHz */

	/* MS4/CLK4 is the source for the LPC43xx microcontroller. */
	si5351c_configure_multisynth(4, 8021, 0, 3, 0); /* 12MHz */

	/* MS5/CLK5 is the source for the RFFC5071 mixer. */
	si5351c_configure_multisynth(5, 1536, 0, 1, 0); /* 50MHz */
#endif

#ifdef JAWBREAKER
	/*
	 * Jawbreaker clocks:
	 *   CLK0 -> MAX5864/CPLD
	 *   CLK1 -> CPLD
	 *   CLK2 -> SGPIO
	 *   CLK3 -> external clock output
	 *   CLK4 -> RFFC5072
	 *   CLK5 -> MAX2837
	 *   CLK6 -> none
	 *   CLK7 -> LPC4330 (but LPC4330 starts up on its own crystal)
	 */

	/* MS4/CLK4 is the source for the RFFC5071 mixer. */
	si5351c_configure_multisynth(4, 1536, 0, 1, 0); /* 50MHz */

	/* MS5/CLK5 is the source for the MAX2837 clock input. */
	si5351c_configure_multisynth(5, 2048, 0, 1, 0); /* 40MHz */

	/* MS7/CLK7 is the source for the LPC43xx microcontroller. */
	//si5351c_configure_multisynth(7, 8021, 0, 3, 0); /* 12MHz */
#endif

	/* Set to 10 MHz, the common rate between Jellybean and Jawbreaker. */
	sample_rate_set(10000000);

	si5351c_configure_clock_control();
	si5351c_enable_clock_outputs();

	//FIXME disable I2C
	/* Kick I2C0 down to 400kHz when we switch over to APB1 clock = 204MHz */
	i2c0_init(255);

	/*
	 * 12MHz clock is entering LPC XTAL1/OSC input now.  On
	 * Jellybean/Lemondrop, this is a signal from the clock generator.  On
	 * Jawbreaker, there is a 12 MHz crystal at the LPC.
	 * Set up PLL1 to run from XTAL1 input.
	 */

	//FIXME a lot of the details here should be in a CGU driver

#ifdef JELLYBEAN
	/* configure xtal oscillator for external clock input signal */
	CGU_XTAL_OSC_CTRL |= CGU_XTAL_OSC_CTRL_BYPASS;
#endif

	/* set xtal oscillator to low frequency mode */
	CGU_XTAL_OSC_CTRL &= ~CGU_XTAL_OSC_CTRL_HF;

	/* power on the oscillator and wait until stable */
	CGU_XTAL_OSC_CTRL &= ~CGU_XTAL_OSC_CTRL_ENABLE;

	/* use XTAL_OSC as clock source for BASE_M4_CLK (CPU) */
	CGU_BASE_M4_CLK = CGU_BASE_M4_CLK_CLK_SEL(CGU_SRC_XTAL);

	/* use XTAL_OSC as clock source for APB1 */
	CGU_BASE_APB1_CLK = CGU_BASE_APB1_CLK_AUTOBLOCK
			| CGU_BASE_APB1_CLK_CLK_SEL(CGU_SRC_XTAL);

	/* use XTAL_OSC as clock source for PLL1 */
	/* Start PLL1 at 12MHz * 17 / (2+2) = 51MHz. */
	CGU_PLL1_CTRL = CGU_PLL1_CTRL_CLK_SEL(CGU_SRC_XTAL)
            | CGU_PLL1_CTRL_PSEL(1)
            | CGU_PLL1_CTRL_NSEL(0)
            | CGU_PLL1_CTRL_MSEL(16)
            | CGU_PLL1_CTRL_PD;

	/* power on PLL1 and wait until stable */
	CGU_PLL1_CTRL &= ~CGU_PLL1_CTRL_PD;
	while (!(CGU_PLL1_STAT & CGU_PLL1_STAT_LOCK));

	/* use PLL1 as clock source for BASE_M4_CLK (CPU) */
	CGU_BASE_M4_CLK = CGU_BASE_M4_CLK_CLK_SEL(CGU_SRC_PLL1);

	/* Move PLL1 up to 12MHz * 17 = 204MHz. */
	CGU_PLL1_CTRL = CGU_PLL1_CTRL_CLK_SEL(CGU_SRC_XTAL)
            | CGU_PLL1_CTRL_PSEL(0)
            | CGU_PLL1_CTRL_NSEL(0)
			| CGU_PLL1_CTRL_MSEL(16)
			| CGU_PLL1_CTRL_FBSEL;

	/* wait until stable */
	while (!(CGU_PLL1_STAT & CGU_PLL1_STAT_LOCK));

	/* use XTAL_OSC as clock source for PLL0USB */
	CGU_PLL0USB_CTRL = CGU_PLL0USB_CTRL_PD
			| CGU_PLL0USB_CTRL_AUTOBLOCK
			| CGU_PLL0USB_CTRL_CLK_SEL(CGU_SRC_XTAL);
	while (CGU_PLL0USB_STAT & CGU_PLL0USB_STAT_LOCK);

	/* configure PLL0USB to produce 480 MHz clock from 12 MHz XTAL_OSC */
	/* Values from User Manual v1.4 Table 94, for 12MHz oscillator. */
	CGU_PLL0USB_MDIV = 0x06167FFA;
	CGU_PLL0USB_NP_DIV = 0x00302062;
	CGU_PLL0USB_CTRL |= (CGU_PLL0USB_CTRL_PD
			| CGU_PLL0USB_CTRL_DIRECTI
			| CGU_PLL0USB_CTRL_DIRECTO
			| CGU_PLL0USB_CTRL_CLKEN);

	/* power on PLL0USB and wait until stable */
	CGU_PLL0USB_CTRL &= ~CGU_PLL0USB_CTRL_PD;
	while (!(CGU_PLL0USB_STAT & CGU_PLL0USB_STAT_LOCK));

	/* use PLL0USB as clock source for USB0 */
	CGU_BASE_USB0_CLK = CGU_BASE_USB0_CLK_AUTOBLOCK
			| CGU_BASE_USB0_CLK_CLK_SEL(CGU_SRC_PLL0USB);

	/* Switch peripheral clock over to use PLL1 (204MHz) */
	CGU_BASE_PERIPH_CLK = CGU_BASE_PERIPH_CLK_AUTOBLOCK
			| CGU_BASE_PERIPH_CLK_CLK_SEL(CGU_SRC_PLL1);

	/* Switch APB1 clock over to use PLL1 (204MHz) */
	CGU_BASE_APB1_CLK = CGU_BASE_APB1_CLK_AUTOBLOCK
			| CGU_BASE_APB1_CLK_CLK_SEL(CGU_SRC_PLL1);
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

void pin_setup(void) {
	/* Release CPLD JTAG pins */
	scu_pinmux(SCU_PINMUX_CPLD_TDO, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION4);
	scu_pinmux(SCU_PINMUX_CPLD_TCK, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_CPLD_TMS, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_CPLD_TDI, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
	
	GPIO_DIR(PORT_CPLD_TDO) &= ~PIN_CPLD_TDO;
	GPIO_DIR(PORT_CPLD_TCK) &= ~PIN_CPLD_TCK;
	GPIO_DIR(PORT_CPLD_TMS) &= ~PIN_CPLD_TMS;
	GPIO_DIR(PORT_CPLD_TDI) &= ~PIN_CPLD_TDI;
	
	/* Configure SCU Pin Mux as GPIO */
	scu_pinmux(SCU_PINMUX_LED1, SCU_GPIO_FAST);
	scu_pinmux(SCU_PINMUX_LED2, SCU_GPIO_FAST);
	scu_pinmux(SCU_PINMUX_LED3, SCU_GPIO_FAST);
	
	scu_pinmux(SCU_PINMUX_EN1V8, SCU_GPIO_FAST);
	
	scu_pinmux(SCU_PINMUX_BOOT0, SCU_GPIO_FAST);
	scu_pinmux(SCU_PINMUX_BOOT1, SCU_GPIO_FAST);
	scu_pinmux(SCU_PINMUX_BOOT2, SCU_GPIO_FAST);
	scu_pinmux(SCU_PINMUX_BOOT3, SCU_GPIO_FAST);
	
	/* Configure all GPIO as Input (safe state) */
	GPIO0_DIR = 0;
	GPIO1_DIR = 0;
	GPIO2_DIR = 0;
	GPIO3_DIR = 0;
	GPIO4_DIR = 0;
	GPIO5_DIR = 0;
	GPIO6_DIR = 0;
	GPIO7_DIR = 0;
	
	/* Configure GPIO2[1/2/8] (P4_1/2 P6_12) as output. */
	GPIO2_DIR |= (PIN_LED1 | PIN_LED2 | PIN_LED3);
	
	/* GPIO3[6] on P6_10  as output. */
	GPIO3_DIR |= PIN_EN1V8;
	
	/* Configure SSP1 Peripheral (to be moved later in SSP driver) */
	scu_pinmux(SCU_SSP1_MISO, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP1_MOSI, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP1_SCK, (SCU_SSP_IO | SCU_CONF_FUNCTION1));
	scu_pinmux(SCU_SSP1_SSEL, (SCU_SSP_IO | SCU_CONF_FUNCTION1));
}

void enable_1v8_power(void) {
	gpio_set(PORT_EN1V8, PIN_EN1V8);
}

/* 
Set freq/tuning between 30MHz to 6000 MHz (less than 16bits really used)
hz between 0 to 999999 Hz (not checked)
return false on error or true if success.
*/
bool set_freq(uint32_t freq_mhz, uint32_t freq_hz)
{
	bool success;
	uint32_t RFFC5071_freq_mhz;
	uint32_t MAX2837_freq_hz;
	uint32_t real_RFFC5071_freq_mhz;
	uint32_t tmp_hz;

	success = true;
	
	if(freq_mhz >= MIN_LP_FREQ_MHZ)
	{
		if(freq_mhz < MAX_LP_FREQ_MHZ)
		{
			/* TODO fix/check Switch to LP mode (shall not change RX/TX mode) */
			//rffc5071_set_gpo(0); /* SWITCHCTRL_LP = 0 */
			
			RFFC5071_freq_mhz = MAX2837_FREQ_NOMINAL_MHZ - freq_mhz;
			/* Set Freq and read real freq */
			real_RFFC5071_freq_mhz = rffc5071_set_frequency(RFFC5071_freq_mhz, 0);
			if(real_RFFC5071_freq_mhz < RFFC5071_freq_mhz)
			{
				tmp_hz = -((RFFC5071_freq_mhz - real_RFFC5071_freq_mhz) * FREQ_ONE_MHZ);
			}else
			{
				tmp_hz = ((real_RFFC5071_freq_mhz - RFFC5071_freq_mhz) * FREQ_ONE_MHZ);
			}
			MAX2837_freq_hz = MAX2837_FREQ_NOMINAL_HZ + tmp_hz + freq_hz;
			max2837_set_frequency(MAX2837_freq_hz);
			rffc5071_tx(0);
		}else if( (freq_mhz >= MIN_BYPASS_FREQ_MHZ) && (freq_mhz < MAX_BYPASS_FREQ_MHZ) )
		{
			/* TODO fix/check Switch to SWITCHCTRL_MIX_BYPASS mode (shall not change RX/TX mode) */
			//rffc5071_set_gpo(SWITCHCTRL_MIX_BYPASS);

			MAX2837_freq_hz = (freq_mhz * FREQ_ONE_MHZ) + freq_hz;
			/* RFFC5071_freq_mhz <= not used in Bypass mode */
			max2837_set_frequency(MAX2837_freq_hz);
			rffc5071_tx(SWITCHCTRL_MIX_BYPASS);
		}else if(  (freq_mhz >= MIN_HP_FREQ_MHZ) && (freq_mhz < MAX_HP_FREQ_MHZ) )
		{
			/* TODO fix/check Switch to SWITCHCTRL_HP mode (shall not change RX/TX mode) */
			//rffc5071_set_gpo(SWITCHCTRL_HP);
			
			//switch_ctrl = SWITCHCTRL_HP;
			RFFC5071_freq_mhz = freq_mhz - MAX2837_FREQ_NOMINAL_MHZ;
			/* Set Freq and read real freq */
			real_RFFC5071_freq_mhz = rffc5071_set_frequency(RFFC5071_freq_mhz, 0);
			if(real_RFFC5071_freq_mhz < RFFC5071_freq_mhz)
			{
				tmp_hz = ((RFFC5071_freq_mhz - real_RFFC5071_freq_mhz) * FREQ_ONE_MHZ);
			}else
			{
				tmp_hz = -((real_RFFC5071_freq_mhz - RFFC5071_freq_mhz) * FREQ_ONE_MHZ);
			}
			MAX2837_freq_hz = MAX2837_FREQ_NOMINAL_HZ + tmp_hz + freq_hz;
			max2837_set_frequency(MAX2837_freq_hz);
			rffc5071_tx(SWITCHCTRL_HP);
		}else
		{
			/* Error freq_mhz too high */
			success = false;
		}
	}else
	{
		/* Error freq_mhz too low */
		success = false;		
	}
	return success;
}
