/*
 * Copyright 2012-2022 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2012 Will Code <willcode4@gmail.com>
 * Copyright 2014 Jared Boone <jared@sharebrained.com>
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

#include "max2837_target.h"

#include <libopencm3/lpc43xx/scu.h>
#include "hackrf_core.h"

void max2837_target_init(max2837_driver_t* const drv)
{
	/* Configure SSP1 Peripheral (to be moved later in SSP driver) */
	scu_pinmux(SCU_SSP1_CIPO, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP1_COPI, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP1_SCK, (SCU_SSP_IO | SCU_CONF_FUNCTION1));

	scu_pinmux(SCU_XCVR_CS, SCU_XCVR_CS_PINCFG);

	/* Configure XCVR_CTL GPIO pins. */
	scu_pinmux(SCU_XCVR_ENABLE, SCU_XCVR_ENABLE_PINCFG);
	scu_pinmux(SCU_XCVR_RXENABLE, SCU_XCVR_RXENABLE_PINCFG);
	scu_pinmux(SCU_XCVR_TXENABLE, SCU_XCVR_TXENABLE_PINCFG);

	/* Set GPIO pins as outputs. */
	gpio_output(drv->gpio_enable);
	gpio_output(drv->gpio_rx_enable);
	gpio_output(drv->gpio_tx_enable);
}

void max2837_target_set_mode(max2837_driver_t* const drv, const max2837_mode_t new_mode)
{
	/* MAX2837_MODE_SHUTDOWN:
	 * All circuit blocks are powered down, except the 4-wire serial bus
	 * and its internal programmable registers.
	 *
	 * MAX2837_MODE_STANDBY:
	 * Used to enable the frequency synthesizer block while the rest of the
	 * device is powered down. In this mode, PLL, VCO, and LO generator
	 * are on, so that Tx or Rx modes can be quickly enabled from this mode.
	 * These and other blocks can be selectively enabled in this mode.
	 *
	 * MAX2837_MODE_TX:
	 * All Tx circuit blocks are powered on. The external PA is powered on
	 * after a programmable delay using the on-chip PA bias DAC. The slow-
	 * charging Rx circuits are in a precharged “idle-off” state for fast
	 * Tx-to-Rx turnaround time.
	 *
	 * MAX2837_MODE_RX:
	 * All Rx circuit blocks are powered on and active. Antenna signal is
	 * applied; RF is downconverted, filtered, and buffered at Rx BB I and Q
	 * outputs. The slow- charging Tx circuits are in a precharged “idle-off”
	 * state for fast Rx-to-Tx turnaround time.
	 */
	gpio_write(drv->gpio_enable, new_mode != MAX2837_MODE_SHUTDOWN);
	gpio_write(drv->gpio_rx_enable, new_mode == MAX2837_MODE_RX);
	gpio_write(drv->gpio_tx_enable, new_mode == MAX2837_MODE_TX);
	drv->mode = new_mode;
}
