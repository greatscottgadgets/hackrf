/*
 * Copyright 2017-2026 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include "mixer.h"

#include <stdbool.h>

#include "fixed_point.h"
#include "platform_detect.h" // IWYU pragma: keep (universal)
#include "platform_gpio.h"
#ifdef IS_RAD1O
	#include "max2871.h"
#endif
#ifdef IS_NOT_RAD1O
	#include "rffc5071.h"
	#include "rffc5071_spi.h"
	#include "spi_bus.h"
#endif

#ifdef IS_NOT_RAD1O
static rffc5071_spi_config_t rffc5071_spi_config;

static spi_bus_t spi_bus_rffc5071 = {
	.config = &rffc5071_spi_config,
	.start = rffc5071_spi_start,
	.stop = rffc5071_spi_stop,
	.transfer = rffc5071_spi_transfer,
	.transfer_gather = rffc5071_spi_transfer_gather,
};
#endif

mixer_driver_t mixer = {
#ifdef IS_NOT_RAD1O
	.rffc5071.bus = &spi_bus_rffc5071,
#endif
};

void mixer_bus_setup(mixer_driver_t* const mixer)
{
	(void) mixer;

#ifdef IS_NOT_RAD1O
	if (IS_NOT_RAD1O) {
		const platform_gpio_t* gpio = platform_gpio();

		rffc5071_spi_config = (rffc5071_spi_config_t){
			.gpio_select = gpio->rffc5072_select,
			.gpio_clock = gpio->rffc5072_clock,
			.gpio_data = gpio->rffc5072_data,
		};
		spi_bus_start(&spi_bus_rffc5071, &rffc5071_spi_config);
	}
#endif
}

void mixer_setup(mixer_driver_t* const mixer, mixer_variant_t type)
{
	mixer->type = type;

	const platform_gpio_t* gpio = platform_gpio();

	/* Mixer GPIO serial interface PinMux */
#ifdef IS_PRALINE
	if (IS_PRALINE) {
		mixer->rffc5071.gpio_ld = gpio->rffc5072_ld;
	}
#endif

#ifdef IS_NOT_RAD1O
	if (IS_NOT_RAD1O) {
		mixer->rffc5071.gpio_reset = gpio->rffc5072_reset;
		rffc5071_setup(&mixer->rffc5071);
	}
#endif

#ifdef IS_RAD1O
	if (IS_RAD1O) {
		mixer->max2871.gpio_vco_ce = gpio->vco_ce;
		mixer->max2871.gpio_vco_sclk = gpio->vco_sclk;
		mixer->max2871.gpio_vco_sdata = gpio->vco_sdata;
		mixer->max2871.gpio_vco_le = gpio->vco_le;
		mixer->max2871.gpio_synt_rfout_en = gpio->synt_rfout_en;
		mixer->max2871.gpio_vco_mux = gpio->vco_mux;
		max2871_setup(&mixer->max2871);
	}
#endif
}

fp_40_24_t mixer_set_frequency(mixer_driver_t* const mixer, fp_40_24_t lo, bool program)
{
#ifdef IS_NOT_RAD1O
	if (IS_NOT_RAD1O) {
		return rffc5071_set_frequency(&mixer->rffc5071, lo, program);
	}
#endif

#ifdef IS_RAD1O
	if (IS_RAD1O) {
		return max2871_set_frequency(&mixer->max2871, lo, program);
	}
#endif
}

void mixer_enable(mixer_driver_t* const mixer)
{
#ifdef IS_NOT_RAD1O
	if (IS_NOT_RAD1O) {
		rffc5071_enable(&mixer->rffc5071);
	}
#endif

#ifdef IS_RAD1O
	if (IS_RAD1O) {
		max2871_enable(&mixer->max2871);
	}
#endif
}

void mixer_disable(mixer_driver_t* const mixer)
{
#ifdef IS_NOT_RAD1O
	if (IS_NOT_RAD1O) {
		rffc5071_disable(&mixer->rffc5071);
	}
#endif

#ifdef IS_RAD1O
	if (IS_RAD1O) {
		max2871_disable(&mixer->max2871);
	}
#endif
}

void mixer_set_gpo(mixer_driver_t* const mixer, uint8_t gpo)
{
#ifdef IS_NOT_RAD1O
	if (IS_NOT_RAD1O) {
		rffc5071_set_gpo(&mixer->rffc5071, gpo);
	}
#endif

#ifdef IS_RAD1O
	if (IS_RAD1O) {
		(void) mixer;
		(void) gpo;
	}
#endif
}
