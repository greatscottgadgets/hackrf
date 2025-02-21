/*
 * Copyright 2012-2022 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2012 Jared Boone <jared@sharebrained.com>
 * Copyright 2013 Benjamin Vernoux <titanmkd@gmail.com>
 * Copyright 2025 Fabrizio Pollastri <mxgbot@gmail.com>
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
#include "sgpio.h"
#include "si5351c.h"
#include "spi_ssp.h"
#include "max283x.h"
#include "max5864.h"
#include "max5864_target.h"
#include "w25q80bv.h"
#include "w25q80bv_target.h"
#include "i2c_bus.h"
#include "i2c_lpc.h"
#include "cpld_jtag.h"
#include "platform_detect.h"
#include "clkin.h"
#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/lpc43xx/ccu.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/ssp.h>

#ifdef HACKRF_ONE
	#include "portapack.h"
#endif

#include "gpio_lpc.h"

/* GPIO Output PinMux */
static struct gpio_t gpio_led[] = {
	GPIO(2, 1),
	GPIO(2, 2),
	GPIO(2, 8),
#ifdef RAD1O
	GPIO(5, 26),
#endif
};

// clang-format off
static struct gpio_t gpio_1v8_enable        = GPIO(3,  6);

/* MAX283x GPIO (XCVR_CTL) PinMux */
static struct gpio_t gpio_max283x_select    = GPIO(0, 15);

/* MAX5864 SPI chip select (AD_CS) GPIO PinMux */
static struct gpio_t gpio_max5864_select    = GPIO(2,  7);

/* RFFC5071 GPIO serial interface PinMux */
// #ifdef RAD1O
// static struct gpio_t gpio_rffc5072_select   = GPIO(2, 13);
// static struct gpio_t gpio_rffc5072_clock    = GPIO(5,  6);
// static struct gpio_t gpio_rffc5072_data     = GPIO(3,  3);
// static struct gpio_t gpio_rffc5072_reset    = GPIO(2, 14);
// #endif

/* RF supply (VAA) control */
#ifdef HACKRF_ONE
static struct gpio_t gpio_vaa_disable       = GPIO(2, 9);
#endif
#ifdef RAD1O
static struct gpio_t gpio_vaa_enable        = GPIO(2, 9);
#endif

static struct gpio_t gpio_w25q80bv_hold     = GPIO(1, 14);
static struct gpio_t gpio_w25q80bv_wp       = GPIO(1, 15);
static struct gpio_t gpio_w25q80bv_select   = GPIO(5, 11);

/* RF switch control */
#ifdef HACKRF_ONE
static struct gpio_t gpio_hp                = GPIO(2,  0);
static struct gpio_t gpio_lp                = GPIO(2, 10);
static struct gpio_t gpio_tx_mix_bp         = GPIO(2, 11);
static struct gpio_t gpio_no_mix_bypass     = GPIO(1,  0);
static struct gpio_t gpio_rx_mix_bp         = GPIO(2, 12);
static struct gpio_t gpio_tx_amp            = GPIO(2, 15);
static struct gpio_t gpio_tx                = GPIO(5, 15);
static struct gpio_t gpio_mix_bypass        = GPIO(5, 16);
static struct gpio_t gpio_rx                = GPIO(5,  5);
static struct gpio_t gpio_no_tx_amp_pwr     = GPIO(3,  5);
static struct gpio_t gpio_amp_bypass        = GPIO(0, 14);
static struct gpio_t gpio_rx_amp            = GPIO(1, 11);
static struct gpio_t gpio_no_rx_amp_pwr     = GPIO(1, 12);
#endif
#ifdef RAD1O
static struct gpio_t gpio_tx_rx_n           = GPIO(1,  11);
static struct gpio_t gpio_tx_rx             = GPIO(0,  14);
static struct gpio_t gpio_by_mix            = GPIO(1,  12);
static struct gpio_t gpio_by_mix_n          = GPIO(2,  10);
static struct gpio_t gpio_by_amp            = GPIO(1,  0);
static struct gpio_t gpio_by_amp_n          = GPIO(5,  5);
static struct gpio_t gpio_mixer_en          = GPIO(5,  16);
static struct gpio_t gpio_low_high_filt     = GPIO(2,  11);
static struct gpio_t gpio_low_high_filt_n   = GPIO(2,  12);
static struct gpio_t gpio_tx_amp            = GPIO(2,  15);
static struct gpio_t gpio_rx_lna            = GPIO(5,  15);
#endif

