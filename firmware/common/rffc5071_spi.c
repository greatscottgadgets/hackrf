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

#include <libopencm3/lpc43xx/scu.h>
#include "hackrf_core.h"

#include "rffc5071_spi.h"

static void rffc5071_spi_target_select(spi_bus_t* const bus) {
	const rffc5071_spi_config_t* const config = bus->config;
	gpio_clear(config->gpio_select);
}

static void rffc5071_spi_target_unselect(spi_bus_t* const bus) {
	const rffc5071_spi_config_t* const config = bus->config;
	gpio_set(config->gpio_select);
}

static void rffc5071_spi_direction_out(spi_bus_t* const bus) {
	const rffc5071_spi_config_t* const config = bus->config;
	gpio_output(config->gpio_data);
}

static void rffc5071_spi_direction_in(spi_bus_t* const bus) {
	const rffc5071_spi_config_t* const config = bus->config;
	gpio_input(config->gpio_data);
}

static void rffc5071_spi_data_out(spi_bus_t* const bus, const bool bit) {
	const rffc5071_spi_config_t* const config = bus->config;
	gpio_write(config->gpio_data, bit);
}

static bool rffc5071_spi_data_in(spi_bus_t* const bus) {
	const rffc5071_spi_config_t* const config = bus->config;
	return gpio_read(config->gpio_data);
}

static void rffc5071_spi_bus_init(spi_bus_t* const bus) {
	const rffc5071_spi_config_t* const config = bus->config;

	scu_pinmux(SCU_MIXER_SCLK, SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
	scu_pinmux(SCU_MIXER_SDATA, SCU_GPIO_FAST);

	gpio_output(config->gpio_clock);
	rffc5071_spi_direction_out(bus);

	gpio_clear(config->gpio_clock);
	gpio_clear(config->gpio_data);
}

static void rffc5071_spi_target_init(spi_bus_t* const bus) {
	const rffc5071_spi_config_t* const config = bus->config;

	/* Configure GPIO pins. */
	scu_pinmux(SCU_MIXER_ENX, SCU_GPIO_FAST);
	scu_pinmux(SCU_MIXER_RESETX, SCU_GPIO_FAST);

	/* Set GPIO pins as outputs. */
	gpio_output(config->gpio_select);

	/* set to known state */
	rffc5071_spi_target_unselect(bus);
}

void rffc5071_spi_start(spi_bus_t* const bus, const void* const config) {
	bus->config = config;
	rffc5071_spi_bus_init(bus);
	rffc5071_spi_target_init(bus);
}

void rffc5071_spi_stop(spi_bus_t* const bus) {
	(void)bus;
}

static void rffc5071_spi_serial_delay(spi_bus_t* const bus) {
	(void)bus;
	volatile uint32_t i;

	for (i = 0; i < 2; i++)
		__asm__("nop");
}

static void rffc5071_spi_sck(spi_bus_t* const bus) {
	const rffc5071_spi_config_t* const config = bus->config;

	rffc5071_spi_serial_delay(bus);
	gpio_set(config->gpio_clock);

	rffc5071_spi_serial_delay(bus);
	gpio_clear(config->gpio_clock);
}

static uint32_t rffc5071_spi_exchange_bit(spi_bus_t* const bus, const uint32_t bit) {
	rffc5071_spi_data_out(bus, bit);
	rffc5071_spi_sck(bus);
	return rffc5071_spi_data_in(bus) ? 1 : 0;
}

static uint32_t rffc5071_spi_exchange_word(spi_bus_t* const bus, const uint32_t data, const size_t count) {
	size_t bits = count;
	const uint32_t msb = 1UL << (count - 1);
	uint32_t t = data;

	while (bits--) {
		t = (t << 1) | rffc5071_spi_exchange_bit(bus, t & msb);
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
void rffc5071_spi_transfer(spi_bus_t* const bus, void* const _data, const size_t count) {
	if( count != 2 ) {
		return;
	}

	uint16_t* const data = _data;
	
	const bool direction_read = (data[0] >> 7) & 1;

	/*
	 * The device requires two clocks while ENX is high before a serial
	 * transaction.  This is not clearly documented.
	 */
	rffc5071_spi_sck(bus);
	rffc5071_spi_sck(bus);

	rffc5071_spi_target_select(bus);
	data[0] = rffc5071_spi_exchange_word(bus, data[0], 9);

	if( direction_read ) {
		rffc5071_spi_direction_in(bus);
		rffc5071_spi_sck(bus);
	}
	data[1] = rffc5071_spi_exchange_word(bus, data[1], 16);

	rffc5071_spi_serial_delay(bus);
	rffc5071_spi_target_unselect(bus);
	rffc5071_spi_direction_out(bus);

	/*
	 * The device requires a clock while ENX is high after a serial
	 * transaction.  This is not clearly documented.
	 */
	rffc5071_spi_sck(bus);
}

void rffc5071_spi_transfer_gather(spi_bus_t* const bus, const spi_transfer_t* const transfer, const size_t count) {
	if( count == 1 ) {
		rffc5071_spi_transfer(bus, transfer[0].data, transfer[0].count);
	}
}
