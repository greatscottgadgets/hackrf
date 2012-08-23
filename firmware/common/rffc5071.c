/*
 * Copyright 2012 Michael Ossmann
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

#include <stdint.h>
#include <libopencm3/lpc43xx/ssp.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/gpio.h>
#include "hackrf_core.h"
#include "rffc5071.h"

/* Set up pins for bit-banged serial interface. */
void rffc5071_init(void)
{
	/* Configure GPIO pins. */
	scu_pinmux(SCU_MIXER_ENX, SCU_GPIO_FAST);
	scu_pinmux(SCU_MIXER_SCLK, SCU_GPIO_FAST);
	scu_pinmux(SCU_MIXER_SDATA, SCU_GPIO_FAST);

	/* Set GPIO pins as outputs. */
	GPIO3_DIR |= (PIN_MIXER_ENX | PIN_MIXER_SCLK | PIN_MIXER_SDATA);

	/* set to known state */
	gpio_set(PORT_MIXER, PIN_MIXER_ENX); /* active low */
	gpio_clear(PORT_MIXER, (PIN_MIXER_SCLK | PIN_MIXER_SDATA));

	//FIXME hard coded setup, fields not broken out
	/* initial setup */
	rffc5071_reg_write(RFFC5071_P2_FREQ1, 0x0000);
	rffc5071_reg_write(RFFC5071_VCO_AUTO, 0xff00);
	rffc5071_reg_write(RFFC5071_CT_CAL1, 0xacbf);
	rffc5071_reg_write(RFFC5071_CT_CAL2, 0xacbf);
	rffc5071_reg_write(RFFC5071_TEST, 0x0005);

	/* set to be configured via 3-wire interface, not control pins */
	rffc5071_reg_write(RFFC5071_SDI_CTRL, 0x8000);

	//rffc5071_reg_write(MIX_CONT, 0xc800); /* full duplex */
	rffc5071_reg_write(RFFC5071_MIX_CONT, 0x4800); /* half duplex */
}

#define LO_MAX   5400
#define REF_FREQ 50

/* configure frequency synthesizer in integer mode (lo in MHz) */
void rffc5071_config_synth_int(uint16_t lo) {
	uint8_t n_lo;
	uint8_t lodiv;
	uint16_t fvco;
	uint8_t fbkdiv;
	uint16_t n;

	/* n_lo = int(log2(LO_MAX/lo)) */
	for (n_lo = 0; n_lo < 5; n_lo++)
		if ((2 << n_lo) > (LO_MAX / lo))
			break;

	lodiv = 1 << n_lo;
	fvco = lodiv * lo;

	if (fvco > 3200) {
		fbkdiv = 4;
		/* set charge pump for VCO > 3.2 GHz */
		rffc5071_reg_write(RFFC5071_LF, 0xbefb);
	} else {
		fbkdiv = 2;
	}

	n = (fvco / fbkdiv) / REF_FREQ;

	rffc5071_reg_write(RFFC5071_P1_FREQ1,
			(n << 7) | (n_lo << 4) | (fbkdiv << 1));
	rffc5071_reg_write(RFFC5071_P1_FREQ2, 0x0000);
	rffc5071_reg_write(RFFC5071_P1_FREQ3, 0x0000);
}

void rffc5071_enable_tx(void) {
	rffc5071_reg_write(RFFC5071_SDI_CTRL, 0xc000); /* mixer 1 (TX) */
}

void rffc5071_enable_rx(void) {
	rffc5071_reg_write(RFFC5071_SDI_CTRL, 0xe000); /* mixer 2 (RX) */
}

void serial_delay(void)
{
	uint32_t i;

	for (i = 0; i < 1000; i++)
		__asm__("nop");
}

/*
 * Send 25 bits:
 *   first bit is ignored,
 *   second bit is zero for write operation,
 *   next 7 bits are register address,
 *   next 16 bits are register value.
 */
