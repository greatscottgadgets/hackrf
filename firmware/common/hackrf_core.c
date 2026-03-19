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

#include <libopencm3/lpc43xx/memorymap.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/ssp.h>

#include "platform_detect.h"

#include "delay.h"
#include "gpio.h"
#include "hackrf_core.h"
#include "hackrf_ui.h"
#include "i2c_lpc.h"
#include "max283x.h"
#include "max5864_target.h"
#include "platform_gpio.h"
#include "platform_scu.h"
#include "spi_bus.h"
#include "w25q80bv_target.h"
#ifdef IS_EXPANSION_COMPATIBLE
   #include "portapack.h"
#endif
#ifdef IS_PRALINE
	#include "ice40_spi.h"
#endif

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

#ifdef IS_PRALINE
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

radio_t radio = {
	.sample_rate_cb = sample_rate_set,
};

rf_path_t rf_path;

jtag_gpio_t jtag_gpio_cpld;

jtag_t jtag_cpld = {
	.gpio = &jtag_gpio_cpld,
};

/*
 * Closest fraction to m/d with denominator <= max_den.
 * Returns result in *r / *s with gcd(*r, *s) == 1 and 0 < *s <= max_den.
 * Straight port from CPython's fractions, and better documented there.
 */
void limit_denominator(
	uint64_t m,
	uint64_t d,
	const uint64_t max_den,
	uint64_t* r,
	uint64_t* s)
{
	if (d <= max_den) {
		*r = m;
		*s = d;
		return;
	}

	uint64_t p0 = 0, q0 = 1, p1 = 1, q1 = 0;
	uint64_t n = m, orig_d = d;
	uint64_t tmp;

	while (1) {
		uint64_t a = n / d;
		uint64_t q2 = q0 + a * q1;

		if (q2 > max_den)
			break;

		tmp = p0 + a * p1;
		p0 = p1;
		q0 = q1;
		p1 = tmp;
		q1 = q2;

		tmp = n - a * d;
		n = d;
		d = tmp;
		if (d == 0)
			break;
	}

	uint64_t k = (max_den - q0) / q1;

	/* Return closer candidate. */
	if (2 * d * (q0 + k * q1) <= orig_d) {
		*r = p1;
		*s = q1;
	} else {
		*r = p0 + k * p1;
		*s = q0 + k * q1;
	}
}

/*
 * Configure clock generator to produce sample clock in units of 1/(2**36) Hz.
 * Can be called with program=false for a dry run that returns the resultant
 * frequency without actually configuring the clock generator.
 *
 * The clock generator output frequency is:
 *
 * fs = 128 * vco / (512 + p1 + p2/p3))
 *
 * where p1, p2, and p3 are register values.
 *
 * For more information see:
 * https://www.pa3fwm.nl/technotes/tn42a-si5351-programming.html
 */
