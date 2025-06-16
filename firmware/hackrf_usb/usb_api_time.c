/*
 * Copyright 2025 Fabrizio Pollastri <mxgbot@gmail.com>
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


#include <hackrf_core.h>
#include <usb_queue.h>
//#include <max283x.h>
//#include <rffc5071.h>

#include <math.h>
#include <stddef.h>
//#include <stdint.h>

//#include "gpdma.h"
#include <libopencm3/lpc43xx/timer.h>
//#include <libopencm3/lpc43xx/scu.h>
//#include <libopencm3/lpc43xx/gima.h>
#include <libopencm3/lpc43xx/creg.h>
#include <libopencm3/lpc43xx/m4/nvic.h>
#include <libopencm3/lpc43xx/wwdt.h>
#include <../common/hackrf_core.h>
#include "hackrf_ui.h"
#include "platform_detect.h"
#include "si5351c.h"

#include "usb_api_time.h"


#define PPS1_CLK_INIT_DIVISOR 204000000 - 1
#define PPS1_WIDTH 4000		// 20 us 1pps pulse width @200 MHz cpu clock
#define TRIG_INIT_DELAY 40000000	// 200 ms trigger delay @200 MHz cpu clock 


int64_t seconds;
int64_t new_seconds;
uint32_t new_divisor;
uint32_t one_pps_divisor;
uint32_t current_divisor;
uint32_t new_trig_delay;
uint32_t current_trig_delay;


/**** time timer functions ****/

void time_timer_init(void)
{
	/* no prescaler */
	timer_set_prescaler(TIMER3, 0);

	/* count cpu clock cycles */
	timer_set_mode(TIMER3, TIMER_CTCR_MODE_TIMER);

	/* reset counter and do pps interrupt when match register 0 (R0) */
	TIMER3_MCR = TIMER_MCR_MR0R | TIMER_MCR_MR0I;

	/* 1pps leading edge: at next R0 match, set to 1 the external output. */
	TIMER3_EMR = (TIMER_EMR_EMC_SET << TIMER_EMR_EMC0_SHIFT);

	/* set proper divisor (1Hz) to match R0 */
	TIMER3_MR0 = PPS1_CLK_INIT_DIVISOR;
	current_divisor = PPS1_CLK_INIT_DIVISOR;

	/* sampling trigger leading edge: at next R1 match, set to 1 the
	 * extenal output */
	TIMER3_EMR |= (TIMER_EMR_EMC_SET << TIMER_EMR_EMC1_SHIFT);

	/* set init trigger delay to match R1 */
	TIMER3_MR1 = TRIG_INIT_DELAY;
	current_trig_delay = TRIG_INIT_DELAY;

	/* prevent TIMER3_MR3 from interfering with SCT */
	CREG_CREG6 |= CREG_CREG6_CTOUTCTRL;

	/* clear second counter */
	seconds = 0;

	/* clear new values variables */
	new_divisor = 0;
	one_pps_divisor = 0;
	new_seconds = 0;
	new_trig_delay = 0;

	/* enable 1pps interrupt */
    nvic_enable_irq(NVIC_TIMER3_IRQ);

	/* start counting */
	timer_reset(TIMER3);
	timer_enable_counter(TIMER3);
}


void timer3_isr() {

	/* clear pending interrupt */
	TIMER3_IR |= TIMER_IR_MR3INT;

	/* if start of 1pps pulse: 1pps is set (leading edge) and match
	 * R0 is set to count until to next 1pps trailing edge.  */
	if (TIMER3_MR0 != PPS1_WIDTH) {

		/* second counter */
		seconds++;

		/* if requested, update seconds in sync with the new second. */
		if (new_seconds) {
			seconds = new_seconds;
			new_seconds = 0;
		}

		/* set R0 match value to keep 1pps high for the given time */
		TIMER3_MR0 = PPS1_WIDTH;

		/* 1pps trailing edge: at next R0 match, clear the external output. */
		TIMER3_EMR &= ~(0x3 << TIMER_EMR_EMC0_SHIFT);
		TIMER3_EMR |= (TIMER_EMR_EMC_CLEAR << TIMER_EMR_EMC0_SHIFT);

		/* set R1 match value to keep trigger high until the end of 1pps
		 * pulse */
		TIMER3_MR1 = PPS1_WIDTH;

		/* trigger trailing edge: at next R1 match, clear the external output. */
		TIMER3_EMR &= ~(0x3 << TIMER_EMR_EMC1_SHIFT);
		TIMER3_EMR |= (TIMER_EMR_EMC_CLEAR << TIMER_EMR_EMC1_SHIFT);

		/* do not reset counter at next R0 match: count must continue
		 * for one second period. */
		TIMER3_MCR &= ~TIMER_MCR_MR0R;

	}

	/* else is the end of 1pps pulse: match R0 is set to
	 * count until the next 1pps leading edge. */
	else {

		/* if requested, set new current divisor. */
		if (new_divisor) {
			current_divisor = new_divisor;
			new_divisor = 0;
		}

		/* if requested, set one pps divisor for one pps cycle then restore
	 	* current divisor. */
		if (one_pps_divisor) {
			TIMER3_MR0 = one_pps_divisor;
			one_pps_divisor = 0;
		}
		/* otherwise, restore current divisor. */
		else
			TIMER3_MR0 = current_divisor;

		/* 1pps leading edge: at next R0 match (start of second), set to 1
		 * the external output. */
		TIMER3_EMR &= ~(0x3 << TIMER_EMR_EMC0_SHIFT);
		TIMER3_EMR |= (TIMER_EMR_EMC_SET << TIMER_EMR_EMC0_SHIFT);

		/* if requested, update new trig delay. */
		if (new_trig_delay) {
			current_trig_delay = new_trig_delay;
			new_trig_delay = 0;
		}

		/* restore current trig delay */
		TIMER3_MR1 = current_trig_delay;

		/* trigger leading edge: at next R1 match, set to 1 the external
		 * output. */
		TIMER3_EMR &= ~(0x3 << TIMER_EMR_EMC1_SHIFT);
		TIMER3_EMR |= (TIMER_EMR_EMC_SET << TIMER_EMR_EMC1_SHIFT);

		/* reset counter at next R0 match: start counting for the
		 * next second period */
		TIMER3_MCR |= TIMER_MCR_MR0R;

	}

}


