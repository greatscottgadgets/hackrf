/*
 * Copyright 2017-2022 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include "platform_detect.h"
#include "platform_gpio.h"
#include "mixer.h"
#include "rffc5071.h"
#include "rffc5071_spi.h"
#include "max2871.h"

static rffc5071_spi_config_t rffc5071_spi_config;

static spi_bus_t spi_bus_rffc5071 = {
	.config = &rffc5071_spi_config,
	.start = rffc5071_spi_start,
	.stop = rffc5071_spi_stop,
	.transfer = rffc5071_spi_transfer,
	.transfer_gather = rffc5071_spi_transfer_gather,
};

mixer_driver_t mixer = {
	.rffc5071.bus = &spi_bus_rffc5071,
};

void mixer_bus_setup(mixer_driver_t* const mixer)
{
	(void) mixer;

	const platform_gpio_t* gpio = platform_gpio();

	switch (mixer->type) {
	case RFFC5071_VARIANT:
		rffc5071_spi_config = (rffc5071_spi_config_t){
			.gpio_select = gpio->rffc5072_select,
			.gpio_clock = gpio->rffc5072_clock,
			.gpio_data = gpio->rffc5072_data,
		};
		spi_bus_start(&spi_bus_rffc5071, &rffc5071_spi_config);
		break;
	case MAX2871_VARIANT:
		break;
	}
}

void mixer_setup(mixer_driver_t* const mixer, mixer_variant_t type)
{
	mixer->type = type;

	const platform_gpio_t* gpio = platform_gpio();

	/* Mixer GPIO serial interface PinMux */
	switch (mixer->type) {
	case RFFC5071_VARIANT:
		mixer->rffc5071.gpio_reset = gpio->rffc5072_reset;
		if (detected_platform() == BOARD_ID_PRALINE) {
			mixer->rffc5071.gpio_ld = gpio->rffc5072_ld;
		}
		break;
	case MAX2871_VARIANT:
		mixer->max2871.gpio_vco_ce = gpio->vco_ce;
		mixer->max2871.gpio_vco_sclk = gpio->vco_sclk;
		mixer->max2871.gpio_vco_sdata = gpio->vco_sdata;
		mixer->max2871.gpio_vco_le = gpio->vco_le;
		mixer->max2871.gpio_synt_rfout_en = gpio->synt_rfout_en;
		mixer->max2871.gpio_vco_mux = gpio->vco_mux;
		break;
	}

	switch (mixer->type) {
	case RFFC5071_VARIANT:
		rffc5071_setup(&mixer->rffc5071);
		break;
	case MAX2871_VARIANT:
		max2871_setup(&mixer->max2871);
		break;
	}
}

uint64_t mixer_set_frequency(mixer_driver_t* const mixer, uint64_t hz)
{
	switch (mixer->type) {
	case RFFC5071_VARIANT:
		return rffc5071_set_frequency(&mixer->rffc5071, hz);
		break;
	case MAX2871_VARIANT:
		return max2871_set_frequency(&mixer->max2871, hz / 1000000);
		break;
	}

	return 0;
}

void mixer_enable(mixer_driver_t* const mixer)
{
	switch (mixer->type) {
	case RFFC5071_VARIANT:
		rffc5071_enable(&mixer->rffc5071);
		break;
	case MAX2871_VARIANT:
		max2871_enable(&mixer->max2871);
		break;
	}
}

void mixer_disable(mixer_driver_t* const mixer)
{
	switch (mixer->type) {
	case RFFC5071_VARIANT:
		rffc5071_disable(&mixer->rffc5071);
		break;
	case MAX2871_VARIANT:
		max2871_disable(&mixer->max2871);
		break;
	}
}

void mixer_set_gpo(mixer_driver_t* const mixer, uint8_t gpo)
{
	switch (mixer->type) {
	case RFFC5071_VARIANT:
		rffc5071_set_gpo(&mixer->rffc5071, gpo);
		break;
	case MAX2871_VARIANT:
		break;
	}
}
