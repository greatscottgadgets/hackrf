/*
 * Copyright 2018 Schuyler St. Leger
 * Copyright 2021 Great Scott Gadgets
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

#include "operacake_sctimer.h"

#include <hackrf_core.h>

#include <libopencm3/lpc43xx/sgpio.h>
#include <libopencm3/lpc43xx/rgu.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/gima.h>
#include "sct.h"

#define U1CTRL_SET  SCT_OUT14_SET
#define U1CTRL_CLR  SCT_OUT14_CLR
#define U2CTRL0_SET SCT_OUT13_SET
#define U2CTRL0_CLR SCT_OUT13_CLR
#define U2CTRL1_SET SCT_OUT12_SET
#define U2CTRL1_CLR SCT_OUT12_CLR
#define U3CTRL0_SET SCT_OUT11_SET
#define U3CTRL0_CLR SCT_OUT11_CLR
#define U3CTRL1_SET SCT_OUT8_SET
#define U3CTRL1_CLR SCT_OUT8_CLR

static uint32_t default_output = 0;

/**
 * Configure the SCTimer to rotate the antennas with the Operacake in phase with
 * the sample clock. This will configure the SCTimer to output to the pins for
 * GPIO control of the Operacake, however the Operacake must be configured for
 * GPIO control. The timing is not set in this function.
 *
 * To trigger the antenna switching synchronously with the sample clock, the
 * SGPIO is configured to output its clock (f=2 * sample clock) to the SCTimer.
 */
void operacake_sctimer_init()
{
	// We start by resetting the SCTimer
	RESET_CTRL1 = RESET_CTRL1_SCT_RST;

	// Delay to allow reset sigal to be processed
	// The reset generator uses a 12MHz clock (IRC)
	// The difference between this and the core clock (CCLK)
	// determines this value (CCLK/IRC).
	// The value used here is a bit shorter than would be required, since
	// there are additional instructions that fill the time. If the duration of
	// the actions from here to the first access to the SCTimer is changed, then
	// this delay may need to be increased.
	delay(8);

	// Pin definitions for the HackRF
	// U2CTRL0
	scu_pinmux(
		P7_4,
		SCU_CONF_EPUN_DIS_PULLUP | SCU_CONF_EHS_FAST | SCU_CONF_FUNCTION1);
	// U2CTRL1
	scu_pinmux(
		P7_5,
		SCU_CONF_EPUN_DIS_PULLUP | SCU_CONF_EHS_FAST | SCU_CONF_FUNCTION1);
	// U3CTRL0
	scu_pinmux(
		P7_6,
		SCU_CONF_EPUN_DIS_PULLUP | SCU_CONF_EHS_FAST | SCU_CONF_FUNCTION1);
	// U3CTRL1
	scu_pinmux(
		P7_7,
		SCU_CONF_EPUN_DIS_PULLUP | SCU_CONF_EHS_FAST | SCU_CONF_FUNCTION1);
	// U1CTRL
	scu_pinmux(
		P7_0,
		SCU_CONF_EPUN_DIS_PULLUP | SCU_CONF_EHS_FAST | SCU_CONF_FUNCTION1);

	// Configure the SGPIO to output the clock (f=2 * sample clock) on pin 12
	SGPIO_OUT_MUX_CFG12 = SGPIO_OUT_MUX_CFG_P_OUT_CFG(0x08) | // clkout output mode
		SGPIO_OUT_MUX_CFG_P_OE_CFG(0);                    // gpio_oe
	SGPIO_GPIO_OENREG |= BIT12;

	// Use the GIMA to connect the SGPIO clock to the SCTimer
	GIMA_CTIN_1_IN = 0x2 << 4; // Route SGPIO12 to SCTIN1

	// We configure this register first, because the user manual says to
	SCT_CONFIG |= SCT_CONFIG_UNIFY_32_BIT | SCT_CONFIG_CLKMODE_PRESCALED_BUS_CLOCK |
		SCT_CONFIG_CKSEL_RISING_EDGES_ON_INPUT_1;

	// Halt the SCTimer to enable it to be configured
	SCT_CTRL = SCT_CTRL_HALT_L(1);

	// Prescaler - run at half the SGPIO clock (ie: at the sample clock)
	SCT_CTRL |= SCT_CTRL_PRE_L(1);

	// Default to state 0, events disabled
	SCT_STATE = 0;

	// Enable the SCTimer
	SCT_CTRL &= ~SCT_CTRL_HALT_L(1);
}

