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

#include "hackrf_core.h"

int main(void)
{
	int i;
	pin_setup();

	/* enable all power supplies */
	enable_1v8_power();

	/* Blink LED1/2/3 on the board. */
	while (1) 
	{
		led_on(LED1);
		led_on(LED2);
		led_on(LED3);

		for (i = 0; i < 2000000; i++)	/* Wait a bit. */
			__asm__("nop");
		
		led_off(LED1);
		led_off(LED2);
		led_off(LED3);
		
		for (i = 0; i < 2000000; i++)	/* Wait a bit. */
			__asm__("nop");
	}

	return 0;
}
