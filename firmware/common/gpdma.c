/*
 * Copyright 2013 Jared Boone <jared@sharebrained.com>
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

#include <gpdma.h>

#include <libopencm3/lpc43xx/gpdma.h>

void gpdma_controller_enable() {
	GPDMA_CONFIG |= GPDMA_CONFIG_E(1);
	while( (GPDMA_CONFIG & GPDMA_CONFIG_E_MASK) == 0 );
}

void gpdma_channel_enable(const uint_fast8_t channel) {
	GPDMA_CCONFIG(channel) |= GPDMA_CCONFIG_E(1);
}

void gpdma_channel_disable(const uint_fast8_t channel) {
	GPDMA_CCONFIG(channel) &= ~GPDMA_CCONFIG_E_MASK;
	while( (GPDMA_ENBLDCHNS & GPDMA_ENBLDCHNS_ENABLEDCHANNELS(1 << channel)) );
}

void gpdma_channel_interrupt_tc_clear(const uint_fast8_t channel) {
	GPDMA_INTTCCLEAR = GPDMA_INTTCCLEAR_INTTCCLEAR(1 << channel);
}

void gpdma_channel_interrupt_error_clear(const uint_fast8_t channel) {
	GPDMA_INTERRCLR = GPDMA_INTERRCLR_INTERRCLR(1 << channel);
}

void gpdma_lli_enable_interrupt(gpdma_lli_t* const lli) {
	lli->ccontrol |= GPDMA_CCONTROL_I(1);
}

void gpdma_lli_create_loop(gpdma_lli_t* const lli, const size_t lli_count) {
	for(size_t i=0; i<lli_count; i++) {
		gpdma_lli_t* const next_lli = &lli[(i + 1) % lli_count];
		lli[i].clli = (lli[i].clli & ~GPDMA_CLLI_LLI_MASK) | GPDMA_CLLI_LLI((uint32_t)next_lli >> 2);
	}
}

void gpdma_lli_create_oneshot(gpdma_lli_t* const lli, const size_t lli_count) {
	gpdma_lli_create_loop(lli, lli_count);
	lli[lli_count - 1].clli &= ~GPDMA_CLLI_LLI_MASK;
}