/**** API functions ****/


void si5351c_set_freq(si5351c_driver_t* const drv,uint8_t msn,double vco,
         double freq,uint8_t rdiv) {

	double frac, a, b, c;
	uint32_t p1, p2, p3;

	// fractional and integer part of pll divisor
	if (freq < vco)
		frac = modf(vco / freq, &a);
	else
		frac = modf(freq / vco, &a);

	// fractional expressed as c parts
	c = 1024 * 1024 - 1;
	b = round(frac * c);

	// convert a,b,c parms to p1,p2,p3 si5351c parms.
	p1 = 128 * a + floor(128. * b / c) - 512;
	p2 = 128 * b - c * floor(128. * b / c);
	p3 = c;

	// set multisynth frequency in given section
	si5351c_configure_multisynth(drv,msn,p1,p2,p3,rdiv);

}


/**** USB API ****/

usb_request_status_t usb_vendor_request_time_set_divisor_next_pps(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
        static uint32_t divisor = 0;
        if (stage == USB_TRANSFER_STAGE_SETUP) {
                usb_transfer_schedule_block(
                        endpoint->out,
                        (uint8_t*)&divisor,
                        4,
                        NULL,
                        NULL);
                return USB_REQUEST_STATUS_OK;
		} else if (stage == USB_TRANSFER_STAGE_DATA) {

						new_divisor = divisor;

                        usb_transfer_schedule_ack(endpoint->in);
                        return USB_REQUEST_STATUS_OK;
        } else {
                return USB_REQUEST_STATUS_OK;
        }
}


usb_request_status_t usb_vendor_request_time_set_divisor_one_pps(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
        static uint32_t divisor = 0;
        if (stage == USB_TRANSFER_STAGE_SETUP) {
                usb_transfer_schedule_block(
                        endpoint->out,
                        (uint8_t*)&divisor,
                        4,
                        NULL,
                        NULL);
                return USB_REQUEST_STATUS_OK;
        } else if (stage == USB_TRANSFER_STAGE_DATA) {

						one_pps_divisor = divisor;

                        usb_transfer_schedule_ack(endpoint->in);
                        return USB_REQUEST_STATUS_OK;
        } else {
                return USB_REQUEST_STATUS_OK;
        }
}


usb_request_status_t usb_vendor_request_time_set_trig_delay_next_pps(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
        static uint32_t trig_delay = 0;
        if (stage == USB_TRANSFER_STAGE_SETUP) {
                usb_transfer_schedule_block(
                        endpoint->out,
                        (uint8_t*)&trig_delay,
                        4,
                        NULL,
                        NULL);
                return USB_REQUEST_STATUS_OK;
        } else if (stage == USB_TRANSFER_STAGE_DATA) {

						new_trig_delay = trig_delay;

                        usb_transfer_schedule_ack(endpoint->in);
                        return USB_REQUEST_STATUS_OK;
        } else {
                return USB_REQUEST_STATUS_OK;
        }
}


usb_request_status_t usb_vendor_request_time_get_seconds_now(
        usb_endpoint_t* const endpoint,
        const usb_transfer_stage_t stage)
{
	int64_t secs;
        if (stage == USB_TRANSFER_STAGE_SETUP) {

                secs = seconds;

                usb_transfer_schedule_block(
                        endpoint->in,
                        (uint8_t*)&secs,
                        8,
                        NULL,
                        NULL);
                usb_transfer_schedule_ack(endpoint->out);
        }
        return USB_REQUEST_STATUS_OK;
}


