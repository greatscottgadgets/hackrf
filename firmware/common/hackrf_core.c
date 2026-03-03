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
#include "hackrf_ui.h"
#include "delay.h"
#include "max283x.h"
#include "max5864_target.h"
#include "w25q80bv_target.h"
#include "i2c_lpc.h"
#include "ice40_spi.h"
#include "platform_detect.h"
#include "platform_gpio.h"
#include "platform_scu.h"
#include "clkin.h"
#include "portapack.h"
#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/lpc43xx/ccu.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/ssp.h>

i2c_bus_t i2c0 = {
	.obj = (void*) I2C0_BASE,
	.start = i2c_lpc_start,
	.stop = i2c_lpc_stop,
	.transfer = i2c_lpc_transfer,
};

i2c_bus_t i2c1 = {
	.obj = (void*) I2C1_BASE,
	.start = i2c_lpc_start,
	.stop = i2c_lpc_stop,
	.transfer = i2c_lpc_transfer,
};

// const i2c_lpc_config_t i2c_config_si5351c_slow_clock = {
// 	.duty_cycle_count = 15,
// };

const i2c_lpc_config_t i2c_config_si5351c_fast_clock = {
	.duty_cycle_count = 255,
};

si5351c_driver_t clock_gen = {
	.bus = &i2c0,
	.i2c_address = 0x60,
};

static ssp_config_t ssp_config_max283x = {
	/* FIXME speed up once everything is working reliably */
	/*
	// Freq About 0.0498MHz / 49.8KHz => Freq = PCLK / (CPSDVSR * [SCR+1]) with PCLK=PLL1=204MHz
	const uint8_t serial_clock_rate = 32;
	const uint8_t clock_prescale_rate = 128;
	*/
	// Freq About 4.857MHz => Freq = PCLK / (CPSDVSR * [SCR+1]) with PCLK=PLL1=204MHz
	.serial_clock_rate = 21,
	.clock_prescale_rate = 2,
};

max283x_driver_t max283x = {};

static ssp_config_t ssp_config_max5864 = {
	/* FIXME speed up once everything is working reliably */
	/*
	// Freq About 0.0498MHz / 49.8KHz => Freq = PCLK / (CPSDVSR * [SCR+1]) with PCLK=PLL1=204MHz
	const uint8_t serial_clock_rate = 32;
	const uint8_t clock_prescale_rate = 128;
	*/
	// Freq About 4.857MHz => Freq = PCLK / (CPSDVSR * [SCR+1]) with PCLK=PLL1=204MHz
	.data_bits = SSP_DATA_8BITS,
	.serial_clock_rate = 21,
	.clock_prescale_rate = 2,
};

spi_bus_t spi_bus_ssp1 = {
	.obj = (void*) SSP1_BASE,
	.config = &ssp_config_max5864,
	.start = spi_ssp_start,
	.stop = spi_ssp_stop,
	.transfer = spi_ssp_transfer,
	.transfer_gather = spi_ssp_transfer_gather,
};

max5864_driver_t max5864 = {
	.bus = &spi_bus_ssp1,
	.target_init = max5864_target_init,
};

ssp_config_t ssp_config_w25q80bv = {
	.data_bits = SSP_DATA_8BITS,
	.serial_clock_rate = 2,
	.clock_prescale_rate = 2,
};

static spi_bus_t spi_bus_ssp0 = {
	.obj = (void*) SSP0_BASE,
	.config = &ssp_config_w25q80bv,
	.start = spi_ssp_start,
	.stop = spi_ssp_stop,
	.transfer = spi_ssp_transfer,
	.transfer_gather = spi_ssp_transfer_gather,
};

w25q80bv_driver_t spi_flash = {
	.bus = &spi_bus_ssp0,
	.target_init = w25q80bv_target_init,
};

sgpio_config_t sgpio_config = {
	.slice_mode_multislice = true,
};

#if defined(PRALINE) || defined(UNIVERSAL)
static ssp_config_t ssp_config_ice40_fpga = {
	.data_bits = SSP_DATA_8BITS,
	.spi_mode = SSP_CPOL_1_CPHA_1,
	.serial_clock_rate = 21,
	.clock_prescale_rate = 2,
};

ice40_spi_driver_t ice40 = {
	.bus = &spi_bus_ssp1,
};

fpga_driver_t fpga = {
	.bus = &ice40,
};
#endif

