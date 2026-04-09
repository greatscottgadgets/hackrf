/*
 * Copyright 2026 Great Scott Gadgets <info@greatscottgadgets.com>
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

#ifndef STREAMING_SIMD_H
#define STREAMING_SIMD_H

#include <stdint.h>
#include <stdbool.h>
#include "dsp_simd.h"

// SIMD-optimized streaming interface for HackRF baseband processing
// Provides high-performance I/Q sample processing using ARM SIMD instructions

// Function prototypes for SIMD-optimized streaming
void streaming_simd_init(void);
void streaming_simd_enable_filters(bool enable);
void streaming_simd_enable_iq_correction(bool enable);

// Filter configuration
void streaming_simd_set_tx_filter(const int16_t* coeffs, uint8_t num_taps);
void streaming_simd_set_rx_filter(const int16_t* coeffs, uint8_t num_taps);

// I/Q correction configuration
void streaming_simd_set_iq_correction(int16_t gain_i, int16_t gain_q, 
                                     int16_t offset_i, int16_t offset_q, 
                                     int16_t phase_corr);

// Sample processing functions
void streaming_simd_process_tx_samples(const int16_t* input, int16_t* output, 
                                       uint16_t block_size);
void streaming_simd_process_rx_samples(const int16_t* input, int16_t* output, 
                                       uint16_t block_size);

// Performance monitoring
void streaming_simd_get_performance(dsp_simd_perf_t* perf);
void streaming_simd_reset_performance(void);

#endif // STREAMING_SIMD_H