/* CPLD JTAG interface GPIO pins */
static struct gpio_t gpio_cpld_tdo          = GPIO(5, 18);
static struct gpio_t gpio_cpld_tck          = GPIO(3,  0);
#if (defined HACKRF_ONE || defined RAD1O)
static struct gpio_t gpio_cpld_tms          = GPIO(3,  4);
static struct gpio_t gpio_cpld_tdi          = GPIO(3,  1);
#else
static struct gpio_t gpio_cpld_tms          = GPIO(3,  1);
static struct gpio_t gpio_cpld_tdi          = GPIO(3,  4);
#endif

#ifdef HACKRF_ONE
static struct gpio_t gpio_cpld_pp_tms       = GPIO(1,  1);
static struct gpio_t gpio_cpld_pp_tdo       = GPIO(1,  8);
#endif

/* other CPLD interface GPIO pins */
static struct gpio_t gpio_hw_sync_enable    = GPIO(5, 12);
static struct gpio_t gpio_q_invert          = GPIO(0, 13);

/* HackRF One r9 */
#ifdef HACKRF_ONE
static struct gpio_t gpio_h1r9_rx             = GPIO(0, 7);
static struct gpio_t gpio_h1r9_1v8_enable     = GPIO(2, 9);
static struct gpio_t gpio_h1r9_vaa_disable    = GPIO(3, 6);
static struct gpio_t gpio_h1r9_hw_sync_enable = GPIO(5, 5);
#endif
// clang-format on

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

const ssp_config_t ssp_config_max283x = {
	/* FIXME speed up once everything is working reliably */
	/*
	// Freq About 0.0498MHz / 49.8KHz => Freq = PCLK / (CPSDVSR * [SCR+1]) with PCLK=PLL1=204MHz
	const uint8_t serial_clock_rate = 32;
	const uint8_t clock_prescale_rate = 128;
	*/
	// Freq About 4.857MHz => Freq = PCLK / (CPSDVSR * [SCR+1]) with PCLK=PLL1=204MHz
	.data_bits = SSP_DATA_16BITS,
	.serial_clock_rate = 21,
	.clock_prescale_rate = 2,
	.gpio_select = &gpio_max283x_select,
};

const ssp_config_t ssp_config_max5864 = {
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
	.gpio_select = &gpio_max5864_select,
};

spi_bus_t spi_bus_ssp1 = {
	.obj = (void*) SSP1_BASE,
	.config = &ssp_config_max5864,
	.start = spi_ssp_start,
	.stop = spi_ssp_stop,
	.transfer = spi_ssp_transfer,
	.transfer_gather = spi_ssp_transfer_gather,
};

max283x_driver_t max283x = {};

max5864_driver_t max5864 = {
	.bus = &spi_bus_ssp1,
	.target_init = max5864_target_init,
};

const ssp_config_t ssp_config_w25q80bv = {
	.data_bits = SSP_DATA_8BITS,
	.serial_clock_rate = 2,
	.clock_prescale_rate = 2,
	.gpio_select = &gpio_w25q80bv_select,
};

spi_bus_t spi_bus_ssp0 = {
	.obj = (void*) SSP0_BASE,
	.config = &ssp_config_w25q80bv,
	.start = spi_ssp_start,
	.stop = spi_ssp_stop,
	.transfer = spi_ssp_transfer,
	.transfer_gather = spi_ssp_transfer_gather,
};

w25q80bv_driver_t spi_flash = {
	.bus = &spi_bus_ssp0,
	.gpio_hold = &gpio_w25q80bv_hold,
	.gpio_wp = &gpio_w25q80bv_wp,
	.target_init = w25q80bv_target_init,
};

sgpio_config_t sgpio_config = {
	.gpio_q_invert = &gpio_q_invert,
	.gpio_hw_sync_enable = &gpio_hw_sync_enable,
	.slice_mode_multislice = true,
};

