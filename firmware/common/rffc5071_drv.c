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

#include <libopencm3/lpc43xx/ssp.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/gpio.h>
#include "hackrf_core.h"

static void rffc5071_target_select() {
	gpio_clear(PORT_MIXER_ENX, PIN_MIXER_ENX);
}

static void rffc5071_target_unselect() {
	gpio_set(PORT_MIXER_ENX, PIN_MIXER_ENX);
}

static void rffc5071_spi_direction_out() {
	GPIO_DIR(PORT_MIXER_SDATA) |= PIN_MIXER_SDATA;
}

static void rffc5071_spi_direction_in() {
	GPIO_DIR(PORT_MIXER_SDATA) &= ~PIN_MIXER_SDATA;
}

static void rffc5071_spi_data_out(const bool bit) {
	if (bit)
		gpio_set(PORT_MIXER_SDATA, PIN_MIXER_SDATA);
	else
		gpio_clear(PORT_MIXER_SDATA, PIN_MIXER_SDATA);
}

static bool rffc5071_spi_data_in() {
	return MIXER_SDATA_STATE;
}

static void rffc5071_spi_init() {
	scu_pinmux(SCU_MIXER_SCLK, SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
	scu_pinmux(SCU_MIXER_SDATA, SCU_GPIO_FAST);

	GPIO_DIR(PORT_MIXER_SCLK) |= PIN_MIXER_SCLK;
	rffc5071_spi_direction_out();

	gpio_clear(PORT_MIXER_SCLK, PIN_MIXER_SCLK);
	gpio_clear(PORT_MIXER_SDATA, PIN_MIXER_SDATA);
}

static void rffc5071_target_init() {
	/* Configure GPIO pins. */
	scu_pinmux(SCU_MIXER_ENX, SCU_GPIO_FAST);
	scu_pinmux(SCU_MIXER_RESETX, SCU_GPIO_FAST);

	/* Set GPIO pins as outputs. */
	GPIO_DIR(PORT_MIXER_ENX) |= PIN_MIXER_ENX;
	GPIO_DIR(PORT_MIXER_RESETX) |= PIN_MIXER_RESETX;

	/* set to known state */
	rffc5071_target_unselect();
	gpio_set(PORT_MIXER_RESETX, PIN_MIXER_RESETX); /* active low */
}

void rffc5071_pin_config(rffc5071_driver_t* const drv) {
	(void)drv;

	rffc5071_spi_init();
	rffc5071_target_init();
}

static void rffc5071_spi_serial_delay(void) {
	volatile uint32_t i;

	for (i = 0; i < 2; i++)
		__asm__("nop");
}

static void rffc5071_spi_sck() {
	rffc5071_spi_serial_delay();
	gpio_set(PORT_MIXER_SCLK, PIN_MIXER_SCLK);

	rffc5071_spi_serial_delay();
	gpio_clear(PORT_MIXER_SCLK, PIN_MIXER_SCLK);
}

static uint32_t rffc5071_spi_exchange_bit(const uint32_t bit) {
	rffc5071_spi_data_out(bit);
	rffc5071_spi_sck();
	return rffc5071_spi_data_in() ? 1 : 0;
}

static uint32_t rffc5071_spi_exchange_word(const uint32_t data, const size_t count) {
	size_t bits = count;
	const uint32_t msb = 1UL << (count - 1);
	uint32_t t = data;

	while (bits--) {
		t = (t << 1) | rffc5071_spi_exchange_bit(t & msb);
	}

	return t & ((1UL << count) - 1);
}

/* SPI register read.
 *
 * Send 9 bits:
 *   first bit is ignored,
 *   second bit is one for read operation,
 *   next 7 bits are register address.
 * Then receive 16 bits (register value).
 */
/* SPI register write
 *
 * Send 25 bits:
 *   first bit is ignored,
 *   second bit is zero for write operation,
 *   next 7 bits are register address,
 *   next 16 bits are register value.
 */
void rffc5071_spi_transfer(rffc5071_driver_t* const drv, uint16_t* const data, const size_t count) {
	(void)drv;

	if( count != 2 ) {
		return;
	}

	const bool direction_read = (data[0] >> 7) & 1;

	/*
	 * The device requires two clocks while ENX is high before a serial
	 * transaction.  This is not clearly documented.
	 */
	rffc5071_spi_sck();
	rffc5071_spi_sck();

	rffc5071_target_select();
	data[0] = rffc5071_spi_exchange_word(data[0], 9);

	if( direction_read ) {
		rffc5071_spi_direction_in();
		rffc5071_spi_sck();
	}
	data[1] = rffc5071_spi_exchange_word(data[1], 16);

	rffc5071_spi_serial_delay();
	rffc5071_target_unselect();
	rffc5071_spi_direction_out();

	/*
	 * The device requires a clock while ENX is high after a serial
	 * transaction.  This is not clearly documented.
	 */
	rffc5071_spi_sck();
}