radio_t radio;

rf_path_t rf_path;

jtag_gpio_t jtag_gpio_cpld;

jtag_t jtag_cpld = {
	.gpio = &jtag_gpio_cpld,
};

/* GCD algo from wikipedia */
/* http://en.wikipedia.org/wiki/Greatest_common_divisor */
static uint32_t gcd(uint32_t u, uint32_t v)
{
	int s;

	if (!u || !v) {
		return u | v;
	}

	for (s = 0; !((u | v) & 1); s++) {
		u >>= 1;
		v >>= 1;
	}

	while (!(u & 1)) {
		u >>= 1;
	}

	do {
		while (!(v & 1)) {
			v >>= 1;
		}

		if (u > v) {
			uint32_t t;
			t = v;
			v = u;
			u = t;
		}

		v = v - u;
	} while (v);

	return u << s;
}

bool sample_rate_frac_set(uint32_t rate_num, uint32_t rate_denom)
{
	const uint64_t VCO_FREQ = 800 * 1000 * 1000; /* 800 MHz */
	uint32_t MSx_P1, MSx_P2, MSx_P3;
	uint32_t a, b, c;
	uint32_t rem;

	/* Round to the nearest Hz for display. */
	uint32_t rate_hz = (rate_num + (rate_denom >> 1)) / rate_denom;
	hackrf_ui()->set_sample_rate(rate_hz);

	/*
	 * First double the sample rate so that we can produce a clock at twice
	 * the intended sample rate. The 2x clock is sometimes used directly,
	 * and it is divided by two in an output divider to produce the actual
	 * AFE clock.
	 */
	rate_num *= 2;

	/* Find best config */
	a = (VCO_FREQ * rate_denom) / rate_num;

	rem = (VCO_FREQ * rate_denom) - (a * rate_num);

	if (!rem) {
		/* Integer mode */
		b = 0;
		c = 1;
	} else {
		/* Fractional */
		uint32_t g = gcd(rem, rate_num);
		rem /= g;
		rate_num /= g;

		if (rate_num < (1 << 20)) {
			/* Perfect match */
			b = rem;
			c = rate_num;
		} else {
			/* Approximate */
			c = (1 << 20) - 1;
			b = ((uint64_t) c * (uint64_t) rem) / rate_num;

			g = gcd(b, c);
			b /= g;
			c /= g;
		}
	}

	bool streaming = sgpio_cpld_stream_is_enabled(&sgpio_config);

	if (streaming) {
		sgpio_cpld_stream_disable(&sgpio_config);
	}

	/* Can we enable integer mode ? */
	if (a & 0x1 || b) {
		si5351c_set_int_mode(&clock_gen, 0, 0);
	} else {
		si5351c_set_int_mode(&clock_gen, 0, 1);
	}

	/* Final MS values */
	MSx_P1 = 128 * a + (128 * b / c) - 512;
	MSx_P2 = (128 * b) % c;
	MSx_P3 = c;

	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
		/*
		 * On HackRF One r9 all sample clocks are externally derived
		 * from MS1/CLK1 operating at twice the sample rate.
		 */
		si5351c_configure_multisynth(&clock_gen, 1, MSx_P1, MSx_P2, MSx_P3, 0);
	} else if (detected_platform() != BOARD_ID_PRALINE) {
		/*
		 * On other platforms the clock generator produces three
		 * different sample clocks, all derived from multisynth 0.
		 */
		/* MS0/CLK0 is the source for the MAX5864/CPLD (CODEC_CLK). */
		si5351c_configure_multisynth(&clock_gen, 0, MSx_P1, MSx_P2, MSx_P3, 1);

		/* MS0/CLK1 is the source for the CPLD (CODEC_X2_CLK). */
		si5351c_configure_multisynth(&clock_gen, 1, 0, 0, 0, 0); //p1 doesn't matter

		/* MS0/CLK2 is the source for SGPIO (CODEC_X2_CLK) */
		si5351c_configure_multisynth(&clock_gen, 2, 0, 0, 0, 0); //p1 doesn't matter
	} else {
		/* MS0/CLK0 is the source for the MAX5864/FPGA (AFE_CLK). */
		si5351c_configure_multisynth(&clock_gen, 0, MSx_P1, MSx_P2, MSx_P3, 1);
	}

	if (streaming) {
		sgpio_cpld_stream_enable(&sgpio_config);
	}

	return true;
}

