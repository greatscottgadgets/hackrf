/*
 * Copyright 2013-2022 Great Scott Gadgets <info@greatscottgadgets.com>
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

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#include <libopencm3/lpc43xx/gpdma.h>

/* LPC43xx GPDMA (UM10503 §11.5.1): LLI address bits [1:0] must be zero,
 * meaning each LLI must be 4-byte aligned. Also verify size is a multiple
 * of 4 so that arrays of LLIs never produce an unaligned successor pointer. */
_Static_assert(
	_Alignof(gpdma_lli_t) >= 4,
	"gpdma_lli_t must be at least 4-byte aligned (LPC43xx GPDMA requirement)");
_Static_assert(
	sizeof(gpdma_lli_t) % 4 == 0,
	"gpdma_lli_t size must be a multiple of 4 bytes");

void gpdma_controller_enable(void);

void gpdma_channel_enable(const uint_fast8_t channel);
void gpdma_channel_disable(const uint_fast8_t channel);

void gpdma_channel_interrupt_tc_clear(const uint_fast8_t channel);
void gpdma_channel_interrupt_error_clear(const uint_fast8_t channel);

void gpdma_lli_enable_interrupt(gpdma_lli_t* const lli);

void gpdma_lli_create_loop(gpdma_lli_t* const lli, const size_t lli_count);
void gpdma_lli_create_oneshot(gpdma_lli_t* const lli, const size_t lli_count);
