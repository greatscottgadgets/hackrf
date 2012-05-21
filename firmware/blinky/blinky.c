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

void wait(uint8_t duration)
{
	uint32_t i;
	for (i = 0; i < duration * 1000000; i++);
}

int main()
{
	gpio_init();

	while (1) {
		LED1_SET;
		LED2_SET;
		LED3_SET;
		wait(1);
		LED1_CLR;
		LED2_CLR;
		LED3_CLR;
		wait(1);
	}
}
