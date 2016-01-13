/*
 * Copyright 2012 Michael Ossmann
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

#include "hackrf_core.h"

/* Default register values. */
static const uint16_t rffc5071_regs_default[RFFC5071_NUM_REGS] = { 
	0xbefa,   /* 00 */
	0x4064,   /* 01 */
	0x9055,   /* 02 */
	0x2d02,   /* 03 */
	0xacbf,   /* 04 */
	0xacbf,   /* 05 */
	0x0028,   /* 06 */
	0x0028,   /* 07 */
	0xff00,   /* 08 */
	0x8220,   /* 09 */
	0x0202,   /* 0A */
	0x4800,   /* 0B */
	0x1a94,   /* 0C */
	0xd89d,   /* 0D */
	0x8900,   /* 0E */
	0x1e84,   /* 0F */
	0x89d8,   /* 10 */
	0x9d00,   /* 11 */
	0x2a20,   /* 12 */
	0x0000,   /* 13 */
	0x0000,   /* 14 */
	0x0000,   /* 15 */
	0x0000,   /* 16 */
	0x4900,   /* 17 */
	0x0281,   /* 18 */
	0xf00f,   /* 19 */
	0x0000,   /* 1A */
	0x0000,   /* 1B */
	0xc840,   /* 1C */
	0x1000,   /* 1D */
	0x0005,   /* 1E */ };

/* Set up all registers according to defaults specified in docs. */
void rffc5071_init(rffc5071_driver_t* const drv)
{
	memcpy(drv->regs, rffc5071_regs_default, sizeof(drv->regs));
	drv->regs_dirty = 0x7fffffff;

	/* Write default register values to chip. */
	rffc5071_regs_commit(drv);
}

/*
 * Set up pins for GPIO and SPI control, configure SSP peripheral for SPI, and
 * set our own default register configuration.
 */
void rffc5071_setup(rffc5071_driver_t* const drv)
{
	gpio_set(drv->gpio_reset);
	gpio_output(drv->gpio_reset);

	rffc5071_init(drv);

	/* initial setup */
	/* put zeros in freq contol registers */
	set_RFFC5071_P2N(drv, 0);
	set_RFFC5071_P2LODIV(drv, 0);
	set_RFFC5071_P2PRESC(drv, 0);
	set_RFFC5071_P2VCOSEL(drv, 0);

	set_RFFC5071_P2N(drv, 0);
	set_RFFC5071_P2LODIV(drv, 0);
	set_RFFC5071_P2PRESC(drv, 0);
	set_RFFC5071_P2VCOSEL(drv, 0);

	set_RFFC5071_P2N(drv, 0);
	set_RFFC5071_P2LODIV(drv, 0);
	set_RFFC5071_P2PRESC(drv, 0);
	set_RFFC5071_P2VCOSEL(drv, 0);

	/* set ENBL and MODE to be configured via 3-wire interface,
	 * not control pins. */
	set_RFFC5071_SIPIN(drv, 1);

	/* GPOs are active at all times */
	set_RFFC5071_GATE(drv, 1);

	rffc5071_regs_commit(drv);
}

static uint16_t rffc5071_spi_read(rffc5071_driver_t* const drv, uint8_t r) {
	(void)drv;

	uint16_t data[] = { 0x80 | (r & 0x7f), 0xffff };
	spi_bus_transfer(drv->bus, data, 2);
	return data[1];
}

static void rffc5071_spi_write(rffc5071_driver_t* const drv, uint8_t r, uint16_t v) {
	(void)drv;

	uint16_t data[] = { 0x00 | (r & 0x7f), v };
	spi_bus_transfer(drv->bus, data, 2);
}

uint16_t rffc5071_reg_read(rffc5071_driver_t* const drv, uint8_t r)
{
	/* Readback register is not cached. */
	if (r == RFFC5071_READBACK_REG)
		return rffc5071_spi_read(drv, r);

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
	for(r = 0; r < RFFC5071_NUM_REGS; r++) {
		if ((drv->regs_dirty >> r) & 0x1) {
			rffc5071_reg_commit(drv, r);
		}
	}
}

void rffc5071_tx(rffc5071_driver_t* const drv) {
	set_RFFC5071_ENBL(drv, 0);
	set_RFFC5071_FULLD(drv, 0);
	set_RFFC5071_MODE(drv, 1); /* mixer 2 used for both RX and TX */
	rffc5071_regs_commit(drv);
}