fp_28_36_t sample_rate_set(const fp_28_36_t sample_rate, const bool program)
{
	const uint64_t vco_hz = 800 * 1000ULL * 1000ULL;
	uint64_t p1, p2, p3;
	uint64_t n, d, q1, q2, q3, r1, r2;
	fp_28_36_t resultant_rate;

	/*
	 * First double the sample rate so that we can produce a clock at twice
	 * the intended sample rate. The 2x clock is sometimes used directly,
	 * and it is divided by two in an output divider to produce the actual
	 * AFE clock.
	 */
	fp_28_36_t rate = sample_rate * 2;

	/*
	 * Computes p1 = (N << 36) / rate - 512, where N = 128 * vco_hz.
	 *
	 * Full numerator (N << 36) is 73 bits, so we split the division:
	 *
	 *   (N << 36) / rate = ((N << 27) / rate) << 9
	 *                    + (((N << 27) % rate) << 9) / rate
	 *
	 * IMPORTANT: Assumes sample rate is in [200e3 << 36, 43.6e6 << 36].
	 */
	const uint64_t A = (128 * vco_hz) << 27;

	q1 = A / rate;
	r1 = A % rate;

	// Remaining 9 bits with long division.
	q2 = 0;
	r2 = r1;
	for (int j = 0; j < 9; j++) {
		uint64_t msb = r2 >> 63;
		r2 <<= 1;
		q2 <<= 1;
		if (msb || r2 >= rate) {
			r2 -= rate;
			q2 |= 1;
		}
	}

	p1 = (q1 << 9) + q2 - 512;

	if (r2) {
		/* Use the remainder for the fractional part. */
		n = r2;
		d = rate;

		/* Reduce fraction. */
		const uint64_t p3_max = 0xfffff;
		limit_denominator(n, d, p3_max, &p2, &p3);

		/* Roll over to next p1 to enable integer mode. */
		if (p2 >= p3) {
			p1++;
			p2 = 0;
		}
	} else {
		p2 = 0;
	}

	/* Maximum: (128 * 2048) - 512 */
	if (p1 > 0x3fe00) {
		p1 = 0x3fe00;
		p2 = 0;
	}

	if (p2 == 0) {
		/* Use unity denominator for integer mode. */
		p3 = 1;
		n = (128 * vco_hz) << 18;
		d = (p1 + 512);
		q1 = n / d;
		r1 = n % d;
		q2 = ((r1 << 18) + (d / 2)) / d;
		resultant_rate = (q1 << 18) + q2;
	} else {
		n = p3 * vco_hz * 128;
		d = p3 * (p1 + 512) + p2;
		q1 = n / d;
		r1 = n % d;
		q2 = (r1 << 18) / d;
		r2 = (r1 << 18) % d;
		q3 = ((r2 << 18) + (d / 2)) / d;
		resultant_rate = (q1 << 36) + (q2 << 18) + q3;
	}

	/* Return MCU sample rate, not AFE clock rate. */
	resultant_rate = (resultant_rate + 1) / 2;

	if (!program) {
		return resultant_rate;
	}

	bool streaming = sgpio_cpld_stream_is_enabled(&sgpio_config);

	if (streaming) {
		sgpio_cpld_stream_disable(&sgpio_config);
	}

#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
		/* Integer mode can be enabled if p1 is even and p2 is zero. */
		if (p1 & 0x1 || p2) {
			si5351c_set_int_mode(&clock_gen, 0, 0);
		} else {
			si5351c_set_int_mode(&clock_gen, 0, 1);
		}

	#ifdef IS_H1_R9
		if (IS_H1_R9) {
			/*
			 * On HackRF One r9 all sample clocks are externally derived
			 * from MS1/CLK1 operating at twice the sample rate.
			 */
			si5351c_configure_multisynth(&clock_gen, 1, p1, p2, p3, 0);
		}
	#endif
	#ifdef IS_NOT_H1_R9
		if (IS_NOT_H1_R9) {
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
				0,
				0,
				0,
				0); //p1 doesn't matter

			/* MS0/CLK2 is the source for SGPIO (CODEC_X2_CLK) */
			si5351c_configure_multisynth(
				&clock_gen,
				2,
				0,
				0,
				0,
				0); //p1 doesn't matter
		}
	#endif
	}