rf_path_t rf_path = {
	.switchctrl = 0,
#ifdef HACKRF_ONE
	.gpio_hp = &gpio_hp,
	.gpio_lp = &gpio_lp,
	.gpio_tx_mix_bp = &gpio_tx_mix_bp,
	.gpio_no_mix_bypass = &gpio_no_mix_bypass,
	.gpio_rx_mix_bp = &gpio_rx_mix_bp,
	.gpio_tx_amp = &gpio_tx_amp,
	.gpio_tx = &gpio_tx,
	.gpio_mix_bypass = &gpio_mix_bypass,
	.gpio_rx = &gpio_rx,
	.gpio_no_tx_amp_pwr = &gpio_no_tx_amp_pwr,
	.gpio_amp_bypass = &gpio_amp_bypass,
	.gpio_rx_amp = &gpio_rx_amp,
	.gpio_no_rx_amp_pwr = &gpio_no_rx_amp_pwr,
#endif
#ifdef RAD1O
	.gpio_tx_rx_n = &gpio_tx_rx_n,
	.gpio_tx_rx = &gpio_tx_rx,
	.gpio_by_mix = &gpio_by_mix,
	.gpio_by_mix_n = &gpio_by_mix_n,
	.gpio_by_amp = &gpio_by_amp,
	.gpio_by_amp_n = &gpio_by_amp_n,
	.gpio_mixer_en = &gpio_mixer_en,
	.gpio_low_high_filt = &gpio_low_high_filt,
	.gpio_low_high_filt_n = &gpio_low_high_filt_n,
	.gpio_tx_amp = &gpio_tx_amp,
	.gpio_rx_lna = &gpio_rx_lna,
#endif
};

jtag_gpio_t jtag_gpio_cpld = {
	.gpio_tms = &gpio_cpld_tms,
	.gpio_tck = &gpio_cpld_tck,
	.gpio_tdi = &gpio_cpld_tdi,
	.gpio_tdo = &gpio_cpld_tdo,
#ifdef HACKRF_ONE
	.gpio_pp_tms = &gpio_cpld_pp_tms,
	.gpio_pp_tdo = &gpio_cpld_pp_tdo,
#endif
};

jtag_t jtag_cpld = {
	.gpio = &jtag_gpio_cpld,
};

void delay(uint32_t duration)
{
	uint32_t i;

	for (i = 0; i < duration; i++) {
		__asm__("nop");
	}
}

void delay_us_at_mhz(uint32_t us, uint32_t mhz)
{
	// The loop below takes 3 cycles per iteration.
	uint32_t loop_iterations = (us * mhz) / 3;
	asm volatile("start%=:\n"
		     "    subs %[ITERATIONS], #1\n" // 1 cycle
		     "    bpl start%=\n"            // 2 cycles
		     :
		     : [ITERATIONS] "r"(loop_iterations));
}

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

	hackrf_ui()->set_sample_rate(rate_num / 2);

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
	} else {
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
	} else {
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
	}

	return true;
}

bool baseband_filter_bandwidth_set(const uint32_t bandwidth_hz)
{
	uint32_t bandwidth_hz_real;
	bandwidth_hz_real = max283x_set_lpf_bandwidth(&max283x, bandwidth_hz);

	if (bandwidth_hz_real) {
		hackrf_ui()->set_filter_bw(bandwidth_hz_real);
	}

	return bandwidth_hz_real != 0;
}

