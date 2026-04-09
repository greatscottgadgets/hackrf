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

#include "fixed_point.h"
#include "platform_gpio.h"
#if defined(JAWBREAKER) || defined(HACKRF_ONE) || defined(PRALINE)
	#include "rffc5071.h"
	#include "rffc5071_spi.h"
	#include "spi_bus.h"
#elif defined(RAD1O)
	#include "max2871.h"
#endif

#if defined(JAWBREAKER) || defined(HACKRF_ONE) || defined(PRALINE)
static rffc5071_spi_config_t rffc5071_spi_config;

spi_bus_t spi_bus_rffc5071 = {
	.config = &rffc5071_spi_config,
	.start = rffc5071_spi_start,
	.stop = rffc5071_spi_stop,
	.transfer = rffc5071_spi_transfer,
	.transfer_gather = rffc5071_spi_transfer_gather,
};

mixer_driver_t mixer = {
	.bus = &spi_bus_rffc5071,
};
#elif defined(RAD1O)
mixer_driver_t mixer = {};
#endif

void mixer_bus_setup(mixer_driver_t* const mixer)
{
	(void) mixer;

#if defined(JAWBREAKER) || defined(HACKRF_ONE) || defined(PRALINE)
	const platform_gpio_t* gpio = platform_gpio();
	rffc5071_spi_config = (rffc5071_spi_config_t){
		.gpio_select = gpio->rffc5072_select,
		.gpio_clock = gpio->rffc5072_clock,
		.gpio_data = gpio->rffc5072_data,
	};
	spi_bus_start(&spi_bus_rffc5071, &rffc5071_spi_config);
#endif
}

void mixer_setup(mixer_driver_t* const mixer)
{
	const platform_gpio_t* gpio = platform_gpio();

#if defined(JAWBREAKER) || defined(HACKRF_ONE) || defined(PRALINE)
	mixer->gpio_reset = gpio->rffc5072_reset;
	#if defined(PRALINE)
	mixer->gpio_ld = gpio->rffc5072_ld;
	#endif
	rffc5071_setup(mixer);
#elif defined(RAD1O)
	mixer->gpio_vco_ce = gpio->vco_ce;
	mixer->gpio_vco_sclk = gpio->vco_sclk;
	mixer->gpio_vco_sdata = gpio->vco_sdata;
	mixer->gpio_vco_le = gpio->vco_le;
	mixer->gpio_synt_rfout_en = gpio->synt_rfout_en;
	mixer->gpio_vco_mux = gpio->vco_mux;
	max2871_setup(mixer);
#endif
}

fp_40_24_t mixer_set_frequency(mixer_driver_t* const mixer, fp_40_24_t lo)
{
#if defined(JAWBREAKER) || defined(HACKRF_ONE) || defined(PRALINE)
	return rffc5071_set_frequency(mixer, lo);
#elif defined(RAD1O)
	return max2871_set_frequency(mixer, lo);
#endif
}

void mixer_enable(mixer_driver_t* const mixer)
{
#if defined(JAWBREAKER) || defined(HACKRF_ONE) || defined(PRALINE)
	rffc5071_enable(mixer);
#elif defined(RAD1O)
	max2871_enable(mixer);
#endif
}

void mixer_disable(mixer_driver_t* const mixer)
{
#if defined(JAWBREAKER) || defined(HACKRF_ONE) || defined(PRALINE)
	rffc5071_disable(mixer);
#elif defined(RAD1O)
	max2871_disable(mixer);
#endif
}

void mixer_set_gpo(mixer_driver_t* const mixer, uint8_t gpo)
{
#if defined(JAWBREAKER) || defined(HACKRF_ONE) || defined(PRALINE)
	rffc5071_set_gpo(mixer, gpo);
#elif defined(RAD1O)
	(void) mixer;
	(void) gpo;
#endif
}