bool sample_rate_set(const uint32_t sample_rate_hz)
{
	uint32_t p1 = 4608;
	uint32_t p2 = 0;
	uint32_t p3 = 0;

	switch (sample_rate_hz) {
	case 8000000:
		p1 = SI_INTDIV(50); // 800MHz / 50 = 16 MHz (SGPIO), 8 MHz (codec)
		break;

	case 9216000:
		// 43.40277777777778: a = 43; b = 29; c = 72
		p1 = 5043;
		p2 = 40;
		p3 = 72;
		break;

	case 10000000:
		p1 = SI_INTDIV(40); // 800MHz / 40 = 20 MHz (SGPIO), 10 MHz (codec)
		break;

	case 12288000:
		// 32.552083333333336: a = 32; b = 159; c = 288
		p1 = 3654;
		p2 = 192;
		p3 = 288;
		break;

	case 12500000:
		p1 = SI_INTDIV(32); // 800MHz / 32 = 25 MHz (SGPIO), 12.5 MHz (codec)
		break;

	case 16000000:
		p1 = SI_INTDIV(25); // 800MHz / 25 = 32 MHz (SGPIO), 16 MHz (codec)
		break;

	case 18432000:
		// 21.70138888889: a = 21; b = 101; c = 144
		p1 = 2265;
		p2 = 112;
		p3 = 144;
		break;

	case 20000000:
		p1 = SI_INTDIV(20); // 800MHz / 20 = 40 MHz (SGPIO), 20 MHz (codec)
		break;

	default:
		return false;
	}

	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
		/*
		 * On HackRF One r9 all sample clocks are externally derived
		 * from MS1/CLK1 operating at twice the sample rate.
		 */
		si5351c_configure_multisynth(&clock_gen, 1, p1, p2, p3, 0);
	} else if (detected_platform() != BOARD_ID_PRALINE) {
		/*
		 * On other platforms the clock generator produces three
		 * different sample clocks, all derived from multisynth 0.
		 */
		/* MS0/CLK0 is the source for the MAX5864/CPLD (CODEC_CLK). */
		si5351c_configure_multisynth(&clock_gen, 0, p1, p2, p3, 1);

		/* MS0/CLK1 is the source for the CPLD (CODEC_X2_CLK). */
		si5351c_configure_multisynth(
			&clock_gen,
			1,
			p1,
			0,
			1,
			0); //p1 doesn't matter

		/* MS0/CLK2 is the source for SGPIO (CODEC_X2_CLK) */
		si5351c_configure_multisynth(
			&clock_gen,
			2,
			p1,
			0,
			1,
			0); //p1 doesn't matter
	} else {
		/* MS0/CLK0 is the source for the MAX5864/FPGA (AFE_CLK). */
		si5351c_configure_multisynth(&clock_gen, 0, p1, p2, p3, 1);

		/* MS0/CLK1 is the source for SCT_CLK (CODEC_X2_CLK). */
		si5351c_configure_multisynth(&clock_gen, 1, p1, p2, p3, 0);
	}

	return true;
}

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

	switch (detected_platform()) {
	case BOARD_ID_JAWBREAKER:
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
	case BOARD_ID_PRALINE:
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
		break;
	default:
		break;
	}
}

