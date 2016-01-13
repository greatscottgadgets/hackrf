/*
 * Copyright 2012 Michael Ossmann
 * Copyright (C) 2012 Jared Boone
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
#include <libopencm3/lpc43xx/sgpio.h>

#include <hackrf_core.h>

volatile uint32_t buffer[4096];

void tx_test() {
	sgpio_set_slice_mode(&sgpio_config, false);
	sgpio_configure(&sgpio_config, TRANSCEIVER_MODE_TX);

	// LSB goes out first, samples are 0x<Q1><I1><Q0><I0>
	buffer[0] = 0xda808080;
	buffer[1] = 0xda80ff80;
	buffer[2] = 0x26808080;
	buffer[3] = 0x26800180;

	uint32_t i = 0;

	sgpio_cpld_stream_enable(&sgpio_config);

	while(true) {
		while(SGPIO_STATUS_1 == 0);
		SGPIO_REG_SS(SGPIO_SLICE_A) = buffer[(i++) & 3];
		SGPIO_CLR_STATUS_1 = 1;
	}
}

void rx_test() {
	sgpio_set_slice_mode(&sgpio_config, false);
	sgpio_configure(&sgpio_config, TRANSCEIVER_MODE_RX);
    
    uint32_t i = 0;

	sgpio_cpld_stream_enable(&sgpio_config);

    while(true) {
        while(SGPIO_STATUS_1 == 0);
        SGPIO_CLR_STATUS_1 = 1;
        buffer[i++ & 4095] = SGPIO_REG_SS(SGPIO_SLICE_A);
    }
}

int main(void) {
	pin_setup();
	enable_1v8_power();
	cpu_clock_init();

	led_on(LED1);

	ssp1_set_mode_max5864();
	max5864_setup(&max5864);
	max5864_xcvr(&max5864);

	while (1) {

	}

	return 0;
}
