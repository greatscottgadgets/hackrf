/*
 * Copyright 2022 Great Scott Gadgets
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

#include "gpdma.h"
#include <libopencm3/lpc43xx/timer.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/gima.h>
#include <libopencm3/lpc43xx/creg.h>
#include <stdint.h>

#define CLOCK_CYCLES_1_MS     (204000)
#define MEASUREMENT_WINDOW_MS (50)
#define MEASUREMENT_CYCLES    (CLOCK_CYCLES_1_MS * MEASUREMENT_WINDOW_MS)

/* DMA linked list item */
typedef struct {
	uint32_t src;
	uint32_t dest;
	uint32_t next_lli;
	uint32_t control;
} dma_lli;

/* timer control register configuration sequence */
typedef struct {
	uint32_t first_tcr;
	uint32_t second_tcr;
} tcr_sequence;

dma_lli timer_dma_lli;
tcr_sequence reset;

void clkin_detect_init(void)
{
	/* Timer1 triggers periodic measurement */
	timer_set_prescaler(TIMER1, 0);
	timer_set_mode(TIMER1, TIMER_CTCR_MODE_TIMER);
	TIMER1_MCR = TIMER_MCR_MR0R;
	TIMER1_EMR = (TIMER_EMR_EMC_SET << TIMER_EMR_EMC0_SHIFT) |
		(TIMER_EMR_EMC_TOGGLE << TIMER_EMR_EMC3_SHIFT);
	TIMER1_MR3 = MEASUREMENT_CYCLES;
	TIMER1_MR0 = MEASUREMENT_CYCLES;

	/* prevent TIMER1_MR3 from interfering with SCT */
	CREG_CREG6 |= CREG_CREG6_CTOUTCTRL;

	/* Timer2 counts CLKIN */
	timer_set_prescaler(TIMER2, 0);
	TIMER2_CCR = TIMER_CCR_CAP3RE;
	GIMA_CAP2_3_IN = 0x20; // T1_MAT3

	/* measure CLKIN_DETECT signal on P4_8, pin 15, CTIN_5 */
	TIMER2_CTCR = TIMER_CTCR_MODE_COUNTER_RISING | TIMER_CTCR_CINSEL_CAPN_2;
	scu_pinmux(P4_8, SCU_GPIO_PDN | SCU_CONF_FUNCTION1); // CTIN_5
	GIMA_CAP2_2_IN = 0x00;                               // CTIN_5

	reset.first_tcr = TIMER_TCR_CEN | TIMER_TCR_CRST;
	reset.second_tcr = TIMER_TCR_CEN;
	timer_dma_lli.src = (uint32_t) &(reset);
	timer_dma_lli.dest = (uint32_t) &(TIMER2_TCR);
	timer_dma_lli.next_lli = (uint32_t) &(timer_dma_lli);
	timer_dma_lli.control = GPDMA_CCONTROL_TRANSFERSIZE(2) |
		GPDMA_CCONTROL_SBSIZE(0)   // 1
		| GPDMA_CCONTROL_DBSIZE(0) // 1
		| GPDMA_CCONTROL_SWIDTH(2) // 32-bit word
		| GPDMA_CCONTROL_DWIDTH(2) // 32-bit word
		| GPDMA_CCONTROL_S(0)      // AHB Master 0
		| GPDMA_CCONTROL_D(1)      // AHB Master 1
		| GPDMA_CCONTROL_SI(1)     // increment source
		| GPDMA_CCONTROL_DI(0)     // do not increment destination
		| GPDMA_CCONTROL_PROT1(0)  // user mode
		| GPDMA_CCONTROL_PROT2(0)  // not bufferable
		| GPDMA_CCONTROL_PROT3(0)  // not cacheable
		| GPDMA_CCONTROL_I(0);     // interrupt disabled
	gpdma_controller_enable();
	GPDMA_C0SRCADDR = timer_dma_lli.src;
	GPDMA_C0DESTADDR = timer_dma_lli.dest;
	GPDMA_C0LLI = timer_dma_lli.next_lli;
	GPDMA_C0CONTROL = timer_dma_lli.control;
	GPDMA_C0CONFIG = GPDMA_CCONFIG_DESTPERIPHERAL(0x3) // T1_MAT0
		| GPDMA_CCONFIG_FLOWCNTRL(1)               // memory-to-peripheral
		| GPDMA_CCONFIG_H(0);                      // do not halt
	gpdma_channel_enable(0);

	/* start counting */
	timer_reset(TIMER2);
	timer_reset(TIMER1);
	timer_enable_counter(TIMER2);
	timer_enable_counter(TIMER1);
}

uint32_t clkin_frequency(void)
{
	return TIMER2_CR3 * (1000 / MEASUREMENT_WINDOW_MS);
};