void clock_gen_init(void)
{
	i2c_bus_start(clock_gen.bus, &i2c_config_si5351c_fast_clock);

	si5351c_init(&clock_gen);
	si5351c_disable_all_outputs(&clock_gen);
	si5351c_disable_oeb_pin_control(&clock_gen);
	si5351c_power_down_all_clocks(&clock_gen);
	si5351c_set_crystal_configuration(&clock_gen);
	si5351c_enable_xo_and_ms_fanout(&clock_gen);
	si5351c_configure_pll_sources(&clock_gen);
	si5351c_configure_pll_multisynth(&clock_gen);

	/*
	 * Clocks on HackRF One r9:
	 *   CLK0 -> MAX5864/CPLD/SGPIO (sample clocks)
	 *   CLK1 -> RFFC5072/MAX2839
	 *   CLK2 -> External Clock Output/LPC43xx (power down at boot)
	 *
	 * Clocks on other platforms:
	 *   CLK0 -> MAX5864/CPLD
	 *   CLK1 -> CPLD
	 *   CLK2 -> SGPIO
	 *   CLK3 -> External Clock Output (power down at boot)
	 *   CLK4 -> RFFC5072 (MAX2837 on rad1o)
	 *   CLK5 -> MAX2837 (MAX2871 on rad1o)
	 *   CLK6 -> none
	 *   CLK7 -> LPC43xx (uses a 12MHz crystal by default)
	 *
	 * Clocks on Praline:
	 *   CLK0 -> AFE_CLK (MAX5864/FPGA)
	 *   CLK1 -> SCT_CLK
	 *   CLK2 -> MCU_CLK (uses a 12MHz crystal by default)
	 *   CLK3 -> External Clock Output (power down at boot)
	 *   CLK4 -> XCVR_CLK (MAX2837)
	 *   CLK5 -> MIX_CLK (RFFC5072)
	 *   CLK6 -> AUX_CLK1
	 *   CLK7 -> AUX_CLK2
	 */

	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
		/* MS0/CLK0 is the reference for both RFFC5071 and MAX2839. */
		si5351c_configure_multisynth(
			&clock_gen,
			0,
			20 * 128 - 512,
			0,
			1,
			0); /* 800/20 = 40MHz */
	} else {
		/* MS4/CLK4 is the source for the RFFC5071 mixer (MAX2837 on rad1o). */
		si5351c_configure_multisynth(
			&clock_gen,
			4,
			20 * 128 - 512,
			0,
			1,
			0); /* 800/20 = 40MHz */
		/* MS5/CLK5 is the source for the MAX2837 clock input (MAX2871 on rad1o). */
		si5351c_configure_multisynth(
			&clock_gen,
			5,
			20 * 128 - 512,
			0,
			1,
			0); /* 800/20 = 40MHz */
	}

	/* MS6/CLK6 is unused. */
	/* MS7/CLK7 is unused. */

	/* Set to 10 MHz, the common rate between Jawbreaker and HackRF One. */
	sample_rate_set(10000000);

	si5351c_set_clock_source(&clock_gen, PLL_SOURCE_XTAL);
	// soft reset
	si5351c_reset_pll(&clock_gen);
	si5351c_enable_clock_outputs(&clock_gen);
}

void clock_gen_shutdown(void)
{
	i2c_bus_start(clock_gen.bus, &i2c_config_si5351c_fast_clock);
	si5351c_disable_all_outputs(&clock_gen);
	si5351c_disable_oeb_pin_control(&clock_gen);
	si5351c_power_down_all_clocks(&clock_gen);
}

clock_source_t activate_best_clock_source(void)
{
	switch (detected_platform()) {
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
	case BOARD_ID_PRALINE:
#if defined(HACKRF_ONE) || defined(PRALINE) || defined(UNIVERSAL)
		/* Ensure PortaPack reference oscillator is off while checking for external clock input. */
		if (portapack_reference_oscillator && portapack()) {
			portapack_reference_oscillator(false);
		}
#endif
		break;
	default:
		break;
	}

	clock_source_t source = CLOCK_SOURCE_HACKRF;

	/* Check for external clock input. */
	if (si5351c_clkin_signal_valid(&clock_gen)) {
		source = CLOCK_SOURCE_EXTERNAL;
	} else {
		switch (detected_platform()) {
		case BOARD_ID_HACKRF1_OG:
		case BOARD_ID_HACKRF1_R9:
		case BOARD_ID_PRALINE:
#if defined(HACKRF_ONE) || defined(PRALINE) || defined(UNIVERSAL)
			/* Enable PortaPack reference oscillator (if present), and check for valid clock. */
			if (portapack_reference_oscillator && portapack()) {
				portapack_reference_oscillator(true);
				delay(510000); /* loop iterations @ 204MHz for >10ms for oscillator to enable. */
				if (si5351c_clkin_signal_valid(&clock_gen)) {
					source = CLOCK_SOURCE_PORTAPACK;
				} else {
					portapack_reference_oscillator(false);
				}
			}
#endif
			break;
		default:
			break;
		}
		/* No external or PortaPack clock was found. Use HackRF Si5351C crystal. */
	}

	si5351c_set_clock_source(
		&clock_gen,
		(source == CLOCK_SOURCE_HACKRF) ? PLL_SOURCE_XTAL : PLL_SOURCE_CLKIN);
	hackrf_ui()->set_clock_source(source);

	return source;
}

void ssp1_set_mode_max283x(void)
{
	spi_bus_start(&spi_bus_ssp1, &ssp_config_max283x);
}