/*
Configure PLL1 (Main MCU Clock) to max speed (204MHz).
Note: PLL1 clock is used by M4/M0 core, Peripheral, APB1.
This function shall be called after cpu_clock_init().
*/
void cpu_clock_pll1_max_speed(uint8_t clock_source,uint8_t msel)
{
	uint32_t reg_val;

	/* This function implements the sequence recommended in:
	 * UM10503 Rev 2.4 (Aug 2018), section 13.2.1.1, page 167. */

	/* 1. Select the IRC as BASE_M4_CLK source. */
	reg_val = CGU_BASE_M4_CLK;
	reg_val &= ~CGU_BASE_M4_CLK_CLK_SEL_MASK;
	reg_val |= CGU_BASE_M4_CLK_CLK_SEL(CGU_SRC_IRC) | CGU_BASE_M4_CLK_AUTOBLOCK(1);
	CGU_BASE_M4_CLK = reg_val;

	/* 2. if required, enable the crystal oscillator. */
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
	/* Set PLL1 up to 12MHz * 17 = 204MHz or 10Mhz * 20 = 200MHz.
	 * Direct mode: FCLKOUT = FCCO = M*(FCLKIN/N) */
	reg_val |= CGU_PLL1_CTRL_CLK_SEL(clock_source) |
	           CGU_PLL1_CTRL_PSEL(0) |
	           CGU_PLL1_CTRL_NSEL(0) |
	           CGU_PLL1_CTRL_MSEL(msel-1) |
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
void cpu_clock_init(uint8_t clock_source,uint8_t msel)
{
	/* use IRC as clock source for APB1 (including I2C0) */
	CGU_BASE_APB1_CLK = CGU_BASE_APB1_CLK_CLK_SEL(CGU_SRC_IRC);

	/* use IRC as clock source for APB3 */
	CGU_BASE_APB3_CLK = CGU_BASE_APB3_CLK_CLK_SEL(CGU_SRC_IRC);

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
	 *   CLK0 -> RFFC5072/MAX2839 (40 MHz tuner)
	 *   CLK1 -> MAX5864/CPLD/SGPIO (sample clock x 2)
	 *   CLK2 -> External Clock Output/LPC43xx (power down at boot)
	 *
	 * Clocks on other platforms:
	 *   CLK0 -> MAX5864/CPLD (sample clock)
	 *   CLK1 -> CPLD (sample clock x 2)
	 *   CLK2 -> SGPIO (sample clock x 2)
	 *   CLK3 -> External Clock Output (power down at boot)
	 *   CLK4 -> RFFC5072 (MAX2837 on rad1o) (40 MHz tuner)
	 *   CLK5 -> MAX2837 (MAX2871 on rad1o) (40 MHz tuner)
	 *   CLK6 -> none
	 *   CLK7 -> LPC43xx (uses a 12MHz crystal by default)
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

	cpu_clock_pll1_max_speed(clock_source,msel);

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

#if (defined JAWBREAKER || defined HACKRF_ONE)
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
	CGU_BASE_SPIFI_CLK = CGU_BASE_SPIFI_CLK_PD(1); /* SPIFI is only used at boot */
	CGU_BASE_USB1_CLK = CGU_BASE_USB1_CLK_PD(1);   /* USB1 is not exposed on HackRF */
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
	CCU1_CLK_APB3_ADC0_CFG = 0;
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
	//CCU1_CLK_M4_TIMER3_CFG = 0;
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
#endif
}

clock_source_t activate_best_clock_source(void)
{
#ifdef HACKRF_ONE
	/* Ensure PortaPack reference oscillator is off while checking for external clock input. */
	if (portapack_reference_oscillator && portapack()) {
		portapack_reference_oscillator(false);
	}
#endif

	clock_source_t source = CLOCK_SOURCE_HACKRF;

	/* Check for external clock input. */
	if (si5351c_clkin_signal_valid(&clock_gen)) {
		source = CLOCK_SOURCE_EXTERNAL;
	} else {
#ifdef HACKRF_ONE
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

void pin_setup(void)
{
	/* Configure all GPIO as Input (safe state) */
	gpio_init();

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

        /* configure pin as TIMER 3 MATCH 0 output: pps out */
        scu_pinmux(SCU_PINMUX_PPS, SCU_GPIO_PDN | SCU_CONF_FUNCTION6);
        /* configure pin as TIMER 3 MATCH 1 output: sampling trigger out */
        scu_pinmux(SCU_PINMUX_SAMP_TRIGGER, SCU_GPIO_PDN | SCU_CONF_FUNCTION6);

#ifdef HACKRF_ONE
	scu_pinmux(SCU_PINMUX_PP_TMS, SCU_GPIO_PUP | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_PP_TDO, SCU_GPIO_PDN | SCU_CONF_FUNCTION0);
#endif
	scu_pinmux(SCU_PINMUX_CPLD_TMS, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_CPLD_TDI, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_CPLD_TDO, SCU_GPIO_PDN | SCU_CONF_FUNCTION4);
	scu_pinmux(SCU_PINMUX_CPLD_TCK, SCU_GPIO_PDN | SCU_CONF_FUNCTION0);

	/* Configure SCU Pin Mux as GPIO */
	scu_pinmux(SCU_PINMUX_LED1, SCU_GPIO_NOPULL);
	scu_pinmux(SCU_PINMUX_LED2, SCU_GPIO_NOPULL);
	scu_pinmux(SCU_PINMUX_LED3, SCU_GPIO_NOPULL);
#ifdef RAD1O
	scu_pinmux(SCU_PINMUX_LED4, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION4);
#endif

	/* Configure USB indicators */
#ifdef JAWBREAKER
	scu_pinmux(SCU_PINMUX_USB_LED0, SCU_CONF_FUNCTION3);
	scu_pinmux(SCU_PINMUX_USB_LED1, SCU_CONF_FUNCTION3);
#endif

	gpio_output(&gpio_led[0]);
	gpio_output(&gpio_led[1]);
	gpio_output(&gpio_led[2]);
#ifdef RAD1O
	gpio_output(&gpio_led[3]);
#endif

	disable_1v8_power();
	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
#ifdef HACKRF_ONE
		gpio_output(&gpio_h1r9_1v8_enable);
		scu_pinmux(SCU_H1R9_EN1V8, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
#endif
	} else {
		gpio_output(&gpio_1v8_enable);
		scu_pinmux(SCU_PINMUX_EN1V8, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
	}

#ifdef HACKRF_ONE
	/* Safe state: start with VAA turned off: */
	disable_rf_power();

	/* Configure RF power supply (VAA) switch control signal as output */
	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
		gpio_output(&gpio_h1r9_vaa_disable);
	} else {
		gpio_output(&gpio_vaa_disable);
	}
#endif

#ifdef RAD1O
	/* Safe state: start with VAA turned off: */
	disable_rf_power();

	/* Configure RF power supply (VAA) switch control signal as output */
	gpio_output(&gpio_vaa_enable);

	/* Disable unused clock outputs. They generate noise. */
	scu_pinmux(CLK0, SCU_CLK_IN | SCU_CONF_FUNCTION7);
	scu_pinmux(CLK2, SCU_CLK_IN | SCU_CONF_FUNCTION7);

	scu_pinmux(SCU_PINMUX_GPIO3_10, SCU_GPIO_PDN | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_GPIO3_11, SCU_GPIO_PDN | SCU_CONF_FUNCTION0);

#endif

	/* enable input on SCL and SDA pins */
	SCU_SFSI2C0 = SCU_I2C0_NOMINAL;

	spi_bus_start(&spi_bus_ssp1, &ssp_config_max283x);

	mixer_bus_setup(&mixer);

#ifdef HACKRF_ONE
	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
		rf_path.gpio_rx = &gpio_h1r9_rx;
		sgpio_config.gpio_hw_sync_enable = &gpio_h1r9_hw_sync_enable;
	}
#endif
	rf_path_pin_setup(&rf_path);

	/* Configure external clock in */
	if (detected_platform() == BOARD_ID_HACKRF1_R9)
		scu_pinmux(SCU_PINMUX_GP_CLKIN_R9, SCU_CLK_IN | SCU_CONF_FUNCTION1);
	else
		scu_pinmux(SCU_PINMUX_GP_CLKIN_NOTR9, SCU_CLK_IN | SCU_CONF_FUNCTION1);


	sgpio_configure_pin_functions(&sgpio_config);
}

void enable_1v8_power(void)
{
	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
#ifdef HACKRF_ONE
		gpio_set(&gpio_h1r9_1v8_enable);
#endif
	} else {
		gpio_set(&gpio_1v8_enable);
	}
}

void disable_1v8_power(void)
{
	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
#ifdef HACKRF_ONE
		gpio_clear(&gpio_h1r9_1v8_enable);
#endif
	} else {
		gpio_clear(&gpio_1v8_enable);
	}
}

