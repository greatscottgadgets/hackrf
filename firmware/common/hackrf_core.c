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

void gpio_init()
{
	/* 
	 * Basic GPIO setup for all pins.  This shouldn't be necessary after a
	 * reset, but we might get called at other times.
	 */
	all_pins_off();

	/* set certain pins as outputs, all others inputs */
	GPIO_DIR2 = (PIN_LED1 | PIN_LED2 | PIN_LED3);
}

void all_pins_off(void)
{
	/* configure all pins for GPIO? */

	/* configure all pins as inputs */
	GPIO_PIN0 = 0;
	GPIO_PIN1 = 0;
	GPIO_PIN2 = 0;
	GPIO_PIN3 = 0;
	GPIO_PIN4 = 0;
	GPIO_PIN5 = 0;
	GPIO_PIN6 = 0;
	GPIO_PIN7 = 0;

	/* pull-up on every pin? */

	/* set all outputs low? */
}
