/*
 * Copyright 2012 Michael Ossmann
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
	const uint32_t freq = 2441000000U;

	pin_setup();
	enable_1v8_power();
#ifdef HACKRF_ONE
	enable_rf_power();
#endif
	cpu_clock_init();
    
	led_on(LED1);

	ssp1_set_mode_max2837();
	max2837_setup(&max2837);
	led_on(LED2);
	max2837_set_frequency(&max2837, freq);
	max2837_start(&max2837);
	max2837_tx(&max2837);
	led_on(LED3);
	while (1);
	max2837_stop(&max2837);

	return 0;
}
