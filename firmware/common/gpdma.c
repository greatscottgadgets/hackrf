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
