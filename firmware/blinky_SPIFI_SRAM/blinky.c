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

#include <stdint.h>

#include "hackrf_core.h"

void wait(uint8_t duration)
{
	volatile uint32_t i;
	for (i = 0; i < duration * 1000000; i++);
}

uint32_t boot0, boot1, boot2, boot3;

int main()
{

	gpio_init();

	EN1V8_SET;
	EN1V8_CLR;

	while (1) {
	    boot0 = BOOT0;
	    boot1 = BOOT1;
	    boot2 = BOOT2;
	    boot3 = BOOT3;

		LED1_SET;
		LED2_SET;
		LED3_SET;
		wait(1);
		LED1_CLR;
		LED2_CLR;
		LED3_CLR;
		wait(1);
	}

	return 0 ;
}
