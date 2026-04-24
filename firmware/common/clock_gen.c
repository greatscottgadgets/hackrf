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

#include "clock_gen.h"

#include <stdint.h>

#include "drivers.h"
#include "hackrf_ui.h"
#include "i2c_bus.h"
#include "sgpio.h"
#include "si5351c.h"
#if defined(IS_HACKRF_ONE) || defined(IS_PRALINE)
	#include "delay.h"
	#include "platform_detect.h"
	#include "portapack.h"
#endif

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
