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
 
#include <sgpio_dma.h>

#include <libopencm3/lpc43xx/creg.h>
#include <libopencm3/lpc43xx/gpdma.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/sgpio.h>

#include <sgpio.h>
#include <gpdma.h>

void sgpio_dma_configure_lli(
	gpdma_lli_t* const lli,
	const size_t lli_count,
	const bool direction_transmit,
	void* const buffer,
	const size_t transfer_bytes
) {
	const size_t bytes_per_word = 4;
	const size_t transfer_words = (transfer_bytes + bytes_per_word - 1) / bytes_per_word;

	gpdma_lli_create_loop(lli, lli_count);

	for(size_t i=0; i<lli_count; i++) {
		void* const peripheral_address = (void*)&SGPIO_REG_SS(0);
		void* const memory_address = buffer + (transfer_words * bytes_per_word * i);
		
		const uint_fast8_t source_master = direction_transmit ? 1 : 0;
		const uint_fast8_t destination_master = direction_transmit ? 0 : 1;
		const uint_fast8_t lli_fetch_master = direction_transmit ? 0 : 1;

		lli[i].csrcaddr = direction_transmit ? memory_address : peripheral_address;
		lli[i].cdestaddr = direction_transmit ? peripheral_address : memory_address;
		lli[i].clli = (lli[i].clli & ~GPDMA_CLLI_LM_MASK) | GPDMA_CLLI_LM(lli_fetch_master);
		lli[i].ccontrol =
			GPDMA_CCONTROL_TRANSFERSIZE(transfer_words) |
			GPDMA_CCONTROL_SBSIZE(0) |
			GPDMA_CCONTROL_DBSIZE(0) |
			GPDMA_CCONTROL_SWIDTH(2) |
			GPDMA_CCONTROL_DWIDTH(2) |
			GPDMA_CCONTROL_S(source_master) |
			GPDMA_CCONTROL_D(destination_master) |
			GPDMA_CCONTROL_SI(direction_transmit ? 1 : 0) |
			GPDMA_CCONTROL_DI(direction_transmit ? 0 : 1) |
			GPDMA_CCONTROL_PROT1(0) |
			GPDMA_CCONTROL_PROT2(0) |
			GPDMA_CCONTROL_PROT3(0) |
			GPDMA_CCONTROL_I(0)
			;
	}
}

static void sgpio_dma_enable(const uint_fast8_t channel, const gpdma_lli_t* const lli, const bool direction_transmit) {
	gpdma_channel_disable(channel);
	gpdma_channel_interrupt_tc_clear(channel);
	gpdma_channel_interrupt_error_clear(channel);

	GPDMA_CSRCADDR(channel) = (uint32_t)lli->csrcaddr;
	GPDMA_CDESTADDR(channel) = (uint32_t)lli->cdestaddr;
	GPDMA_CLLI(channel) = (uint32_t)lli->clli;
	GPDMA_CCONTROL(channel) = lli->ccontrol;

	/* 1: Memory -> Peripheral
	 * 2: Peripheral -> Memory */
	const uint_fast8_t flowcntrl = direction_transmit ? 1 : 2;

	GPDMA_CCONFIG(channel) =
		GPDMA_CCONFIG_E(0) |
		GPDMA_CCONFIG_SRCPERIPHERAL(0) |
		GPDMA_CCONFIG_DESTPERIPHERAL(0) |
		GPDMA_CCONFIG_FLOWCNTRL(flowcntrl) |
		GPDMA_CCONFIG_IE(1) |
		GPDMA_CCONFIG_ITC(1) |
		GPDMA_CCONFIG_L(0) |
		GPDMA_CCONFIG_H(0)
		;

	gpdma_channel_enable(channel);
}

void sgpio_dma_init() {
	/* DMA peripheral/source 0, option 2 (SGPIO14) -- BREQ */
	CREG_DMAMUX &= ~(CREG_DMAMUX_DMAMUXPER0_MASK);
	CREG_DMAMUX |= CREG_DMAMUX_DMAMUXPER0(0x2);

	// Disable sync, maybe it is causing max speed (10MT/sec) glitches?
	//GPDMA_DMACSYNC = (1 << 0);
	//GPDMA_SYNC = GPDMA_SYNC_DMACSYNC(0xFFFF); // TODO: Don't do this, I'm just going nuts here.

	gpdma_controller_enable();
}

static const uint_fast8_t dma_channel_sgpio = 0;

void sgpio_dma_rx_start(const gpdma_lli_t* const start_lli) {
	sgpio_dma_enable(dma_channel_sgpio, start_lli, false);
}

void sgpio_dma_tx_start(const gpdma_lli_t* const start_lli) {
	sgpio_dma_enable(dma_channel_sgpio, start_lli, true);
}

void sgpio_dma_irq_tc_acknowledge() {
	gpdma_channel_interrupt_tc_clear(dma_channel_sgpio);
}

void sgpio_dma_stop() {
	gpdma_channel_disable(dma_channel_sgpio);
}

size_t sgpio_dma_current_transfer_index(
	const gpdma_lli_t* const lli,
	const size_t lli_count
) {
	const uint32_t next_lli = GPDMA_CLLI(dma_channel_sgpio);
	for(size_t i=0; i<lli_count; i++) {
		if( lli[i].clli == next_lli ) {
			return i;
		}
	}
	return 0;
}