static uint32_t operacake_sctimer_port_to_output(uint8_t port)
{
	int bit0 = (port >> 0) & 1;
	int bit1 = (port >> 1) & 1;
	int bit2 = (port >> 2) & 1;

	return (bit0 << 11) | (bit0 << 13) | (bit1 << 8) | (bit1 << 12) |
		(((~bit2) & 1) << 14);
}

void operacake_sctimer_enable(bool enable)
{
	SCT_CTRL = SCT_CTRL_HALT_L(1);
	SCT_STATE = enable;
	SCT_CTRL &= ~SCT_CTRL_HALT_L(1);
}

void operacake_sctimer_set_dwell_times(struct operacake_dwell_times* times, int n)
{
	SCT_CTRL = SCT_CTRL_HALT_L(1);

	uint32_t counter = 0;
	uint32_t bit0_set = 0, bit0_clr = 0, bit1_set = 0, bit1_clr = 0, bit2_set = 0,
		 bit2_clr = 0;
	for (int i = 0; i < n; i++) {
		// Enable event i in state 1, set to match on match register i
		SCT_EVn_STATE(i) = SCT_EVn_STATE_STATEMSK1(1);
		SCT_EVn_CTRL(i) = SCT_EVn_CTRL_COMBMODE_MATCH | SCT_EVn_CTRL_MATCHSEL(i);

		// Calculate the counter value to match on
		counter += times[i].dwell;

		// Wrapping from SCT_LIMIT seems to add an extra cycle,
		// so we reduce the counter value for the first event.
		if (i == 0)
			counter -= 1;

		SCT_MATCHn(i) = counter;
		SCT_MATCHRELn(i) = counter;

		// The match event selects the *next* port, so retreive that here.
		int port;
		if (i == n - 1) {
			port = times[0].port;
		} else {
			port = times[i + 1].port;
		}

		int bit0 = (port >> 0) & 1;
		int bit1 = (port >> 1) & 1;
		int bit2 = (port >> 2) & 1;

		// Find bits to set/clear on event i
		bit0_set |= SCT_OUTn_SETm(bit0, i);
		bit0_clr |= SCT_OUTn_CLRm(~bit0, i);

		bit1_set |= SCT_OUTn_SETm(bit1, i);
		bit1_clr |= SCT_OUTn_CLRm(~bit1, i);

		// (U1CTRL is inverted)
		bit2_set |= SCT_OUTn_SETm(~bit2, i);
		bit2_clr |= SCT_OUTn_CLRm(bit2, i);
	}

	// Apply event set/clear mappings
	U2CTRL0_SET = bit0_set;
	U2CTRL0_CLR = bit0_clr;
	U3CTRL0_SET = bit0_set;
	U3CTRL0_CLR = bit0_clr;
	U2CTRL1_SET = bit1_set;
	U2CTRL1_CLR = bit1_clr;
	U3CTRL1_SET = bit1_set;
	U3CTRL1_CLR = bit1_clr;
	U1CTRL_SET = bit2_set;
	U1CTRL_CLR = bit2_clr;

	// Set output pins to select the first port in the list
	default_output = operacake_sctimer_port_to_output(times[0].port);
	SCT_OUTPUT = default_output;

	// Reset counter on final event
	SCT_LIMIT = (1 << (n - 1));

	SCT_CTRL &= ~SCT_CTRL_HALT_L(1);
}

void operacake_sctimer_stop()
{
	// Halt timer
	SCT_CTRL |= SCT_CTRL_HALT_L(1);
}

/**
 * This will reset the state of the SCTimer, but retains its configuration.
 * This reset will return the selected antenna to 1 and samesided. This is
 * called by set_transceiver_mode so the HackRF starts capturing with the
 * same antenna selected each time.
 */
void operacake_sctimer_reset_state()
{
	SCT_CTRL |= SCT_CTRL_HALT_L(1);

	// Clear the counter value
	SCT_CTRL |= SCT_CTRL_CLRCTR_L(1);

	// Reset to select the first port
	SCT_OUTPUT = default_output;

	SCT_CTRL &= ~SCT_CTRL_HALT_L(1);
}
