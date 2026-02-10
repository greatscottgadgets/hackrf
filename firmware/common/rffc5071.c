/*
 * Copyright 2012-2022 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2014 Jared Boone <jared@sharebrained.com>
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

/*
 * 'gcc -DTEST -DDEBUG -O2 -o test rffc5071.c' prints out what test
 * program would do if it had a real spi library
 */

/*
 * The actual part on Jawbreaker is the RFFC5072, not the RFFC5071, but the
 * RFFC5071 may be installed instead.  The only difference between the parts is
 * that the RFFC5071 includes a second mixer.
 */

#include <stdint.h>
#include <string.h>
#include "rffc5071.h"
#include "rffc5071_regs.def" // private register def macros
#include "selftest.h"

#include <libopencm3/lpc43xx/scu.h>
#include "hackrf_core.h"
#include "delay.h"

static bool enabled = false;

/* Default register values from vendor documentation or software. */
static const uint16_t rffc5071_regs_default[RFFC5071_NUM_REGS] = {
	0xfffb, /* 00 */
	0x4064, /* 01 */
	0x9055, /* 02 */
	0x2d02, /* 03 */
	0xb0bf, /* 04 */
	0xb0bf, /* 05 */
	0x0028, /* 06 */
	0x0028, /* 07 */
	0xfc06, /* 08 */
	0x8220, /* 09 */
	0x0202, /* 0A */
	0x0400, /* 0B */
	0x1a94, /* 0C */
	0xd89d, /* 0D */
	0x8900, /* 0E */
	0x1e84, /* 0F */
	0x89d8, /* 10 */
	0x9d00, /* 11 */
	0x2a80, /* 12 */
	0x0000, /* 13 */
	0x0000, /* 14 */
	0x0000, /* 15 */
	0x0206, /* 16 */
	0x4900, /* 17 */
	0x0281, /* 18 */
	0xf00f, /* 19 */
	0x0000, /* 1A */
	0x0000, /* 1B */
	0xc840, /* 1C */
	0x0000, /* 1D, readsel = 0b0000 */
	0x0005,
	/* 1E */};

/* Set up all registers according to defaults specified in docs. */
void rffc5071_init(rffc5071_driver_t* const drv)
{
	memcpy(drv->regs, rffc5071_regs_default, sizeof(drv->regs));
	drv->regs_dirty = 0x7fffffff;

	/* Write default register values to chip. */
	rffc5071_regs_commit(drv);

	selftest.mixer_id = rffc5071_reg_read(drv, RFFC5071_READBACK_REG);
	if ((selftest.mixer_id >> 3) != 4416) {
		selftest.report.pass = false;
	}
}

/*
 * Set up pins for GPIO and SPI control, configure SSP peripheral for SPI, and
 * set our own default register configuration.
 */
void rffc5071_setup(rffc5071_driver_t* const drv)
{
	gpio_set(drv->gpio_reset);
	gpio_output(drv->gpio_reset);

#ifdef PRALINE
	/* Configure mixer PLL lock detect pin */
	scu_pinmux(SCU_MIXER_LD, SCU_MIXER_LD_PINCFG);
	gpio_input(drv->gpio_ld);
#endif

	rffc5071_init(drv);

	/* zero low bits of fractional divider */
	set_RFFC5071_P2NLSB(drv, 0);

	/* set ENBL and MODE to be configured via 3-wire interface,
	 * not control pins. */
	set_RFFC5071_SIPIN(drv, 1);

	/* GPOs are active at all times */
	set_RFFC5071_GATE(drv, 1);

	/* mixer 2 used for both RX and TX */
	set_RFFC5071_FULLD(drv, 0);
	set_RFFC5071_MODE(drv, 1);

#if defined(PRALINE) || defined(HACKRF_ONE)
	/* Enable GPO Lock output signal */
	set_RFFC5071_LOCK(drv, 1);
#endif

	/* Enable reference oscillator standby */
	set_RFFC5071_REFST(drv, 1);

	/* Disable dither */
	set_RFFC5071_SDM(drv, 0b11);

	/* Maximize VCO warm-up time */
	set_RFFC5071_TVCO(drv, 31);

	rffc5071_regs_commit(drv);
}

void rffc5071_lock_test(rffc5071_driver_t* const drv)
{
	bool lock = false;

	for (int i = 0; i < NUM_LOCK_ATTEMPTS; i++) {
		// Tune to 100MHz.
		rffc5071_set_frequency(drv, 100000000);
		rffc5071_enable(drv);

		// Wait 1ms.
		delay_us_at_mhz(1000, 204);

		// Check for lock.
		lock = rffc5071_check_lock(drv);

		rffc5071_disable(drv);
		delay_us_at_mhz(100, 204);

		selftest.mixer_locks[i] = lock;
	}

	// The last attempt must be successful.
	if (!lock) {
		selftest.report.pass = false;
	}
}

