/*
 * Copyright 2010-2017 Great Scott Gadgets <info@greatscottgadgets.com>
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
#include "platform_detect.h"
#include "delay.h"

int main(void)
{
	detect_hardware_platform();
	pin_setup();

#ifndef PRALINE
	/* enable 1V8 power supply so that the 1V8 LED lights up */
	enable_1v8_power();
#else
	/* enable 1V2 power supply so that the 3V3FPGA LED lights up */
	enable_1v2_power();
#endif

	/* Blink LED1/2/3 on the board. */
	while (1) 
	{
		led_on(LED1);
		led_on(LED2);
		led_on(LED3);

		delay(2000000);
		
		led_off(LED1);
		led_off(LED2);
		led_off(LED3);
		
		delay(2000000);
	}

	return 0;
}