#endif
#ifdef IS_PRALINE
	if (IS_PRALINE) {
		/* MS0/CLK0 is the source for the MAX5864 (AFE_CLK). */
		si5351c_configure_multisynth(&clock_gen, 0, p1, p2, p3, 1);

		/* MS1/CLK1 is the source for the FPGA (FPGA_CLK and SCT_CLK). */
		si5351c_configure_multisynth(&clock_gen, 1, p1, p2, p3, 1);

		/* Delay FPGA_CLK relative to AFE_CLK. */
		uint8_t phase_offset = 0;
		if (p1 < 2100) {
			phase_offset = (p1 >> 4) - 6;
		}
		si5351c_set_phase(&clock_gen, 1, phase_offset);

		if ((detected_revision() & ~BOARD_REV_GSG) < BOARD_REV_PRALINE_R1_1) {
			/*
			 * On older boards FPGA_CLK is on CLK2 while SCT_CLK is on
			 * CLK1. We configure both so that behavior is consistent with
			 * newer boards that use CLK1 for both FPGA_CLK and SCT_CLK.
			 */
			si5351c_configure_multisynth(&clock_gen, 2, p1, p2, p3, 1);
			si5351c_set_phase(&clock_gen, 2, phase_offset);
		}

		/* Reset PLL to synchronize output clock phase. */
		si5351c_reset_pll(&clock_gen, SI5351C_PLL_A);
	}
#endif

	if (streaming) {
		sgpio_cpld_stream_enable(&sgpio_config);
	}

	return resultant_rate;
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

#ifdef IS_H1_R9
	if (IS_H1_R9) {
		/* MS0/CLK0 is the reference for both RFFC5071 and MAX2839. */
		si5351c_configure_multisynth(
			&clock_gen,
			0,
			20 * 128 - 512,
			0,
			1,
			0); /* 800/20 = 40MHz */
	}
