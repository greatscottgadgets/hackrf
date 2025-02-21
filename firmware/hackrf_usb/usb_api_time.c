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


int64_t seconds;
int64_t new_seconds;
uint32_t new_divisor;
uint32_t one_pps_divisor;
uint32_t current_divisor;
uint32_t new_trig_delay;


/**** time timer functions ****/

void time_timer_init(void)
{
	/* no prescaler */
	timer_set_prescaler(TIMER3, 0);

	/* count cpu clock cycles */
	timer_set_mode(TIMER3, TIMER_CTCR_MODE_TIMER);

	/* reset counter and do pps interrupt when match register 0 */
	TIMER3_MCR = TIMER_MCR_MR0R | TIMER_MCR_MR0I;

	/* when match R0, set to 1 the external output: generate pps leading
	 * edge */
	TIMER3_EMR = (TIMER_EMR_EMC_SET << TIMER_EMR_EMC0_SHIFT);

	/* when match R1, set to 1 the sampling trigger */
	TIMER3_EMR |= (TIMER_EMR_EMC_SET << TIMER_EMR_EMC1_SHIFT);

	/* set proper divisor (1Hz) to match register 3 */
	TIMER3_MR0 = PPS1_CLK_INIT_DIVISOR;
	current_divisor = PPS1_CLK_INIT_DIVISOR;

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

	/* clear sampling tigger: P2_4, cpu pin 88, conn P28 pin 5
	 * trigger trailing edge*/
	TIMER3_EMR &= ~TIMER_EMR_EM1;

	/* clear pending interrupt */
	TIMER3_IR |= TIMER_IR_MR3INT;

	/* second counter */
	seconds++;

	/* if requested, update seconds in sync with the new second */
	if (new_seconds) {
		seconds = new_seconds;
		new_seconds = 0;
	}

	/* if requested, update new trig delay in sync with the new second */
	if (new_trig_delay) {
		TIMER3_MR1 = new_trig_delay;
		new_trig_delay = 0;
	}

	/* if requested, update new timer divisor in sync with the new
	 * second */
	if (new_divisor) {
		TIMER3_MR0 = new_divisor;
		current_divisor = new_divisor;
		new_divisor = 0;
	}

	/* if requested, set one pps divisor for one pps cycle then restore
	 * current divisor */
	if (one_pps_divisor) {
		TIMER3_MR0 = one_pps_divisor;
		one_pps_divisor = 0;
		new_divisor = current_divisor;
	}

	/* clear output pps: P2_3, cpu pin 87, conn P28 pin 6, pps trailing
	 * edge */
	TIMER3_EMR &= ~TIMER_EMR_EM0;

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







/**** end ****/