void rffc5071_rx(rffc5071_driver_t* const drv) {
	set_RFFC5071_ENBL(drv, 0);
	set_RFFC5071_FULLD(drv, 0);
	set_RFFC5071_MODE(drv, 1); /* mixer 2 used for both RX and TX */
	rffc5071_regs_commit(drv);
}

/*
 * This function turns on both mixer (full-duplex) on the RFFC5071, but our
 * current hardware designs do not support full-duplex operation.
 */
void rffc5071_rxtx(rffc5071_driver_t* const drv) {
	set_RFFC5071_ENBL(drv, 0);
	set_RFFC5071_FULLD(drv, 1); /* mixer 1 and mixer 2 (RXTX) */
	rffc5071_regs_commit(drv);

	rffc5071_enable(drv);
}

void rffc5071_disable(rffc5071_driver_t* const drv)  {
	set_RFFC5071_ENBL(drv, 0);
	rffc5071_regs_commit(drv);
}

void rffc5071_enable(rffc5071_driver_t* const drv)  {
	set_RFFC5071_ENBL(drv, 1);
	rffc5071_regs_commit(drv);
}

#define LO_MAX 5400
#define REF_FREQ 50
#define FREQ_ONE_MHZ (1000*1000)

/* configure frequency synthesizer in integer mode (lo in MHz) */
uint64_t rffc5071_config_synth_int(rffc5071_driver_t* const drv, uint16_t lo) {
	uint8_t lodiv;
	uint16_t fvco;
	uint8_t fbkdiv;
	uint16_t n;
	uint64_t tune_freq_hz;
	uint16_t p1nmsb;
	uint8_t p1nlsb;
	
	/* Calculate n_lo */
	uint8_t n_lo = 0;
	uint16_t x = LO_MAX / lo;
	while ((x > 1) && (n_lo < 5)) {
		n_lo++;
		x >>= 1;
	}

	lodiv = 1 << n_lo;
	fvco = lodiv * lo;

	/* higher divider and charge pump current required above
	 * 3.2GHz. Programming guide says these values (fbkdiv, n,
	 * maybe pump?) can be changed back after enable in order to
	 * improve phase noise, since the VCO will already be stable
	 * and will be unaffected. */
	if (fvco > 3200) {
		fbkdiv = 4;
		set_RFFC5071_PLLCPL(drv, 3);
	} else {
		fbkdiv = 2;
		set_RFFC5071_PLLCPL(drv, 2);
	}

	uint64_t tmp_n = ((uint64_t)fvco << 29ULL) / (fbkdiv*REF_FREQ) ;
	n = tmp_n >> 29ULL;

	p1nmsb = (tmp_n >> 13ULL) & 0xffff;
	p1nlsb = (tmp_n >> 5ULL) & 0xff;
	
	tune_freq_hz = (REF_FREQ * (tmp_n >> 5ULL) * fbkdiv * FREQ_ONE_MHZ)
			/ (lodiv * (1 << 24ULL));

	/* Path 1 */
	set_RFFC5071_P1LODIV(drv, n_lo);
	set_RFFC5071_P1N(drv, n);
	set_RFFC5071_P1PRESC(drv, fbkdiv >> 1);
	set_RFFC5071_P1NMSB(drv, p1nmsb);
	set_RFFC5071_P1NLSB(drv, p1nlsb);

	/* Path 2 */
	set_RFFC5071_P2LODIV(drv, n_lo);
	set_RFFC5071_P2N(drv, n);
	set_RFFC5071_P2PRESC(drv, fbkdiv >> 1);
	set_RFFC5071_P2NMSB(drv, p1nmsb);
	set_RFFC5071_P2NLSB(drv, p1nlsb);

	rffc5071_regs_commit(drv);

	return tune_freq_hz;
}

/* !!!!!!!!!!! hz is currently ignored !!!!!!!!!!! */
uint64_t rffc5071_set_frequency(rffc5071_driver_t* const drv, uint16_t mhz) {
	uint32_t tune_freq;

	rffc5071_disable(drv);
	tune_freq = rffc5071_config_synth_int(drv, mhz);
	rffc5071_enable(drv);

	return tune_freq;
}

void rffc5071_set_gpo(rffc5071_driver_t* const drv, uint8_t gpo)
{
	/* We set GPO for both paths just in case. */
	set_RFFC5071_P1GPO(drv, gpo);
	set_RFFC5071_P2GPO(drv, gpo);

	rffc5071_regs_commit(drv);
}
