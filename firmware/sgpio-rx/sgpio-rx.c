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
#include <tuning.h>

void tx_test() {
	sgpio_set_slice_mode(&sgpio_config, false);
	sgpio_configure(&sgpio_config, TRANSCEIVER_MODE_TX);
	
	// LSB goes out first, samples are 0x<Q1><I1><Q0><I0>
	volatile uint32_t buffer[] = {
		0xda808080,
		0xda80ff80,
		0x26808080,
		0x26800180,
	};
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

    volatile uint32_t buffer[4096];
    uint32_t i = 0;
	int16_t magsq;
	int8_t sigi, sigq;

    sgpio_cpld_stream_enable(&sgpio_config);

	led_on(LED2);
    while(true) {
        while(SGPIO_STATUS_1 == 0);
		led_on(LED1);
        SGPIO_CLR_STATUS_1 = 1;
        buffer[i & 4095] = SGPIO_REG_SS(SGPIO_SLICE_A);

		/* find the magnitude squared */
		sigi = (buffer[i & 4095] & 0xff) - 0x80;
		sigq = ((buffer[i & 4095] >> 8) & 0xff) - 0x80;
		magsq = sigi * sigq;
		if ((uint16_t)magsq & 0x8000) {
			magsq ^= 0xffff;
			magsq++;
		}
		
		/* illuminate LED3 only when magsq exceeds threshold */
		if (magsq > 0x3c00)
			led_on(LED3);
		else
			led_off(LED3);
		i++;
    }
}

int main(void) {

	const uint64_t freq = 2700000000U;

	pin_setup();
	enable_1v8_power();
#ifdef HACKRF_ONE
	enable_rf_power();
#endif
	cpu_clock_init();
    rf_path_init(&rf_path);
    rf_path_set_direction(&rf_path, RF_PATH_DIRECTION_RX);

	set_freq(freq);

	rx_test();
	led_on(LED2);

	while (1) {

	}

	return 0;
}
