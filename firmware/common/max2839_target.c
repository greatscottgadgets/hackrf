/*
 * Copyright 2012-2022 Great Scott Gadgets
 * Copyright 2014 Jared Boone <jared@sharebrained.com>
 * Copyright 2012 Will Code <willcode4@gmail.com>
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

#include "max2839_target.h"

#include "platform_scu.h"

void max2839_target_init(max2839_driver_t* const drv)
{
	const platform_scu_t* scu = platform_scu();

	/* Configure SSP1 Peripheral (to be moved later in SSP driver) */
	scu_pinmux(scu->SSP1_CIPO, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(scu->SSP1_COPI, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(scu->SSP1_SCK, (SCU_SSP_IO | SCU_CONF_FUNCTION1));

	scu_pinmux(scu->XCVR_CS, SCU_GPIO_FAST);

	/*
	 * Configure XCVR_CTL GPIO pins.
	 *
	 * The RXTX pin is also known as RXENABLE because of its use on the
	 * MAX2837 which had a separate TXENABLE. On MAX2839 a single RXTX pin
	 * switches between RX (high) and TX (low) modes.
 	 */
	scu_pinmux(scu->XCVR_ENABLE, SCU_GPIO_FAST);
	scu_pinmux(scu->XCVR_RXENABLE, SCU_GPIO_FAST);

	/* Set GPIO pins as outputs. */
	gpio_output(drv->gpio_enable);
	gpio_output(drv->gpio_rxtx);
}

void max2839_target_set_mode(max2839_driver_t* const drv, const max2839_mode_t new_mode)
{
	/* MAX2839_MODE_SHUTDOWN:
	 * All circuit blocks are powered down, except the 4-wire serial bus
	 * and its internal programmable registers.
	 *
	 * MAX2839_MODE_STANDBY:
	 * Used to enable the frequency synthesizer block while the rest of the
	 * device is powered down. In this mode, PLL, VCO, and LO generator
	 * are on, so that Tx or Rx modes can be quickly enabled from this mode.
	 * These and other blocks can be selectively enabled in this mode.
	 *
	 * MAX2839_MODE_TX:
	 * All Tx circuit blocks are powered on. The external PA is powered on
	 * after a programmable delay using the on-chip PA bias DAC. The slow-
	 * charging Rx circuits are in a precharged “idle-off” state for fast
	 * Tx-to-Rx turnaround time.
	 *
	 * MAX2839_MODE_RX:
	 * All Rx circuit blocks are powered on and active. Antenna signal is
	 * applied; RF is downconverted, filtered, and buffered at Rx BB I and Q
	 * outputs. The slow- charging Tx circuits are in a precharged “idle-off”
	 * state for fast Rx-to-Tx turnaround time.
	 */

	switch (new_mode) {
	default:
	case MAX2839_MODE_SHUTDOWN:
		gpio_clear(drv->gpio_enable);
		gpio_clear(drv->gpio_rxtx);
		break;
	case MAX2839_MODE_STANDBY:
		gpio_clear(drv->gpio_enable);
		gpio_set(drv->gpio_rxtx);
		break;
	case MAX2839_MODE_TX:
		gpio_set(drv->gpio_enable);
		gpio_clear(drv->gpio_rxtx);
		break;
	case MAX2839_MODE_RX:
		gpio_set(drv->gpio_enable);
		gpio_set(drv->gpio_rxtx);
		break;
	}
	drv->mode = new_mode;
}