void ssp1_set_mode_max5864(void)
{
	spi_bus_start(max5864.bus, &ssp_config_max5864);
}

#if defined(PRALINE) || defined(UNIVERSAL)
void ssp1_set_mode_ice40(void)
{
	spi_bus_start(&spi_bus_ssp1, &ssp_config_ice40_fpga);
}
#endif

void pin_shutdown(void)
{
	/* Configure all GPIO as Input (safe state) */
	gpio_init();

	/* Detect Platform */
	board_id_t board_id = detected_platform();
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
	switch (board_id) {
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
	case BOARD_ID_PRALINE:
#if defined(HACKRF_ONE) || defined(PRALINE) || defined(UNIVERSAL)
		scu_pinmux(scu->PINMUX_PP_TMS, SCU_GPIO_PUP | SCU_CONF_FUNCTION0);
		scu_pinmux(scu->PINMUX_PP_TDO, SCU_GPIO_PDN | SCU_CONF_FUNCTION0);
#endif
		break;
	default:
		break;
	}
	scu_pinmux(scu->PINMUX_CPLD_TCK, SCU_GPIO_PDN | SCU_CONF_FUNCTION0);
	if (board_id != BOARD_ID_PRALINE) {
#if !defined(PRALINE) || defined(UNIVERSAL)
		scu_pinmux(scu->PINMUX_CPLD_TMS, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
		scu_pinmux(scu->PINMUX_CPLD_TDI, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
		scu_pinmux(scu->PINMUX_CPLD_TDO, SCU_GPIO_PDN | SCU_CONF_FUNCTION4);
#endif
	}

	/* Configure SCU Pin Mux as GPIO */
	scu_pinmux(scu->PINMUX_LED1, SCU_GPIO_NOPULL);
	scu_pinmux(scu->PINMUX_LED2, SCU_GPIO_NOPULL);
	scu_pinmux(scu->PINMUX_LED3, SCU_GPIO_NOPULL);
	switch (board_id) {
	case BOARD_ID_RAD1O:
#if defined(RAD1O)
		scu_pinmux(scu->PINMUX_LED4, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION4);
#endif
		break;
	case BOARD_ID_PRALINE:
#if defined(PRALINE) || defined(UNIVERSAL)
		scu_pinmux(scu->PINMUX_LED4, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
#endif
		break;
	default:
		break;
	}

	/* Configure USB indicators */
	if (board_id == BOARD_ID_JAWBREAKER) {
#if defined(JAWBREAKER)
		scu_pinmux(scu->PINMUX_USB_LED0, SCU_CONF_FUNCTION3);
		scu_pinmux(scu->PINMUX_USB_LED1, SCU_CONF_FUNCTION3);
#endif
	}

	switch (board_id) {
	case BOARD_ID_PRALINE:
#if defined(PRALINE) || defined(UNIVERSAL)
		disable_1v2_power();
		disable_3v3aux_power();
		gpio_output(gpio->gpio_1v2_enable);
		gpio_output(gpio->gpio_3v3aux_enable_n);
		scu_pinmux(scu->PINMUX_EN1V2, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		scu_pinmux(scu->PINMUX_EN3V3_AUX_N, SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
#endif
		break;
	default:
		disable_1v8_power();
		if (detected_platform() == BOARD_ID_HACKRF1_R9) {
#if defined(HACKRF_ONE) || defined(UNIVERSAL)
			gpio_output(gpio->h1r9_1v8_enable);
			scu_pinmux(scu->H1R9_EN1V8, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
#endif
		} else {
			gpio_output(gpio->gpio_1v8_enable);
			scu_pinmux(scu->PINMUX_EN1V8, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		}
		break;
	}

	switch (board_id) {
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
	case BOARD_ID_PRALINE:
#if defined(HACKRF_ONE) || defined(PRALINE) || defined(UNIVERSAL)
		/* Safe state: start with VAA turned off: */
		disable_rf_power();

		/* Configure RF power supply (VAA) switch control signal as output */
		if (board_id == BOARD_ID_HACKRF1_R9) {
	#if defined(HACKRF_ONE) || defined(UNIVERSAL)
			gpio_output(gpio->h1r9_vaa_disable);
	#endif
		} else {
			gpio_output(gpio->vaa_disable);
		}
		break;
#endif
	case BOARD_ID_RAD1O:
#if defined(RAD1O)
		/* Safe state: start with VAA turned off: */
		disable_rf_power();

		/* Configure RF power supply (VAA) switch control signal as output */
		gpio_output(gpio->vaa_enable);

		/* Disable unused clock outputs. They generate noise. */
		scu_pinmux(CLK0, SCU_CLK_IN | SCU_CONF_FUNCTION7);
		scu_pinmux(CLK2, SCU_CLK_IN | SCU_CONF_FUNCTION7);

		scu_pinmux(scu->PINMUX_GPIO3_10, SCU_GPIO_PDN | SCU_CONF_FUNCTION0);
		scu_pinmux(scu->PINMUX_GPIO3_11, SCU_GPIO_PDN | SCU_CONF_FUNCTION0);
#endif
		break;
	default:
		break;
	}

	switch (board_id) {
	case BOARD_ID_PRALINE:
#if defined(PRALINE) || defined(UNIVERSAL)
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
		clkin_ctrl_set(CLKIN_SIGNAL_P22);

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
#endif
		break;
	default:
		break;
	}

	/* enable input on SCL and SDA pins */
	SCU_SFSI2C0 = SCU_I2C0_NOMINAL;
}

/* Run after pin_shutdown() and prior to enabling power supplies. */
void pin_setup(void)
{
	/* Detect Platform */
	board_id_t board_id = detected_platform();
	const platform_gpio_t* gpio = platform_gpio();
	const platform_scu_t* scu = platform_scu();

	/* Configure LEDs */
	led_off(0);
	led_off(1);
	led_off(2);
	switch (board_id) {
	case BOARD_ID_RAD1O:
		led_off(3);
	default:
		break;
	}

	gpio_output(gpio->led[0]);
	gpio_output(gpio->led[1]);
	gpio_output(gpio->led[2]);
	switch (board_id) {
	case BOARD_ID_RAD1O:
	case BOARD_ID_PRALINE:
#if defined(RAD1O) || defined(PRALINE) || defined(UNIVERSAL)
		gpio_output(gpio->led[3]);
#endif
		break;
	default:
		break;
	}

	/* Configure drivers and driver pins */
	ssp_config_max283x.gpio_select = gpio->max283x_select;
	if (board_id == BOARD_ID_PRALINE) {
		ssp_config_max283x.data_bits = SSP_DATA_9BITS; // send 2 words
	} else {
		ssp_config_max283x.data_bits = SSP_DATA_16BITS;
	}

	ssp_config_max5864.gpio_select = gpio->max5864_select;

	ssp_config_w25q80bv.gpio_select = gpio->w25q80bv_select;
	spi_flash.gpio_hold = gpio->w25q80bv_hold;
	spi_flash.gpio_wp = gpio->w25q80bv_wp;

	sgpio_config.gpio_q_invert = gpio->q_invert;
	if (board_id != BOARD_ID_PRALINE) {
#if !defined(PRALINE) || defined(UNIVERSAL)
		sgpio_config.gpio_trigger_enable = gpio->trigger_enable;
#endif
	}

#if defined(PRALINE) || defined(UNIVERSAL)
	ssp_config_ice40_fpga.gpio_select = gpio->fpga_cfg_spi_cs;
	ice40.gpio_select = gpio->fpga_cfg_spi_cs;
	ice40.gpio_creset = gpio->fpga_cfg_creset;
	ice40.gpio_cdone = gpio->fpga_cfg_cdone;
#endif

	jtag_gpio_cpld.gpio_tck = gpio->cpld_tck;
	if (board_id != BOARD_ID_PRALINE) {
#if !defined(PRALINE) || defined(UNIVERSAL)
		jtag_gpio_cpld.gpio_tms = gpio->cpld_tms;
		jtag_gpio_cpld.gpio_tdi = gpio->cpld_tdi;
		jtag_gpio_cpld.gpio_tdo = gpio->cpld_tdo;
#endif
	}
	switch (board_id) {
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
	case BOARD_ID_PRALINE:
#if defined(HACKRF_ONE) || defined(PRALINE) || defined(UNIVERSAL)
		jtag_gpio_cpld.gpio_pp_tms = gpio->cpld_pp_tms;
		jtag_gpio_cpld.gpio_pp_tdo = gpio->cpld_pp_tdo;
#endif
		break;
	default:
		break;
	}

	ssp1_set_mode_max283x();

	mixer_bus_setup(&mixer);

	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
#if defined(HACKRF_ONE) || defined(UNIVERSAL)
		sgpio_config.gpio_trigger_enable = gpio->h1r9_trigger_enable;
#endif
	}

	// initialize rf_path struct and assign gpio's
	switch (board_id) {
	case BOARD_ID_JAWBREAKER:
		break;
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
#if defined(HACKRF_ONE) || defined(UNIVERSAL)
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
		if (board_id == BOARD_ID_HACKRF1_R9) {
			rf_path.gpio_rx = gpio->h1r9_rx;
			rf_path.gpio_h1r9_no_ant_pwr = gpio->h1r9_no_ant_pwr;
		}
#endif
		break;
	case BOARD_ID_RAD1O:
#if defined(RAD1O)
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
#endif
		break;
	case BOARD_ID_PRALINE:
#if defined(PRALINE) || defined(UNIVERSAL)
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
#endif
		break;
	default:
		break;
	}
	rf_path_pin_setup(&rf_path);

	/* Configure external clock in */
	scu_pinmux(scu->PINMUX_GP_CLKIN, SCU_CLK_IN | SCU_CONF_FUNCTION1);

	sgpio_configure_pin_functions(&sgpio_config);
}

#if defined(PRALINE) || defined(UNIVERSAL)
void enable_1v2_power(void)
{
	switch (detected_platform()) {
	case BOARD_ID_PRALINE:
		gpio_set(platform_gpio()->gpio_1v2_enable);
		break;
	default:
		break;
	}
}

void disable_1v2_power(void)
{
	switch (detected_platform()) {
	case BOARD_ID_PRALINE:
		gpio_clear(platform_gpio()->gpio_1v2_enable);
		break;
	default:
		break;
	}
}

void enable_3v3aux_power(void)
{
	switch (detected_platform()) {
	case BOARD_ID_PRALINE:
		gpio_clear(platform_gpio()->gpio_3v3aux_enable_n);
		break;
	default:
		break;
	}
}

void disable_3v3aux_power(void)
{
	switch (detected_platform()) {
	case BOARD_ID_PRALINE:
		gpio_set(platform_gpio()->gpio_3v3aux_enable_n);
		break;
	default:
		break;
	}
}
#endif

void enable_1v8_power(void)
{
	switch (detected_platform()) {
	case BOARD_ID_HACKRF1_R9:
#if defined(HACKRF_ONE) || defined(UNIVERSAL)
		gpio_set(platform_gpio()->h1r9_1v8_enable);
#endif
		break;
	case BOARD_ID_PRALINE:
		break;
	default:
		gpio_set(platform_gpio()->gpio_1v8_enable);
		break;
	}
}

void disable_1v8_power(void)
{
	switch (detected_platform()) {
	case BOARD_ID_HACKRF1_R9:
#if defined(HACKRF_ONE) || defined(UNIVERSAL)
		gpio_clear(platform_gpio()->h1r9_1v8_enable);
#endif
		break;
	case BOARD_ID_PRALINE:
		break;
	default:
		gpio_clear(platform_gpio()->gpio_1v8_enable);
		break;
	}
}

#if defined(HACKRF_ONE) || defined(UNIVERSAL)
static inline void enable_rf_power_hackrf_one(void)
{
	const platform_gpio_t* gpio = platform_gpio();
	uint32_t i;

	/* many short pulses to avoid one big voltage glitch */
	for (i = 0; i < 1000; i++) {
		if (detected_platform() == BOARD_ID_HACKRF1_R9) {
			gpio_set(gpio->h1r9_vaa_disable);
			gpio_clear(gpio->h1r9_vaa_disable);
		} else {
			gpio_set(gpio->vaa_disable);
			gpio_clear(gpio->vaa_disable);
		}
	}
}

static inline void disable_rf_power_hackrf_one(void)
{
	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
		gpio_set(platform_gpio()->h1r9_vaa_disable);
	} else {
		gpio_set(platform_gpio()->vaa_disable);
	}
}
#endif

#if defined(PRALINE) || defined(UNIVERSAL)
static inline void enable_rf_power_praline(void)
{
	gpio_clear(platform_gpio()->vaa_disable);

	/* Let the voltage stabilize */
	delay(1000000);
}

static inline void disable_rf_power_praline(void)
{
	gpio_set(platform_gpio()->vaa_disable);
}
#endif

#if defined(RAD1O)
static inline void enable_rf_power_rad1o(void)
{
	gpio_set(platform_gpio()->vaa_enable);

	/* Let the voltage stabilize */
	delay(1000000);
}

static inline void disable_rf_power_rad1o(void)
{
	gpio_clear(platform_gpio()->vaa_enable);
}
#endif

void enable_rf_power(void)
{
	switch (detected_platform()) {
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
#if defined(HACKRF_ONE) || defined(UNIVERSAL)
		enable_rf_power_hackrf_one();
#endif
		break;
	case BOARD_ID_PRALINE:
#if defined(PRALINE) || defined(UNIVERSAL)
		enable_rf_power_praline();
#endif
		break;
	case BOARD_ID_RAD1O:
#if defined(RAD1O)
		enable_rf_power_rad1o();
#endif
		break;
	default:
		break;
	}
}

void disable_rf_power(void)
{
	switch (detected_platform()) {
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
#if defined(HACKRF_ONE) || defined(UNIVERSAL)
		disable_rf_power_hackrf_one();
#endif
		break;
	case BOARD_ID_PRALINE:
#if defined(PRALINE) || defined(UNIVERSAL)
		disable_rf_power_praline();
#endif
		break;
	case BOARD_ID_RAD1O:
#if defined(RAD1O)
		disable_rf_power_rad1o();
#endif
		break;
	default:
		break;
	}
}

void led_on(const led_t led)
{
	if (detected_platform() == BOARD_ID_PRALINE) {
		gpio_clear(platform_gpio()->led[led]);
	} else {
		gpio_set(platform_gpio()->led[led]);
	}
}

void led_off(const led_t led)
{
	if (detected_platform() == BOARD_ID_PRALINE) {
		gpio_set(platform_gpio()->led[led]);
	} else {
		gpio_clear(platform_gpio()->led[led]);
	}
}

void led_toggle(const led_t led)
{
	gpio_toggle(platform_gpio()->led[led]);
}

void set_leds(const uint8_t state)
{
	board_id_t board_id = detected_platform();

	int num_leds = 3;
	if (board_id == BOARD_ID_RAD1O || board_id == BOARD_ID_PRALINE) {
		num_leds = 4;
	}

	for (int i = 0; i < num_leds; i++) {
		if (board_id == BOARD_ID_PRALINE) {
			gpio_write(platform_gpio()->led[i], ((state >> i) & 1) == 0);
		} else {
			gpio_write(platform_gpio()->led[i], ((state >> i) & 1) == 1);
		}
	}
}

void trigger_enable(const bool enable)
{
	if (detected_platform() != BOARD_ID_PRALINE) {
#if !defined(PRALINE) || defined(UNIVERSAL)
		gpio_write(sgpio_config.gpio_trigger_enable, enable);
#endif
	} else {
#if defined(PRALINE) || defined(UNIVERSAL)
		fpga_set_trigger_enable(&fpga, enable);
#endif
	}
}

void halt_and_flash(const uint32_t duration)
{
	/* blink LED1, LED2, and LED3 */
	while (1) {
		led_on(LED1);
		led_on(LED2);
		led_on(LED3);
		delay(duration);
		led_off(LED1);
		led_off(LED2);
		led_off(LED3);
		delay(duration);
	}
}

#if defined(PRALINE) || defined(UNIVERSAL)
void p1_ctrl_set(const p1_ctrl_signal_t signal)
{
	gpio_write(platform_gpio()->p1_ctrl0, signal & 1);
	gpio_write(platform_gpio()->p1_ctrl1, (signal >> 1) & 1);
	gpio_write(platform_gpio()->p1_ctrl2, (signal >> 2) & 1);
}

void p2_ctrl_set(const p2_ctrl_signal_t signal)
{
	gpio_write(platform_gpio()->p2_ctrl0, signal & 1);
	gpio_write(platform_gpio()->p2_ctrl1, (signal >> 1) & 1);
}

void clkin_ctrl_set(const clkin_signal_t signal)
{
	gpio_write(platform_gpio()->clkin_ctrl, signal & 1);
}

void pps_out_set(const uint8_t value)
{
	gpio_write(platform_gpio()->pps_out, value & 1);
}

void narrowband_filter_set(const uint8_t value)
{
	gpio_write(platform_gpio()->aa_en, value & 1);
}
#endif
