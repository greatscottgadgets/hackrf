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

#include "leds.h"

#include <stdint.h>

#include "delay.h"
#include "gpio.h"
#include "platform_gpio.h"

void led_on(const led_t led)
{
#if defined(PRALINE)
	gpio_clear(platform_gpio()->led[led]);
#else
	gpio_set(platform_gpio()->led[led]);
#endif
}

void led_off(const led_t led)
{
#if defined(PRALINE)
	gpio_set(platform_gpio()->led[led]);
#else
	gpio_clear(platform_gpio()->led[led]);
#endif
}

void led_toggle(const led_t led)
{
	gpio_toggle(platform_gpio()->led[led]);
}

void set_leds(const uint8_t state)
{
	int num_leds = 3;
#if (defined RAD1O || defined PRALINE)
	num_leds = 4;
#endif
	for (int i = 0; i < num_leds; i++) {
#ifdef PRALINE
		gpio_write(platform_gpio()->led[i], ((state >> i) & 1) == 0);
#else
		gpio_write(platform_gpio()->led[i], ((state >> i) & 1) == 1);
#endif
	}
}

void halt_and_flash(const uint32_t duration)
{
	/* blink LED1, LED2, and LED3 */
	while (1) {
		led_on(LED1);
		led_on(LED2);
		led_on(LED3);
		delay(duration);
		led_off(LED1);
		led_off(LED2);
		led_off(LED3);
		delay(duration);
	}
}