#endif
#ifdef IS_NOT_H1_R9
	if (IS_NOT_H1_R9) {
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
#endif

	/* MS6/CLK6 is unused. */
	/* MS7/CLK7 is unused. */

	/* Set to 10 MHz, the common rate between Jawbreaker and HackRF One. */
	sample_rate_set(SR_FP_MHZ(10), true);

	si5351c_configure_clock_control(&clock_gen);
	si5351c_set_clock_source(&clock_gen, PLL_SOURCE_XTAL);
	// soft reset
	si5351c_reset_pll(&clock_gen, SI5351C_PLL_BOTH);
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
#ifdef IS_EXPANSION_COMPATIBLE
	if (IS_EXPANSION_COMPATIBLE) {
		/* Ensure PortaPack reference oscillator is off while checking for external clock input. */
		if (portapack_reference_oscillator && portapack()) {
			portapack_reference_oscillator(false);
		}
	}
#endif

	clock_source_t source = CLOCK_SOURCE_HACKRF;

	/* Check for external clock input. */
	if (si5351c_clkin_signal_valid(&clock_gen)) {
		source = CLOCK_SOURCE_EXTERNAL;
	} else {
#ifdef IS_EXPANSION_COMPATIBLE
		if (IS_EXPANSION_COMPATIBLE) {
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

#ifdef IS_PRALINE
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
#ifdef IS_EXPANSION_COMPATIBLE
	if (IS_EXPANSION_COMPATIBLE) {
		scu_pinmux(scu->PINMUX_PP_TMS, SCU_GPIO_PUP | SCU_CONF_FUNCTION0);
		scu_pinmux(scu->PINMUX_PP_TDO, SCU_GPIO_PDN | SCU_CONF_FUNCTION0);
	}
#endif
	scu_pinmux(scu->PINMUX_CPLD_TCK, SCU_GPIO_PDN | SCU_CONF_FUNCTION0);
#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
		scu_pinmux(scu->PINMUX_CPLD_TMS, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
		scu_pinmux(scu->PINMUX_CPLD_TDI, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
		scu_pinmux(scu->PINMUX_CPLD_TDO, SCU_GPIO_PDN | SCU_CONF_FUNCTION4);
	}
#endif

	/* Configure SCU Pin Mux as GPIO */
	scu_pinmux(scu->PINMUX_LED1, SCU_GPIO_NOPULL);
	scu_pinmux(scu->PINMUX_LED2, SCU_GPIO_NOPULL);
	scu_pinmux(scu->PINMUX_LED3, SCU_GPIO_NOPULL);
#ifdef IS_RAD1O
	if (IS_RAD1O) {
		scu_pinmux(scu->PINMUX_LED4, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION4);
	}
#endif
#ifdef IS_PRALINE
	if (IS_PRALINE) {
		scu_pinmux(scu->PINMUX_LED4, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
	}
#endif

	/* Configure USB indicators */
#ifdef IS_JAWBREAKER
	if (IS_JAWBREAKER) {
		scu_pinmux(scu->PINMUX_USB_LED0, SCU_CONF_FUNCTION3);
		scu_pinmux(scu->PINMUX_USB_LED1, SCU_CONF_FUNCTION3);
	}
#endif

#ifdef IS_PRALINE
	if (IS_PRALINE) {
		disable_1v2_power();
		disable_3v3aux_power();
		gpio_output(gpio->gpio_1v2_enable);
		gpio_output(gpio->gpio_3v3aux_enable_n);
		scu_pinmux(scu->PINMUX_EN1V2, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		scu_pinmux(scu->PINMUX_EN3V3_AUX_N, SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
	}
#endif

#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
		disable_1v8_power();
	#ifdef IS_H1_R9
		if (IS_H1_R9) {
			gpio_output(gpio->h1r9_1v8_enable);
			scu_pinmux(scu->H1R9_EN1V8, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		}
	#endif
	#ifdef IS_NOT_H1_R9
		if (IS_NOT_H1_R9) {
			gpio_output(gpio->gpio_1v8_enable);
			scu_pinmux(scu->PINMUX_EN1V8, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
		}
	#endif
	}
#endif

#ifdef IS_H1_OR_PRALINE
	if (IS_H1_OR_PRALINE) {
		/* Safe state: start with VAA turned off: */
		disable_rf_power();

		/* Configure RF power supply (VAA) switch control signal as output */
	#ifdef IS_H1_R9
		if (IS_H1_R9) {
			gpio_output(gpio->h1r9_vaa_disable);
		}
	#endif
	#ifdef IS_NOT_H1_R9
		if (IS_NOT_H1_R9) {
			gpio_output(gpio->vaa_disable);
		}
	#endif
	}
#endif

#ifdef IS_RAD1O
	if (IS_RAD1O) {
		/* Safe state: start with VAA turned off: */
		disable_rf_power();

		/* Configure RF power supply (VAA) switch control signal as output */
		gpio_output(gpio->vaa_enable);

		/* Disable unused clock outputs. They generate noise. */
		scu_pinmux(CLK0, SCU_CLK_IN | SCU_CONF_FUNCTION7);
		scu_pinmux(CLK2, SCU_CLK_IN | SCU_CONF_FUNCTION7);

		scu_pinmux(scu->PINMUX_GPIO3_10, SCU_GPIO_PDN | SCU_CONF_FUNCTION0);
		scu_pinmux(scu->PINMUX_GPIO3_11, SCU_GPIO_PDN | SCU_CONF_FUNCTION0);
	}
#endif

#ifdef IS_PRALINE
	if (IS_PRALINE) {
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
	}
#endif

	/* enable input on SCL and SDA pins */
	SCU_SFSI2C0 = SCU_I2C0_NOMINAL;
}

/* Run after pin_shutdown() and prior to enabling power supplies. */
void pin_setup(void)
{
	/* Detect Platform */
	const platform_gpio_t* gpio = platform_gpio();
	const platform_scu_t* scu = platform_scu();

	/* Configure LEDs */
	led_off(0);
	led_off(1);
	led_off(2);
#ifdef IS_FOUR_LEDS
	if (IS_FOUR_LEDS) {
		led_off(3);
	}
#endif

	gpio_output(gpio->led[0]);
	gpio_output(gpio->led[1]);
	gpio_output(gpio->led[2]);

#ifdef IS_FOUR_LEDS
	if (IS_FOUR_LEDS) {
		gpio_output(gpio->led[3]);
	}
#endif

	/* Configure drivers and driver pins */
	ssp_config_max283x.gpio_select = gpio->max283x_select;
#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
		ssp_config_max283x.data_bits = SSP_DATA_16BITS;
	}
#endif
#ifdef IS_PRALINE
	if (IS_PRALINE) {
		ssp_config_max283x.data_bits = SSP_DATA_9BITS; // send 2 words
	}
#endif

	ssp_config_max5864.gpio_select = gpio->max5864_select;

	ssp_config_w25q80bv.gpio_select = gpio->w25q80bv_select;
	spi_flash.gpio_hold = gpio->w25q80bv_hold;
	spi_flash.gpio_wp = gpio->w25q80bv_wp;

	sgpio_config.gpio_q_invert = gpio->q_invert;

#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
		sgpio_config.gpio_trigger_enable = gpio->trigger_enable;
	}
#endif

#ifdef IS_PRALINE
	if (IS_PRALINE) {
		ssp_config_ice40_fpga.gpio_select = gpio->fpga_cfg_spi_cs;
		ice40.gpio_select = gpio->fpga_cfg_spi_cs;
		ice40.gpio_creset = gpio->fpga_cfg_creset;
		ice40.gpio_cdone = gpio->fpga_cfg_cdone;
	}
#endif

	jtag_gpio_cpld.gpio_tck = gpio->cpld_tck;

#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
		jtag_gpio_cpld.gpio_tms = gpio->cpld_tms;
		jtag_gpio_cpld.gpio_tdi = gpio->cpld_tdi;
		jtag_gpio_cpld.gpio_tdo = gpio->cpld_tdo;
	}
#endif

#ifdef IS_EXPANSION_COMPATIBLE
	if (IS_EXPANSION_COMPATIBLE) {
		jtag_gpio_cpld.gpio_pp_tms = gpio->cpld_pp_tms;
		jtag_gpio_cpld.gpio_pp_tdo = gpio->cpld_pp_tdo;
	}
#endif

	ssp1_set_mode_max283x();

	mixer_bus_setup(&mixer);

#ifdef IS_H1_R9
	if (IS_H1_R9) {
		sgpio_config.gpio_trigger_enable = gpio->h1r9_trigger_enable;
	}
#endif

	// initialize rf_path struct and assign gpio's
#ifdef IS_HACKRF_ONE
	if (IS_HACKRF_ONE) {
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
	#ifdef IS_H1_R9
		if (IS_H1_R9) {
			rf_path.gpio_rx = gpio->h1r9_rx;
			rf_path.gpio_h1r9_no_ant_pwr = gpio->h1r9_no_ant_pwr;
		}
	#endif
	}
#endif

#ifdef IS_RAD1O
	if (IS_RAD1O) {
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
	}
#endif

#ifdef IS_PRALINE
	if (IS_PRALINE) {
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
	}
#endif

	rf_path_pin_setup(&rf_path);

	/* Configure external clock in */
	scu_pinmux(scu->PINMUX_GP_CLKIN, SCU_CLK_IN | SCU_CONF_FUNCTION1);

	sgpio_configure_pin_functions(&sgpio_config);
}

#ifdef IS_PRALINE
void enable_1v2_power(void)
{
	if (IS_PRALINE) {
		gpio_set(platform_gpio()->gpio_1v2_enable);
	}
}

void disable_1v2_power(void)
{
	if (IS_PRALINE) {
		gpio_clear(platform_gpio()->gpio_1v2_enable);
	}
}

void enable_3v3aux_power(void)
{
	if (IS_PRALINE) {
		gpio_clear(platform_gpio()->gpio_3v3aux_enable_n);
	}
}

void disable_3v3aux_power(void)
{
	if (IS_PRALINE) {
		gpio_set(platform_gpio()->gpio_3v3aux_enable_n);
	}
}
#endif

void enable_1v8_power(void)
{
#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
	#ifdef IS_H1_R9
		if (IS_H1_R9) {
			gpio_set(platform_gpio()->h1r9_1v8_enable);
		}
	#endif
	#ifdef IS_NOT_H1_R9
		if (IS_NOT_H1_R9) {
			gpio_set(platform_gpio()->gpio_1v8_enable);
		}
	#endif
	}
#endif
}

void disable_1v8_power(void)
{
#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
	#ifdef IS_H1_R9
		if (IS_H1_R9) {
			gpio_clear(platform_gpio()->h1r9_1v8_enable);
		}
	#endif
	#ifdef IS_NOT_H1_R9
		if (IS_NOT_H1_R9) {
			gpio_clear(platform_gpio()->gpio_1v8_enable);
		}
	#endif
	}
#endif
}

#ifdef IS_HACKRF_ONE
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

#ifdef IS_PRALINE
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

#ifdef IS_RAD1O
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
#ifdef IS_HACKRF_ONE
	if (IS_HACKRF_ONE) {
		enable_rf_power_hackrf_one();
	}
#endif
#ifdef IS_PRALINE
	if (IS_PRALINE) {
		enable_rf_power_praline();
	}
#endif
#ifdef IS_RAD1O
	if (IS_RAD1O) {
		enable_rf_power_rad1o();
	}
#endif
}

void disable_rf_power(void)
{
#ifdef IS_HACKRF_ONE
	if (IS_HACKRF_ONE) {
		disable_rf_power_hackrf_one();
	}
#endif
#ifdef IS_PRALINE
	if (IS_PRALINE) {
		disable_rf_power_praline();
	}
#endif
#ifdef IS_RAD1O
	if (IS_RAD1O) {
		disable_rf_power_rad1o();
	}
#endif
}

void led_on(const led_t led)
{
#ifdef IS_PRALINE
	if (IS_PRALINE) {
		gpio_clear(platform_gpio()->led[led]);
	}
#endif
#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
		gpio_set(platform_gpio()->led[led]);
	}
#endif
}

