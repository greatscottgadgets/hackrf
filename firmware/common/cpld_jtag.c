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
#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/scu.h>
#include <stdint.h>

static refill_buffer_cb refill_buffer;
static uint32_t xsvf_buffer_len, xsvf_pos;
static unsigned char* xsvf_buffer;

void cpld_jtag_setup(void) {
	scu_pinmux(SCU_PINMUX_CPLD_TDO, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION4);
	scu_pinmux(SCU_PINMUX_CPLD_TCK, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_CPLD_TMS, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_CPLD_TDI, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
	
	/* TDO is an input */
	GPIO_DIR(PORT_CPLD_TDO) &= ~PIN_CPLD_TDO;

	/* the rest are outputs */
	GPIO_DIR(PORT_CPLD_TCK) |= PIN_CPLD_TCK;
	GPIO_DIR(PORT_CPLD_TMS) |= PIN_CPLD_TMS;
	GPIO_DIR(PORT_CPLD_TDI) |= PIN_CPLD_TDI;
}

/* set pins as inputs so we don't interfere with an external JTAG device */
void cpld_jtag_release(void) {
	scu_pinmux(SCU_PINMUX_CPLD_TDO, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION4);
	scu_pinmux(SCU_PINMUX_CPLD_TCK, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_CPLD_TMS, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_CPLD_TDI, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
	
	GPIO_DIR(PORT_CPLD_TDO) &= ~PIN_CPLD_TDO;
	GPIO_DIR(PORT_CPLD_TCK) &= ~PIN_CPLD_TCK;
	GPIO_DIR(PORT_CPLD_TMS) &= ~PIN_CPLD_TMS;
	GPIO_DIR(PORT_CPLD_TDI) &= ~PIN_CPLD_TDI;
}

/* return 0 if success else return error code see xsvfExecute() */
int cpld_jtag_program(
        const uint32_t buffer_length,
        unsigned char* const buffer,
        refill_buffer_cb refill
) {
	int error;
	cpld_jtag_setup();
	xsvf_buffer = buffer;
	xsvf_buffer_len = buffer_length;
        refill_buffer = refill;
	error = xsvfExecute();
	cpld_jtag_release();
	
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
