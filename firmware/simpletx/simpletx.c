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

#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/i2c.h>
#include <libopencm3/lpc43xx/ssp.h>

#include "hackrf_core.h"
#include "max2837.h"

void pin_setup(void)
{
	/* Configure SCU Pin Mux as GPIO */
	scu_pinmux(SCU_PINMUX_LED1, SCU_GPIO_FAST);
	scu_pinmux(SCU_PINMUX_LED2, SCU_GPIO_FAST);
	scu_pinmux(SCU_PINMUX_LED3, SCU_GPIO_FAST);

	scu_pinmux(SCU_PINMUX_EN1V8, SCU_GPIO_FAST);

	/* Configure all GPIO as Input (safe state) */
	GPIO0_DIR = 0;
	GPIO1_DIR = 0;
	GPIO2_DIR = 0;
	GPIO3_DIR = 0;
	GPIO4_DIR = 0;
	GPIO5_DIR = 0;
	GPIO6_DIR = 0;
	GPIO7_DIR = 0;

	/* Configure GPIO2[1/2/8] (P4_1/2 P6_12) as output. */
	GPIO2_DIR |= (PIN_LED1|PIN_LED2|PIN_LED3);

	/* GPIO3[6] on P6_10  as output. */
	GPIO3_DIR |= PIN_EN1V8;

	/* Configure SSP1 Peripheral (to be moved later in SSP driver) */
	scu_pinmux(SCU_SSP1_MISO, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP1_MOSI, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP1_SCK,  (SCU_SSP_IO | SCU_CONF_FUNCTION1));
	scu_pinmux(SCU_SSP1_SSEL, (SCU_SSP_IO | SCU_CONF_FUNCTION1));
}

int main(void)
{
	u8 serial_clock_rate;
	u8 clock_prescale_rate;

	pin_setup();
	gpio_set(PORT_EN1V8, PIN_EN1V8); /* 1V8 on */
	cpu_clock_init();

	gpio_set(PORT_LED1_3, (PIN_LED1)); /* LED1 on */

	clock_prescale_rate = 2;
	serial_clock_rate = 128;

	ssp_init(SSP1_NUM,
			SSP_DATA_16BITS,
			SSP_FRAME_SPI,
			SSP_CPOL_0_CPHA_0,
			serial_clock_rate,
			clock_prescale_rate,
			SSP_MODE_NORMAL,
			SSP_MASTER,
			SSP_SLAVE_OUT_ENABLE);

	max2837_init();
	max2837_start();
	max2837_set_frequency(2441000000);
	while (1);
	max2837_stop();

	return 0;
}