void rffc5071_reg_write(uint8_t reg, uint16_t val)
{
	int bits = 25;
	int msb = 1 << (bits -1);
	uint32_t data = ((reg & 0x7f) << 16) | val;

	/* make sure everything is starting in the correct state */
	gpio_set(PORT_MIXER, PIN_MIXER_ENX);
	gpio_clear(PORT_MIXER, (PIN_MIXER_SCLK | PIN_MIXER_SDATA));

	/*
	 * The device requires two clocks while ENX is high before a serial
	 * transaction.  This is not clearly documented.
	 */
	serial_delay();
	gpio_set(PORT_MIXER, PIN_MIXER_SCLK);

	serial_delay();
	gpio_clear(PORT_MIXER, PIN_MIXER_SCLK);

	serial_delay();
	gpio_set(PORT_MIXER, PIN_MIXER_SCLK);

	serial_delay();
	gpio_clear(PORT_MIXER, PIN_MIXER_SCLK);

	/* start transaction by bringing ENX low */
	gpio_clear(PORT_MIXER, PIN_MIXER_ENX);

	while (bits--) {
		if (data & msb)
			gpio_set(PORT_MIXER, PIN_MIXER_SDATA);
		else
			gpio_clear(PORT_MIXER, PIN_MIXER_SDATA);
		data <<= 1;

		serial_delay();
		gpio_set(PORT_MIXER, PIN_MIXER_SCLK);

		serial_delay();
		gpio_clear(PORT_MIXER, PIN_MIXER_SCLK);
	}

	serial_delay();
	gpio_set(PORT_MIXER, PIN_MIXER_ENX);
}

/*
 * Send 9 bits:
 *   first bit is ignored,
 *   second bit is one for read operation,
 *   next 7 bits are register address.
 * Then receive 16 bits (register value).
 */
uint16_t rffc5071_reg_read(uint8_t reg)
{
	int bits = 9;
	int msb = 1 << (bits -1);
	uint32_t data = 0x80 | (reg & 0x7f);

	/* make sure everything is starting in the correct state */
	gpio_set(PORT_MIXER, PIN_MIXER_ENX);
	gpio_clear(PORT_MIXER, (PIN_MIXER_SCLK | PIN_MIXER_SDATA));

	/*
	 * The device requires two clocks while ENX is high before a serial
	 * transaction.  This is not clearly documented.
	 */
	serial_delay();
	gpio_set(PORT_MIXER, PIN_MIXER_SCLK);

	serial_delay();
	gpio_clear(PORT_MIXER, PIN_MIXER_SCLK);

	serial_delay();
	gpio_set(PORT_MIXER, PIN_MIXER_SCLK);

	serial_delay();
	gpio_clear(PORT_MIXER, PIN_MIXER_SCLK);

	/* start transaction by bringing ENX low */
	gpio_clear(PORT_MIXER, PIN_MIXER_ENX);

	while (bits--) {
		if (data & msb)
			gpio_set(PORT_MIXER, PIN_MIXER_SDATA);
		else
			gpio_clear(PORT_MIXER, PIN_MIXER_SDATA);
		data <<= 1;

		serial_delay();
		gpio_set(PORT_MIXER, PIN_MIXER_SCLK);

		serial_delay();
		gpio_clear(PORT_MIXER, PIN_MIXER_SCLK);
	}

	serial_delay();
	gpio_set(PORT_MIXER, PIN_MIXER_SCLK);

	serial_delay();
	gpio_clear(PORT_MIXER, PIN_MIXER_SCLK);

	bits = 16;
	data = 0;
	/* set SDATA line as input */
	GPIO3_DIR &= ~PIN_MIXER_SDATA;

	while (bits--) {
		data <<= 1;

		serial_delay();
		gpio_set(PORT_MIXER, PIN_MIXER_SCLK);

		serial_delay();
		gpio_clear(PORT_MIXER, PIN_MIXER_SCLK);
		if (MIXER_SDATA_STATE)
			data |= 1;
	}
	/* set SDATA line as output */
	GPIO3_DIR |= PIN_MIXER_SDATA;

	serial_delay();
	gpio_set(PORT_MIXER, PIN_MIXER_ENX);

	return data;
}