usb_request_status_t usb_vendor_request_time_set_seconds_now(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	static int64_t secs = 0;
        if (stage == USB_TRANSFER_STAGE_SETUP) {
                usb_transfer_schedule_block(
                        endpoint->out,
                        (uint8_t*)&secs,
                        8,
                        NULL,
                        NULL);
                return USB_REQUEST_STATUS_OK;
        } else if (stage == USB_TRANSFER_STAGE_DATA) {

						seconds = secs;

                        usb_transfer_schedule_ack(endpoint->in);
                        return USB_REQUEST_STATUS_OK;
        } else {
                return USB_REQUEST_STATUS_OK;
        }
}


usb_request_status_t usb_vendor_request_time_set_seconds_next_pps(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	static int64_t secs = 0;
	if (stage == USB_TRANSFER_STAGE_SETUP) {
                usb_transfer_schedule_block(
                        endpoint->out,
                        (uint8_t*)&secs,
                        8,
                        NULL,
                        NULL);
                return USB_REQUEST_STATUS_OK;
        } else if (stage == USB_TRANSFER_STAGE_DATA) {

						new_seconds = secs;

                        usb_transfer_schedule_ack(endpoint->in);
                        return USB_REQUEST_STATUS_OK;
        } else {
                return USB_REQUEST_STATUS_OK;
        }
}


usb_request_status_t usb_vendor_request_time_get_ticks_now(
        usb_endpoint_t* const endpoint,
        const usb_transfer_stage_t stage)
{
        if (stage == USB_TRANSFER_STAGE_SETUP) {

                uint32_t ticks = timer_get_counter(TIMER3);

                usb_transfer_schedule_block(
                        endpoint->in,
                        (uint8_t*)&ticks,
                        4,
                        NULL,
                        NULL);
                usb_transfer_schedule_ack(endpoint->out);
        }
        return USB_REQUEST_STATUS_OK;
}


usb_request_status_t usb_vendor_request_time_set_ticks_now(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	static uint32_t tks = 0;
        if (stage == USB_TRANSFER_STAGE_SETUP) {
                usb_transfer_schedule_block(
                        endpoint->out,
                        (uint8_t*)&tks,
                        4,
                        NULL,
                        NULL);
                return USB_REQUEST_STATUS_OK;
        } else if (stage == USB_TRANSFER_STAGE_DATA) {

						timer_disable_counter(TIMER3);
						timer_set_counter(TIMER3,tks);
						timer_enable_counter(TIMER3);

                        usb_transfer_schedule_ack(endpoint->in);
                        return USB_REQUEST_STATUS_OK;
        } else {
                return USB_REQUEST_STATUS_OK;
        }
}


usb_request_status_t usb_vendor_request_time_set_clk_freq(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	static double freq;
	static bool alternate = false;

	if (stage == USB_TRANSFER_STAGE_SETUP) {
		usb_transfer_schedule_block(
			endpoint->out,
			(uint8_t*)&freq,
			8,
			NULL,
			NULL);
	} else {
		if (stage == USB_TRANSFER_STAGE_DATA) {

			if (detected_platform() == BOARD_ID_HACKRF1_R9) {
				if (alternate) {
					si5351c_set_freq(&clock_gen,1,800e6,freq * 2,0);
					si5351c_set_freq(&clock_gen,2,800e6,freq,0);
				}
				else {
					si5351c_set_freq(&clock_gen,2,800e6,freq,0);
					si5351c_set_freq(&clock_gen,1,800e6,freq * 2,0);
				}
				alternate = !alternate;
			}
			else {
				si5351c_set_freq(&clock_gen,0,800e6,freq * 2,1);
			}
			usb_transfer_schedule_ack(endpoint->in);
		}
	}
	return USB_REQUEST_STATUS_OK;
}


usb_request_status_t usb_vendor_request_time_set_mcu_clk_sync(
        usb_endpoint_t* const endpoint,
        const usb_transfer_stage_t stage)
{
        if (stage == USB_TRANSFER_STAGE_SETUP) {

				// change cgu clkin to 10MHz from clock generator (GP_CLKIN)
				// WARNING: clock gen out #2 for r9 boards or out #3 for not
				// r9 must be already set to give 10MHz.
				if (endpoint->setup.value) {

					// enable mcu clock: output clock to mcu
					si5351c_mcu_clk_enable(true);

					// configure si5351c synthesizer: force int more, remove
					// self channel source whenever possible.
					si5351c_mcu_clk_sync(&clock_gen,true);

					// set mcu clk pll1 to nominal maximum speed 200 MHz
					cpu_clock_pll1_max_speed(CGU_SRC_GP_CLKIN,20);

				}
				// return cgu clkin to 12MHz XTAL
				else {

					// set mcu clk pll1 to nominal maximum speed 204 MHz
					cpu_clock_pll1_max_speed(CGU_SRC_XTAL,17);

					// restore the configuration of the si5351c synthesizer
					si5351c_mcu_clk_sync(&clock_gen,false);

					// disable mcu clock: no output clock to mcu
					si5351c_mcu_clk_enable(false);

				}

                usb_transfer_schedule_ack(endpoint->in);
        }
        return USB_REQUEST_STATUS_OK;
}

/**** end ****/
