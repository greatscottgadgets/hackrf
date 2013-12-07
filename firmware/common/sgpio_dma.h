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

#ifndef __SGPIO_DMA_H__
#define __SGPIO_DMA_H__

#include <stddef.h>

void sgpio_dma_init(void* const buffer, const size_t byte_count);
void sgpio_dma_rx_start();
void sgpio_dma_tx_start();
void sgpio_dma_irq_tc_acknowledge();
void sgpio_dma_stop();

size_t sgpio_dma_current_transfer_index();

#endif/*__SGPIO_DMA_H__*/
