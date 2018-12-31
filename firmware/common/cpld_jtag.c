/*
 * Copyright 2013 Michael Ossmann <mike@ossmann.com>
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

#include "cpld_jtag.h"
#include "hackrf_core.h"
#include "xapp058/micro.h"
#include <libopencm3/lpc43xx/scu.h>
#include <stdint.h>

static refill_buffer_cb refill_buffer;
static uint32_t xsvf_buffer_len, xsvf_pos;
static unsigned char* xsvf_buffer;

void cpld_jtag_init(jtag_t* const jtag) {
	/* Initialize pin GPIOs in "released" state. */
	cpld_jtag_release(jtag);

	/* TDI and TMS pull-ups are required in all JTAG-compliant devices.
	 *
	 * The HackRF CPLD is always present, so let the CPLD pull up its TDI and TMS.
	 *
	 * The PortaPack may not be present, so pull up the PortaPack TMS pin from the
	 * microcontroller.
	 *
	 * TCK is recommended to be held low, so use microcontroller pull-down.
	 *
	 * TDO is undriven except when in Shift-IR or Shift-DR phases.
	 * Use the microcontroller to pull down to keep from floating.
	 *
	 * LPC43xx pull-up and pull-down resistors are approximately 53K.
	 */
#ifdef USER_INTERFACE_PORTAPACK
	scu_pinmux(SCU_PINMUX_PP_TMS,   SCU_GPIO_PUP    | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_PP_TDO,   SCU_GPIO_PDN    | SCU_CONF_FUNCTION0);
#endif
	scu_pinmux(SCU_PINMUX_CPLD_TMS, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_CPLD_TDI, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_CPLD_TDO, SCU_GPIO_PDN    | SCU_CONF_FUNCTION4);
	scu_pinmux(SCU_PINMUX_CPLD_TCK, SCU_GPIO_PDN    | SCU_CONF_FUNCTION0);
}

void cpld_jtag_take(jtag_t* const jtag) {
	const jtag_gpio_t* const gpio = jtag->gpio;

#ifdef USER_INTERFACE_PORTAPACK
	/* Do not drive PortaPack-specific TMS pin initially, just to be cautious. */
	gpio_input(gpio->gpio_pp_tms);
	gpio_input(gpio->gpio_pp_tdo);
#endif
	gpio_output(gpio->gpio_tms);
	gpio_output(gpio->gpio_tdi);
	gpio_output(gpio->gpio_tck);
	gpio_input(gpio->gpio_tdo);
}

void cpld_jtag_release(jtag_t* const jtag) {
	const jtag_gpio_t* const gpio = jtag->gpio;

	/* Make all pins inputs when JTAG interface not active.
	 * Let the pull-ups/downs do the work.
	 */
#ifdef USER_INTERFACE_PORTAPACK
	/* Do not drive PortaPack-specific pins, initially, just to be cautious. */
	gpio_input(gpio->gpio_pp_tms);
	gpio_input(gpio->gpio_pp_tdo);
#endif
	gpio_input(gpio->gpio_tms);
	gpio_input(gpio->gpio_tdi);
	gpio_input(gpio->gpio_tck);
	gpio_input(gpio->gpio_tdo);

	/* Set initial GPIO state to the voltages of the internal or external pull-ups/downs,
	 * to avoid any glitches.
	 */
#ifdef USER_INTERFACE_PORTAPACK
	gpio_set(gpio->gpio_pp_tms);
#endif
	gpio_set(gpio->gpio_tms);
	gpio_set(gpio->gpio_tdi);
	gpio_clear(gpio->gpio_tck);
}

/* return 0 if success else return error code see xsvfExecute() */
int cpld_jtag_program(
		jtag_t* const jtag,
        const uint32_t buffer_length,
        unsigned char* const buffer,
        refill_buffer_cb refill
) {
	int error;
	cpld_jtag_take(jtag);
	xsvf_buffer = buffer;
	xsvf_buffer_len = buffer_length;
        refill_buffer = refill;
	error = xsvfExecute(jtag->gpio);
	cpld_jtag_release(jtag);
	
	return error;
}

/* this gets called by the XAPP058 code */
unsigned char cpld_jtag_get_next_byte(void) {
        if (xsvf_pos == xsvf_buffer_len) {
                refill_buffer();
                xsvf_pos = 0;
        }

	unsigned char byte = xsvf_buffer[xsvf_pos];
        xsvf_pos++;
	return byte;
}