bool rffc5071_check_lock(rffc5071_driver_t* const drv)
{
#ifdef PRALINE
	return gpio_read(drv->gpio_ld);
#else
	set_RFFC5071_READSEL(drv, 0b0001);
	rffc5071_regs_commit(drv);
	return !!(rffc5071_reg_read(drv, RFFC5071_READBACK_REG) & 0x8000);
#endif
}

static uint16_t rffc5071_spi_read(rffc5071_driver_t* const drv, uint8_t r)
{
	(void) drv;

	uint16_t data[] = {0x80 | (r & 0x7f), 0xffff};
	spi_bus_transfer(drv->bus, data, 2);
	return data[1];
}

static void rffc5071_spi_write(rffc5071_driver_t* const drv, uint8_t r, uint16_t v)
{
	(void) drv;

	uint16_t data[] = {0x00 | (r & 0x7f), v};
	spi_bus_transfer(drv->bus, data, 2);
}

uint16_t rffc5071_reg_read(rffc5071_driver_t* const drv, uint8_t r)
{
	/* Readback register is not cached. */
	if (r == RFFC5071_READBACK_REG) {
		return rffc5071_spi_read(drv, r);
	}

	/* Discard uncommited write when reading. This shouldn't
	 * happen, and probably has not been tested. */
	if ((drv->regs_dirty >> r) & 0x1) {
		drv->regs[r] = rffc5071_spi_read(drv, r);
	};
	return drv->regs[r];
}

void rffc5071_reg_write(rffc5071_driver_t* const drv, uint8_t r, uint16_t v)
{
	drv->regs[r] = v;
	rffc5071_spi_write(drv, r, v);
	RFFC5071_REG_SET_CLEAN(drv, r);
}

static inline void rffc5071_reg_commit(rffc5071_driver_t* const drv, uint8_t r)
{
	rffc5071_reg_write(drv, r, drv->regs[r]);
}

void rffc5071_regs_commit(rffc5071_driver_t* const drv)
{
	int r;
	for (r = 0; r < RFFC5071_NUM_REGS; r++) {
		if ((drv->regs_dirty >> r) & 0x1) {
			rffc5071_reg_commit(drv, r);
		}
	}
}

void rffc5071_disable(rffc5071_driver_t* const drv)
{
	if (enabled) {
		set_RFFC5071_ENBL(drv, 0);
		rffc5071_regs_commit(drv);
		enabled = false;
	}
}

void rffc5071_enable(rffc5071_driver_t* const drv)
{
	if (!enabled) {
		set_RFFC5071_ENBL(drv, 1);
		rffc5071_regs_commit(drv);
		enabled = true;
	}
}

#define FREQ_ONE_MHZ (1000ULL * 1000ULL)
#define REF_FREQ     (40 * FREQ_ONE_MHZ)
#define LO_MAX       (5400 * FREQ_ONE_MHZ)

/* configure frequency synthesizer (lo in Hz) */
uint64_t rffc5071_config_synth(rffc5071_driver_t* const drv, uint64_t lo)
{
	uint64_t fvco;
	uint8_t fbkdivlog;
	uint16_t n;
	uint64_t tune_freq_hz;
	uint16_t p1nmsb;
	uint8_t p1nlsb;

	/* Calculate n_lo (no division) */
	uint8_t n_lo = 0;
	uint64_t x = LO_MAX >> 1;
	while ((x >= lo) && (n_lo < 5)) {
		n_lo++;
		x >>= 1;
	}

	fvco = lo << n_lo;

	/*
	 * Higher charge pump leakage setting and fbkdivlog are required above
	 * 3.2 GHz.
	 */
	if (fvco > (3200 * FREQ_ONE_MHZ)) {
		fbkdivlog = 2;
		set_RFFC5071_PLLCPL(drv, 3);
	} else {
		fbkdivlog = 1;
		set_RFFC5071_PLLCPL(drv, 2);
	}

	uint64_t tmp_n = (fvco << (24ULL - fbkdivlog)) / REF_FREQ;

	/* Round to nearest step = ref_MHz / 2**s. For s=6, step=625000 Hz */
	/* This also ensures the lowest 22-s fractional bits are set to 0. */
	const uint8_t s = 6;
	const uint8_t d = (24 - fbkdivlog + n_lo) - s;
	tmp_n = ((tmp_n + (1 << (d - 1))) >> d) << d;

	n = tmp_n >> 24ULL;
	p1nmsb = (tmp_n >> 8ULL) & 0xffff;
	p1nlsb = tmp_n & 0xff;

	tune_freq_hz = (tmp_n * REF_FREQ) >> (24 - fbkdivlog + n_lo);

	/* Path 2 */
	set_RFFC5071_P2LODIV(drv, n_lo);
	set_RFFC5071_P2N(drv, n);
	set_RFFC5071_P2PRESC(drv, fbkdivlog);
	set_RFFC5071_P2NMSB(drv, p1nmsb);
	if (s > 14) {
		/* Only set when the step size is small enough. */
		set_RFFC5071_P2NLSB(drv, p1nlsb);
	}

	rffc5071_regs_commit(drv);

	return tune_freq_hz;
}

