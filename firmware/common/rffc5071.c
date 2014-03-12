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

#if (defined DEBUG)
#include <stdio.h>
#define LOG printf
#else
#define LOG(x,...)
#include <libopencm3/lpc43xx/ssp.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/gpio.h>
#include "hackrf_core.h"
#endif

/* Default register values. */
static uint16_t rffc5071_regs_default[RFFC5071_NUM_REGS] = { 
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

uint16_t rffc5071_regs[RFFC5071_NUM_REGS];

/* Mark all regsisters dirty so all will be written at init. */
uint32_t rffc5071_regs_dirty = 0x7fffffff;

/* Set up all registers according to defaults specified in docs. */
void rffc5071_init(void)
{
	LOG("# rffc5071_init\n");
	memcpy(rffc5071_regs, rffc5071_regs_default, sizeof(rffc5071_regs));
	rffc5071_regs_dirty = 0x7fffffff;

	/* Write default register values to chip. */
	rffc5071_regs_commit();
}

/*
 * Set up pins for GPIO and SPI control, configure SSP peripheral for SPI, and
 * set our own default register configuration.
 */
void rffc5071_setup(void)
{
	rffc5071_init();
	LOG("# rffc5071_setup\n");
#if !defined TEST
	/* Configure GPIO pins. */
	scu_pinmux(SCU_MIXER_ENX, SCU_GPIO_FAST);
	scu_pinmux(SCU_MIXER_SCLK, SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
	scu_pinmux(SCU_MIXER_SDATA, SCU_GPIO_FAST);
	scu_pinmux(SCU_MIXER_RESETX, SCU_GPIO_FAST);

	/* Set GPIO pins as outputs. */
	GPIO_DIR(PORT_MIXER_ENX) |= PIN_MIXER_ENX;
	GPIO_DIR(PORT_MIXER_SCLK) |= PIN_MIXER_SCLK;
	GPIO_DIR(PORT_MIXER_SDATA) |= PIN_MIXER_SDATA;
	GPIO_DIR(PORT_MIXER_RESETX) |= PIN_MIXER_RESETX;

	/* set to known state */
	gpio_set(PORT_MIXER_ENX, PIN_MIXER_ENX); /* active low */
	gpio_clear(PORT_MIXER_SCLK, PIN_MIXER_SCLK);
	gpio_clear(PORT_MIXER_SDATA, PIN_MIXER_SDATA);
	gpio_set(PORT_MIXER_RESETX, PIN_MIXER_RESETX); /* active low */
#endif

	/* initial setup */
	/* put zeros in freq contol registers */
	set_RFFC5071_P2N(0);
	set_RFFC5071_P2LODIV(0);
	set_RFFC5071_P2PRESC(0);
	set_RFFC5071_P2VCOSEL(0);

	set_RFFC5071_P2N(0);
	set_RFFC5071_P2LODIV(0);
	set_RFFC5071_P2PRESC(0);
	set_RFFC5071_P2VCOSEL(0);

	set_RFFC5071_P2N(0);
	set_RFFC5071_P2LODIV(0);
	set_RFFC5071_P2PRESC(0);
	set_RFFC5071_P2VCOSEL(0);

	/* set ENBL and MODE to be configured via 3-wire interface,
	 * not control pins. */
	set_RFFC5071_SIPIN(1);

	/* GPOs are active at all times */
	set_RFFC5071_GATE(1);

	rffc5071_regs_commit();
}

void serial_delay(void)
{
	uint32_t i;

	for (i = 0; i < 2; i++)
		__asm__("nop");
}

/* SPI register read.
 *
 * Send 9 bits:
 *   first bit is ignored,
 *   second bit is one for read operation,
 *   next 7 bits are register address.
 * Then receive 16 bits (register value).
 */
uint16_t rffc5071_spi_read(uint8_t r) {

	int bits = 9;
	int msb = 1 << (bits -1);
	uint32_t data = 0x80 | (r & 0x7f);

#if DEBUG
	LOG("reg%d = 0\n", r);
	return 0;
#else
	/* make sure everything is starting in the correct state */
	gpio_set(PORT_MIXER_ENX, PIN_MIXER_ENX);
	gpio_clear(PORT_MIXER_SCLK, PIN_MIXER_SCLK);
	gpio_clear(PORT_MIXER_SDATA, PIN_MIXER_SDATA);

	/*
	 * The device requires two clocks while ENX is high before a serial
	 * transaction.  This is not clearly documented.
	 */
	serial_delay();
	gpio_set(PORT_MIXER_SCLK, PIN_MIXER_SCLK);

	serial_delay();
	gpio_clear(PORT_MIXER_SCLK, PIN_MIXER_SCLK);

	serial_delay();
	gpio_set(PORT_MIXER_SCLK, PIN_MIXER_SCLK);

	serial_delay();
	gpio_clear(PORT_MIXER_SCLK, PIN_MIXER_SCLK);

	/* start transaction by bringing ENX low */
	gpio_clear(PORT_MIXER_ENX, PIN_MIXER_ENX);

	while (bits--) {
		if (data & msb)
			gpio_set(PORT_MIXER_SDATA, PIN_MIXER_SDATA);
		else
			gpio_clear(PORT_MIXER_SDATA, PIN_MIXER_SDATA);
		data <<= 1;

		serial_delay();
		gpio_set(PORT_MIXER_SCLK, PIN_MIXER_SCLK);

		serial_delay();
		gpio_clear(PORT_MIXER_SCLK, PIN_MIXER_SCLK);
	}

	serial_delay();
	gpio_set(PORT_MIXER_SCLK, PIN_MIXER_SCLK);

	serial_delay();
	gpio_clear(PORT_MIXER_SCLK, PIN_MIXER_SCLK);

	bits = 16;
	data = 0;
	/* set SDATA line as input */
	GPIO_DIR(PORT_MIXER_SDATA) &= ~PIN_MIXER_SDATA;

	while (bits--) {
		data <<= 1;

		serial_delay();
		gpio_set(PORT_MIXER_SCLK, PIN_MIXER_SCLK);

		serial_delay();
		gpio_clear(PORT_MIXER_SCLK, PIN_MIXER_SCLK);
		if (MIXER_SDATA_STATE)
			data |= 1;
	}
	/* set SDATA line as output */
	GPIO_DIR(PORT_MIXER_SDATA) |= PIN_MIXER_SDATA;

	serial_delay();
	gpio_set(PORT_MIXER_ENX, PIN_MIXER_ENX);

	/*
	 * The device requires a clock while ENX is high after a serial
	 * transaction.  This is not clearly documented.
	 */
	gpio_set(PORT_MIXER_SCLK, PIN_MIXER_SCLK);

	serial_delay();
	gpio_clear(PORT_MIXER_SCLK, PIN_MIXER_SCLK);

	return data;
#endif /* DEBUG */
}

/* SPI register write
 *
 * Send 25 bits:
 *   first bit is ignored,
 *   second bit is zero for write operation,
 *   next 7 bits are register address,
 *   next 16 bits are register value.
 */
void rffc5071_spi_write(uint8_t r, uint16_t v) {

#if DEBUG
	LOG("0x%04x -> reg%d\n", v, r);
#else

	int bits = 25;
	int msb = 1 << (bits -1);
	uint32_t data = ((r & 0x7f) << 16) | v;

	/* make sure everything is starting in the correct state */
	gpio_set(PORT_MIXER_ENX, PIN_MIXER_ENX);
	gpio_clear(PORT_MIXER_SCLK, PIN_MIXER_SCLK);
	gpio_clear(PORT_MIXER_SDATA, PIN_MIXER_SDATA);

	/*
	 * The device requires two clocks while ENX is high before a serial
	 * transaction.  This is not clearly documented.
	 */
	serial_delay();
	gpio_set(PORT_MIXER_SCLK, PIN_MIXER_SCLK);

	serial_delay();
	gpio_clear(PORT_MIXER_SCLK, PIN_MIXER_SCLK);

	serial_delay();
	gpio_set(PORT_MIXER_SCLK, PIN_MIXER_SCLK);

	serial_delay();
	gpio_clear(PORT_MIXER_SCLK, PIN_MIXER_SCLK);

	/* start transaction by bringing ENX low */
	gpio_clear(PORT_MIXER_ENX, PIN_MIXER_ENX);

	while (bits--) {
		if (data & msb)
			gpio_set(PORT_MIXER_SDATA, PIN_MIXER_SDATA);
		else
			gpio_clear(PORT_MIXER_SDATA, PIN_MIXER_SDATA);
		data <<= 1;

		serial_delay();
		gpio_set(PORT_MIXER_SCLK, PIN_MIXER_SCLK);

		serial_delay();
		gpio_clear(PORT_MIXER_SCLK, PIN_MIXER_SCLK);
	}

	gpio_set(PORT_MIXER_ENX, PIN_MIXER_ENX);

	/*
	 * The device requires a clock while ENX is high after a serial
	 * transaction.  This is not clearly documented.
	 */
	serial_delay();
	gpio_set(PORT_MIXER_SCLK, PIN_MIXER_SCLK);

	serial_delay();
	gpio_clear(PORT_MIXER_SCLK, PIN_MIXER_SCLK);
#endif
}

uint16_t rffc5071_reg_read(uint8_t r)
{
	/* Readback register is not cached. */
	if (r == RFFC5071_READBACK_REG)
		return rffc5071_spi_read(r);

	/* Discard uncommited write when reading. This shouldn't
	 * happen, and probably has not been tested. */
	if ((rffc5071_regs_dirty >> r) & 0x1) {
		rffc5071_spi_read(r);
	};
	return rffc5071_regs[r];
}

void rffc5071_reg_write(uint8_t r, uint16_t v)
{
	rffc5071_regs[r] = v;
	rffc5071_spi_write(r, v);
	RFFC5071_REG_SET_CLEAN(r);
}

static inline void rffc5071_reg_commit(uint8_t r)
{
	rffc5071_reg_write(r,rffc5071_regs[r]);
}

void rffc5071_regs_commit(void)
{
	int r;
	for(r = 0; r < RFFC5071_NUM_REGS; r++) {
		if ((rffc5071_regs_dirty >> r) & 0x1) {
			rffc5071_reg_commit(r);
		}
	}
}

void rffc5071_tx(void) {
	LOG("# rffc5071_tx\n");
	set_RFFC5071_ENBL(0);
	set_RFFC5071_FULLD(0);
	set_RFFC5071_MODE(1); /* mixer 2 used for both RX and TX */
	rffc5071_regs_commit();
}

void rffc5071_rx(void) {
	LOG("# rfc5071_rx\n");
	set_RFFC5071_ENBL(0);
	set_RFFC5071_FULLD(0);
	set_RFFC5071_MODE(1); /* mixer 2 used for both RX and TX */
	rffc5071_regs_commit();
}

/*
 * This function turns on both mixer (full-duplex) on the RFFC5071, but our
 * current hardware designs do not support full-duplex operation.
 */
void rffc5071_rxtx(void) {
	LOG("# rfc5071_rxtx\n");
	set_RFFC5071_ENBL(0);
	set_RFFC5071_FULLD(1); /* mixer 1 and mixer 2 (RXTX) */
	rffc5071_regs_commit();

	rffc5071_enable();
}

void rffc5071_disable(void)  {
	LOG("# rfc5071_disable\n");
	set_RFFC5071_ENBL(0);
	rffc5071_regs_commit();
}

void rffc5071_enable(void)  {
	LOG("# rfc5071_enable\n");
	set_RFFC5071_ENBL(1);
	rffc5071_regs_commit();
}

#define LO_MAX 5400
#define REF_FREQ 50
#define FREQ_ONE_MHZ (1000*1000)

/* configure frequency synthesizer in integer mode (lo in MHz) */
uint64_t rffc5071_config_synth_int(uint16_t lo) {
	uint8_t lodiv;
	uint16_t fvco;
	uint8_t fbkdiv;
	uint16_t n;
	uint64_t tune_freq_hz;
	uint16_t p1nmsb;
	uint8_t p1nlsb;
	
	LOG("# config_synth_int\n");

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
		set_RFFC5071_PLLCPL(3);
	} else {
		fbkdiv = 2;
		set_RFFC5071_PLLCPL(2);
	}

	uint64_t tmp_n = ((uint64_t)fvco << 29ULL) / (fbkdiv*REF_FREQ) ;
	n = tmp_n >> 29ULL;

	p1nmsb = (tmp_n >> 13ULL) & 0xffff;
	p1nlsb = (tmp_n >> 5ULL) & 0xff;
	
	tune_freq_hz = (REF_FREQ * (tmp_n >> 5ULL) * fbkdiv * FREQ_ONE_MHZ)
			/ (lodiv * (1 << 24ULL));
	LOG("# lo=%d n_lo=%d lodiv=%d fvco=%d fbkdiv=%d n=%d tune_freq_hz=%d\n",
			lo, n_lo, lodiv, fvco, fbkdiv, n, tune_freq);

	/* Path 1 */
	set_RFFC5071_P1LODIV(n_lo);
	set_RFFC5071_P1N(n);
	set_RFFC5071_P1PRESC(fbkdiv >> 1);
	set_RFFC5071_P1NMSB(p1nmsb);
	set_RFFC5071_P1NLSB(p1nlsb);

	/* Path 2 */
	set_RFFC5071_P2LODIV(n_lo);
	set_RFFC5071_P2N(n);
	set_RFFC5071_P2PRESC(fbkdiv >> 1);
	set_RFFC5071_P2NMSB(p1nmsb);
	set_RFFC5071_P2NLSB(p1nlsb);

	rffc5071_regs_commit();

	return tune_freq_hz;
}

/* !!!!!!!!!!! hz is currently ignored !!!!!!!!!!! */
uint64_t rffc5071_set_frequency(uint16_t mhz) {
	uint32_t tune_freq;

	rffc5071_disable();
	tune_freq = rffc5071_config_synth_int(mhz);
	rffc5071_enable();

	return tune_freq;
}

void rffc5071_set_gpo(uint8_t gpo)
{
	/* We set GPO for both paths just in case. */
	set_RFFC5071_P1GPO(gpo);
	set_RFFC5071_P2GPO(gpo);

	rffc5071_regs_commit();
}

#ifdef TEST
int main(int ac, char **av)
{
	rffc5071_setup();
	rffc5071_tx(0);
	rffc5071_set_frequency(500, 0);
	rffc5071_set_frequency(525, 0);
	rffc5071_set_frequency(550, 0);
	rffc5071_set_frequency(1500, 0);
	rffc5071_set_frequency(1525, 0);
	rffc5071_set_frequency(1550, 0);
	rffc5071_disable();
	rffc5071_rx(0);
	rffc5071_disable();
	rffc5071_rxtx();
	rffc5071_disable();
}
#endif //TEST
