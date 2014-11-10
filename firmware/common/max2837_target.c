/*
 * Copyright 2012 Will Code? (TODO: Proper attribution)
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
#include <libopencm3/lpc43xx/gpio.h>
#include "hackrf_core.h"

void max2837_target_init(max2837_driver_t* const drv) {
	(void)drv;
	
	scu_pinmux(SCU_XCVR_CS, SCU_GPIO_FAST);
	GPIO_SET(PORT_XCVR_CS) = PIN_XCVR_CS;
	GPIO_DIR(PORT_XCVR_CS) |= PIN_XCVR_CS;

	/* Configure XCVR_CTL GPIO pins. */
#ifdef JELLYBEAN
	scu_pinmux(SCU_XCVR_RXHP, SCU_GPIO_FAST);
	scu_pinmux(SCU_XCVR_B1, SCU_GPIO_FAST);
	scu_pinmux(SCU_XCVR_B2, SCU_GPIO_FAST);
	scu_pinmux(SCU_XCVR_B3, SCU_GPIO_FAST);
	scu_pinmux(SCU_XCVR_B4, SCU_GPIO_FAST);
	scu_pinmux(SCU_XCVR_B5, SCU_GPIO_FAST);
	scu_pinmux(SCU_XCVR_B6, SCU_GPIO_FAST);
	scu_pinmux(SCU_XCVR_B7, SCU_GPIO_FAST);
#endif
	scu_pinmux(SCU_XCVR_ENABLE, SCU_GPIO_FAST);
	scu_pinmux(SCU_XCVR_RXENABLE, SCU_GPIO_FAST);
	scu_pinmux(SCU_XCVR_TXENABLE, SCU_GPIO_FAST);

	/* Set GPIO pins as outputs. */
	GPIO2_DIR |= (PIN_XCVR_ENABLE | PIN_XCVR_RXENABLE | PIN_XCVR_TXENABLE);
#ifdef JELLYBEAN
	GPIO_DIR(PORT_XCVR_RXHP) |= PIN_XCVR_RXHP;
	GPIO_DIR(PORT_XCVR_B) |=
		  PIN_XCVR_B1
		| PIN_XCVR_B2
		| PIN_XCVR_B3
		| PIN_XCVR_B4
		| PIN_XCVR_B5
		| PIN_XCVR_B6
		| PIN_XCVR_B7
		;
#endif

#ifdef JELLYBEAN
	gpio_set(PORT_XCVR_RXHP, PIN_XCVR_RXHP);
	gpio_set(PORT_XCVR_B,
		  PIN_XCVR_B1
		| PIN_XCVR_B2
		| PIN_XCVR_B3
		| PIN_XCVR_B4
		| PIN_XCVR_B5
		| PIN_XCVR_B6
		| PIN_XCVR_B7
	);
#endif
}

void max2837_target_spi_select(spi_t* const spi) {
	(void)spi;
	gpio_clear(PORT_XCVR_CS, PIN_XCVR_CS);
}

void max2837_target_spi_unselect(spi_t* const spi) {
	(void)spi;
	gpio_set(PORT_XCVR_CS, PIN_XCVR_CS);
}

void max2837_mode_shutdown(max2837_driver_t* const drv) {
	(void)drv;
	/* All circuit blocks are powered down, except the 4-wire serial bus
	 * and its internal programmable registers.
	 */
	gpio_clear(PORT_XCVR_ENABLE,
			(PIN_XCVR_ENABLE | PIN_XCVR_RXENABLE | PIN_XCVR_TXENABLE));
}

void max2837_mode_standby(max2837_driver_t* const drv) {
	(void)drv;
	/* Used to enable the frequency synthesizer block while the rest of the
	 * device is powered down. In this mode, PLL, VCO, and LO generator
	 * are on, so that Tx or Rx modes can be quickly enabled from this mode.
	 * These and other blocks can be selectively enabled in this mode.
	 */
	gpio_clear(PORT_XCVR_ENABLE, (PIN_XCVR_RXENABLE | PIN_XCVR_TXENABLE));
	gpio_set(PORT_XCVR_ENABLE, PIN_XCVR_ENABLE);
}

void max2837_mode_tx(max2837_driver_t* const drv) {
	(void)drv;
	/* All Tx circuit blocks are powered on. The external PA is powered on
	 * after a programmable delay using the on-chip PA bias DAC. The slow-
	 * charging Rx circuits are in a precharged “idle-off” state for fast
	 * Tx-to-Rx turnaround time.
	 */
	gpio_clear(PORT_XCVR_ENABLE, PIN_XCVR_RXENABLE);
	gpio_set(PORT_XCVR_ENABLE,
			(PIN_XCVR_ENABLE | PIN_XCVR_TXENABLE));
}

void max2837_mode_rx(max2837_driver_t* const drv) {
	(void)drv;
	/* All Rx circuit blocks are powered on and active. Antenna signal is
	 * applied; RF is downconverted, filtered, and buffered at Rx BB I and Q
	 * outputs. The slow- charging Tx circuits are in a precharged “idle-off”
	 * state for fast Rx-to-Tx turnaround time.
	 */
	gpio_clear(PORT_XCVR_ENABLE, PIN_XCVR_TXENABLE);
	gpio_set(PORT_XCVR_ENABLE,
			(PIN_XCVR_ENABLE | PIN_XCVR_RXENABLE));
}

max2837_mode_t max2837_mode(max2837_driver_t* const drv) {
	(void)drv;
	if( gpio_get(PORT_XCVR_ENABLE, PIN_XCVR_ENABLE) ) {
		if( gpio_get(PORT_XCVR_ENABLE, PIN_XCVR_TXENABLE) ) {
			return MAX2837_MODE_TX;
		} else if( gpio_get(PORT_XCVR_ENABLE, PIN_XCVR_RXENABLE) ) {
			return MAX2837_MODE_RX;
		} else {
			return MAX2837_MODE_STANDBY;
		}
	} else {
		return MAX2837_MODE_SHUTDOWN;
	}
}