void led_off(const led_t led)
{
#ifdef IS_PRALINE
	if (IS_PRALINE) {
		gpio_set(platform_gpio()->led[led]);
	}
#endif
#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
		gpio_clear(platform_gpio()->led[led]);
	}
#endif
}

void led_toggle(const led_t led)
{
	gpio_toggle(platform_gpio()->led[led]);
}

void set_leds(const uint8_t state)
{
	int num_leds = 3;
#ifdef IS_FOUR_LEDS
	if (IS_FOUR_LEDS) {
		num_leds = 4;
	}
#endif

	for (int i = 0; i < num_leds; i++) {
#ifdef IS_PRALINE
		if (IS_PRALINE) {
			gpio_write(platform_gpio()->led[i], ((state >> i) & 1) == 0);
		}
#endif
#ifdef IS_NOT_PRALINE
		if (IS_NOT_PRALINE) {
			gpio_write(platform_gpio()->led[i], ((state >> i) & 1) == 1);
		}
#endif
	}
}

void trigger_enable(const bool enable)
{
#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
		gpio_write(sgpio_config.gpio_trigger_enable, enable);
	}
#endif
#ifdef IS_PRALINE
	if (IS_PRALINE) {
		fpga_set_trigger_enable(&fpga, enable);
	}
#endif
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

#ifdef IS_PRALINE
void p1_ctrl_set(const p1_ctrl_signal_t signal)
{
	const platform_gpio_t* gpio = platform_gpio();

	gpio_write(gpio->p1_ctrl0, signal & 1);
	gpio_write(gpio->p1_ctrl1, (signal >> 1) & 1);
	gpio_write(gpio->p1_ctrl2, (signal >> 2) & 1);
}

void p2_ctrl_set(const p2_ctrl_signal_t signal)
{
	const platform_gpio_t* gpio = platform_gpio();

	gpio_write(gpio->p2_ctrl0, signal & 1);
	gpio_write(gpio->p2_ctrl1, (signal >> 1) & 1);
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
