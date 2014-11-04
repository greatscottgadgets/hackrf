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

void rffc5071_pin_config() {
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
}

static void serial_delay(void)
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