uint64_t rffc5071_set_frequency(rffc5071_driver_t* const drv, uint64_t hz)
{
	uint32_t tune_freq;

	tune_freq = rffc5071_config_synth(drv, hz);
	if (enabled) {
		set_RFFC5071_RELOK(drv, 1);
		rffc5071_regs_commit(drv);
	}

	return tune_freq;
}

void rffc5071_set_gpo(rffc5071_driver_t* const drv, uint8_t gpo)
{
	/* We set GPO for both paths just in case. */
	set_RFFC5071_P1GPO(drv, gpo);
	set_RFFC5071_P2GPO(drv, gpo);

	rffc5071_regs_commit(drv);
}

#ifdef PRALINE
bool rffc5071_poll_ld(rffc5071_driver_t* const drv, uint8_t* prelock_state)
{
	// The RFFC5072 can be configured to output PLL lock status on
	// GPO4. The lock detect signal is produced by a window detector
	// on the VCO tuning voltage. It goes high to show PLL lock when
	// the VCO tuning voltage is within the specified range, typically
	// 0.30V to 1.25V.
	//
	// During the tuning process the lock signal will often go high,
	// only to drop lock briefly before returning to the locked state.
	//
	// Therefore, to reliably detect lock it is necessary to also
	// track the state of the FSM that controls the tuning process.
	//
	// Before re-tuning begins, and after final lock has been
	// established, the FSM can be considered to be in STATE_LOCKED.
	//
	// The very first state change only occurs around 150us _after_ a
	// new frequency has been set and registers updated:
	//
	//   L        123456L   (STATE)
	//   ----___------_---  (LD)
	//
	// This means we need to track the state(s) that occur before
	// STATE_LOCKED to be able to reliably identify lock.
	//
	// At the time of writing 15 different states have been spotted in
	// the wild.
	//
	// The first six states occur at some point during most tuning
	// operations with the others occuring less frequently.
	//
	// Of the first six, two states have been identified as
	// STATE_PRELOCKn which, once entered, indicate that no further
	// changes will occur to the locked state.
	enum state {
		STATE_LOCKED = 0x17, // 0b10111
		STATE_00010 = 0x02,
		STATE_00100 = 0x04,
		STATE_01011 = 0x0b,
		STATE_PRELOCK1 = 0x10, // 0b10000
		STATE_PRELOCK2 = 0x1e, // 0b11110
		STATE_00000 = 0x00,
		STATE_00001 = 0x01, // mixer bypassed
		STATE_00011 = 0x03,
		STATE_00101 = 0x05,
		STATE_00110 = 0x06,
		STATE_00111 = 0x07,
		STATE_01010 = 0x0a,
		STATE_10110 = 0x16,
		STATE_11110 = 0x1e, // ?
		STATE_11111 = 0x1f,
		STATE_NONE = 0xff,
	};

	// Select which fields will be made available in the readback
	// register - we only need to do this the first time.
	if (*prelock_state == STATE_NONE) {
		set_RFFC5071_READSEL(drv, 0b0011);
		rffc5071_regs_commit(drv);
	}

	// read fsm state
	uint16_t rb = rffc5071_reg_read(drv, RFFC5071_READBACK_REG);
	uint8_t rsm_state = (rb >> 11) & 0b11111;

	// get gpo4 lock detect signal
	bool gpo4_ld = gpio_read(drv->gpio_ld);

	// parse state
	switch (rsm_state) {
	case STATE_LOCKED: // 'normal operation'
		if (gpo4_ld &&
		    ((*prelock_state == STATE_PRELOCK1) ||
		     (*prelock_state == STATE_PRELOCK2))) {
			return true;
		}
		break;
	case STATE_00010:
	case STATE_00100:
	case STATE_01011:
		break;
	case STATE_PRELOCK1:
		*prelock_state = rsm_state;
		break;
	case STATE_PRELOCK2:
		*prelock_state = rsm_state;
		break;
		// other states
	case STATE_00000:
	case STATE_00001:
	case STATE_00011:
	case STATE_00101:
	case STATE_00110:
	case STATE_00111:
	case STATE_01010:
	case STATE_10110:
	case STATE_11111:
	case STATE_NONE:
		break;
	default:
		// unknown state
		break;
	}

	return false;
}
#endif