#ifdef HACKRF_ONE
void enable_rf_power(void)
{
	uint32_t i;

	/* many short pulses to avoid one big voltage glitch */
	for (i = 0; i < 1000; i++) {
		if (detected_platform() == BOARD_ID_HACKRF1_R9) {
			gpio_set(&gpio_h1r9_vaa_disable);
			gpio_clear(&gpio_h1r9_vaa_disable);
		} else {
			gpio_set(&gpio_vaa_disable);
			gpio_clear(&gpio_vaa_disable);
		}
	}
}

void disable_rf_power(void)
{
	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
		gpio_set(&gpio_h1r9_vaa_disable);
	} else {
		gpio_set(&gpio_vaa_disable);
	}
}
#endif

#ifdef RAD1O
void enable_rf_power(void)
{
	gpio_set(&gpio_vaa_enable);

	/* Let the voltage stabilize */
	delay(1000000);
}

void disable_rf_power(void)
{
	gpio_clear(&gpio_vaa_enable);
}
#endif

void led_on(const led_t led)
{
	gpio_set(&gpio_led[led]);
}

void led_off(const led_t led)
{
	gpio_clear(&gpio_led[led]);
}

void led_toggle(const led_t led)
{
	gpio_toggle(&gpio_led[led]);
}

void set_leds(const uint8_t state)
{
	int num_leds = 3;
#ifdef RAD1O
	num_leds = 4;
#endif
	for (int i = 0; i < num_leds; i++) {
		gpio_write(&gpio_led[i], ((state >> i) & 1) == 1);
	}
}

void hw_sync_enable(const hw_sync_mode_t hw_sync_mode)
{
	gpio_write(sgpio_config.gpio_hw_sync_enable, hw_sync_mode == 1);
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
