/*
 * Copyright 2010 - 2013 Michael Ossmann
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

#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/scu.h>

#include "hackrf_core.h"
#include "cpld_jtag.h"
#include "sgpio_if_xsvf.h"

#define WAIT_LOOP_DELAY (6000000)

#define ALL_LEDS	(PIN_LED1|PIN_LED2|PIN_LED3)

int main(void)
{
	int i;
	int error;
	
	pin_setup();

	/* Set 1V8 */
	gpio_set(PORT_EN1V8, PIN_EN1V8);

	cpu_clock_init();
	
	/* program test bitstream to CPLD */
	error = cpld_jtag_program(sgpio_if_xsvf_len, &sgpio_if_xsvf[0]);

	gpio_clear(PORT_LED1_3, ALL_LEDS); /* All LEDs off */

	if (error == 0) {
		/* blink LED1, LED2, and LED3 on success */
		while (1) {
			gpio_set(PORT_LED1_3, ALL_LEDS); /* LEDs on */
			for (i = 0; i < WAIT_LOOP_DELAY; i++)	/* Wait a bit. */
				__asm__("nop");
			gpio_clear(PORT_LED1_3, ALL_LEDS); /* LEDs off */
			for (i = 0; i < WAIT_LOOP_DELAY; i++)	/* Wait a bit. */
				__asm__("nop");
		}
	} else {
		/* LED3 (Red) steady on error */
		gpio_set(PORT_LED1_3, PIN_LED3); /* LEDs on */
		while (1);
	}

	return 0;
}
