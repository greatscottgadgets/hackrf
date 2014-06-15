/*
 * Copyright 2010 - 2012 Michael Ossmann
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

uint32_t boot0, boot1, boot2, boot3;

int main(void)
{
	int i;
	pin_setup();

	/* enable all power supplies */
	enable_1v8_power();

	/* Blink LED1/2/3 on the board and Read BOOT0/1/2/3 pins. */
	while (1) 
	{
		boot0 = BOOT0_STATE;
		boot1 = BOOT1_STATE;
		boot2 = BOOT2_STATE;
		boot3 = BOOT3_STATE;

		gpio_set(PORT_LED1_3, (PIN_LED1|PIN_LED2|PIN_LED3)); /* LEDs on */
		for (i = 0; i < 2000000; i++)	/* Wait a bit. */
			__asm__("nop");
		gpio_clear(PORT_LED1_3, (PIN_LED1|PIN_LED2|PIN_LED3)); /* LED off */
		for (i = 0; i < 2000000; i++)	/* Wait a bit. */
			__asm__("nop");
	}

	return 0;
}
