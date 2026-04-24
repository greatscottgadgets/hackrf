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

#if defined(IS_HACKRF_ONE)
	#include <stdint.h>
#endif

#include "gpio.h"
#include "platform_gpio.h"
#if defined(IS_HACKRF_ONE)
	#include "platform_detect.h"
#endif
#if defined(IS_PRALINE) || defined(IS_RAD1O)
	#include "delay.h"
#endif

#ifdef IS_PRALINE
void enable_1v2_power(void)
{
	if (IS_PRALINE) {
		gpio_set(platform_gpio()->gpio_1v2_enable);
	}
}

void disable_1v2_power(void)
{
	if (IS_PRALINE) {
		gpio_clear(platform_gpio()->gpio_1v2_enable);
	}
}

void enable_3v3aux_power(void)
{
	if (IS_PRALINE) {
		gpio_clear(platform_gpio()->gpio_3v3aux_enable_n);
	}
}

void disable_3v3aux_power(void)
{
	if (IS_PRALINE) {
		gpio_set(platform_gpio()->gpio_3v3aux_enable_n);
	}
}
#endif

void enable_1v8_power(void)
{
#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
	#ifdef IS_H1_R9
		if (IS_H1_R9) {
			gpio_set(platform_gpio()->h1r9_1v8_enable);
		}
	#endif
	#ifdef IS_NOT_H1_R9
		if (IS_NOT_H1_R9) {
			gpio_set(platform_gpio()->gpio_1v8_enable);
		}
	#endif
	}
#endif
}

void disable_1v8_power(void)
{
#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
	#ifdef IS_H1_R9
		if (IS_H1_R9) {
			gpio_clear(platform_gpio()->h1r9_1v8_enable);
		}
	#endif
	#ifdef IS_NOT_H1_R9
		if (IS_NOT_H1_R9) {
			gpio_clear(platform_gpio()->gpio_1v8_enable);
		}
	#endif
	}
#endif
}

#ifdef IS_HACKRF_ONE
static inline void enable_rf_power_hackrf_one(void)
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

static inline void disable_rf_power_hackrf_one(void)
{
	if (detected_platform() == BOARD_ID_HACKRF1_R9) {
		gpio_set(platform_gpio()->h1r9_vaa_disable);
	} else {
		gpio_set(platform_gpio()->vaa_disable);
	}
}
#endif

#ifdef IS_PRALINE
static inline void enable_rf_power_praline(void)
{
	gpio_clear(platform_gpio()->vaa_disable);

	/* Let the voltage stabilize */
	delay(1000000);
}

static inline void disable_rf_power_praline(void)
{
	gpio_set(platform_gpio()->vaa_disable);
}
#endif

#ifdef IS_RAD1O
static inline void enable_rf_power_rad1o(void)
{
	gpio_set(platform_gpio()->vaa_enable);

	/* Let the voltage stabilize */
	delay(1000000);
}

static inline void disable_rf_power_rad1o(void)
{
	gpio_clear(platform_gpio()->vaa_enable);
}
#endif

void enable_rf_power(void)
{
#ifdef IS_HACKRF_ONE
	if (IS_HACKRF_ONE) {
		enable_rf_power_hackrf_one();
	}
#endif
#ifdef IS_PRALINE
	if (IS_PRALINE) {
		enable_rf_power_praline();
	}
#endif
#ifdef IS_RAD1O
	if (IS_RAD1O) {
		enable_rf_power_rad1o();
	}
#endif
}

void disable_rf_power(void)
{
#ifdef IS_HACKRF_ONE
	if (IS_HACKRF_ONE) {
		disable_rf_power_hackrf_one();
	}
#endif
#ifdef IS_PRALINE
	if (IS_PRALINE) {
		disable_rf_power_praline();
	}
#endif
#ifdef IS_RAD1O
	if (IS_RAD1O) {
		disable_rf_power_rad1o();
	}
#endif
}
