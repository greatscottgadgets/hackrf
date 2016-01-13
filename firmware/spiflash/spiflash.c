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
	uint8_t buf[515];

	pin_setup();

	enable_1v8_power();

	cpu_clock_init();

	/* program test data to SPI flash */
	for (i = 0; i < 515; i++)
		buf[i] = (i * 3) & 0xFF;
	w25q80bv_setup(&w25q80bv);
	w25q80bv_chip_erase(&w25q80bv);
	w25q80bv_program(&w25q80bv, 790, 515, &buf[0]);

	/* blink LED1 and LED3 */
	while (1) 
	{
		led_on(LED1);
		led_on(LED3);
		for (i = 0; i < 8000000; i++)	/* Wait a bit. */
			__asm__("nop");
		led_off(LED1);
		led_off(LED3);
		for (i = 0; i < 8000000; i++)	/* Wait a bit. */
			__asm__("nop");
	}

	return 0;
}
