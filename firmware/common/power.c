/*
 * Copyright 2026 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include "power.h"

#if defined(HACKRF_ONE)
	#include <stdint.h>
#endif
#if defined(HACKRF_ONE) || defined(RAD1O) || defined(JAWBREAKER)
	#include "platform_detect.h"
#endif
#if defined(PRALINE) || defined(RAD1O)
	#include "delay.h"
#endif

#include "gpio.h"
#include "platform_gpio.h"

#if defined(PRALINE)
void enable_1v2_power(void)
{
	gpio_set(platform_gpio()->gpio_1v2_enable);
}

void disable_1v2_power(void)
{
	gpio_clear(platform_gpio()->gpio_1v2_enable);
}

void enable_3v3aux_power(void)
{
	gpio_clear(platform_gpio()->gpio_3v3aux_enable_n);
}

void disable_3v3aux_power(void)
{
	gpio_set(platform_gpio()->gpio_3v3aux_enable_n);
}

#else
void enable_1v8_power(void)
{
	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
	#ifdef HACKRF_ONE
		gpio_set(platform_gpio()->h1r9_1v8_enable);
	#endif
	} else {
		gpio_set(platform_gpio()->gpio_1v8_enable);
	}
}

void disable_1v8_power(void)
{
	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
	#ifdef HACKRF_ONE
		gpio_clear(platform_gpio()->h1r9_1v8_enable);
	#endif
	} else {
		gpio_clear(platform_gpio()->gpio_1v8_enable);
	}
}
#endif

#if defined(HACKRF_ONE)
void enable_rf_power(void)
{
	const platform_gpio_t* gpio = platform_gpio();
	uint32_t i;

	/* many short pulses to avoid one big voltage glitch */
	for (i = 0; i < 1000; i++) {
		if (detected_platform() == BOARD_ID_HACKRF1_R9) {
			gpio_set(gpio->h1r9_vaa_disable);
			gpio_clear(gpio->h1r9_vaa_disable);
		} else {
			gpio_set(gpio->vaa_disable);
			gpio_clear(gpio->vaa_disable);
		}
	}
}

void disable_rf_power(void)
{
	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
		gpio_set(platform_gpio()->h1r9_vaa_disable);
	} else {
		gpio_set(platform_gpio()->vaa_disable);
	}
}

#elif defined(PRALINE)
void enable_rf_power(void)
{
	gpio_clear(platform_gpio()->vaa_disable);

	/* Let the voltage stabilize */
	delay(1000000);
}

void disable_rf_power(void)
{
	gpio_set(platform_gpio()->vaa_disable);
}

#elif defined(RAD1O)
void enable_rf_power(void)
{
	gpio_set(platform_gpio()->vaa_enable);

	/* Let the voltage stabilize */
	delay(1000000);
}

void disable_rf_power(void)
{
	gpio_clear(platform_gpio()->vaa_enable);
}
#endif
