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

#ifndef DSP_SIMD_H
#define DSP_SIMD_H

#include <stdint.h>
#include <stdbool.h>
#include <arm_acle.h>

// SIMD-optimized DSP functions for Cortex-M4F
// Uses ARM SIMD instructions: SMLAD, SMUAD, SMLSD, SMUSD

// Complex number structure for I/Q samples
typedef struct {
    int16_t i;  // In-phase component
    int16_t q;  // Quadrature component
} iq_sample_t;

// SIMD-optimized FIR filter state
typedef struct {
    int16_t coeffs[32];    // Filter coefficients (max 32 taps)
    int16_t delay_line[64]; // Delay line for circular buffer
    uint8_t num_taps;      // Number of filter taps
    uint8_t index;         // Current index in delay line
} fir_filter_simd_t;

// SIMD-optimized I/Q correction state
typedef struct {
    int16_t gain_i;        // I gain correction
    int16_t gain_q;        // Q gain correction
    int16_t offset_i;      // I offset correction
    int16_t offset_q;      // Q offset correction
    int16_t phase_corr;    // Phase correction (small angle approximation)
} iq_correction_simd_t;

// Function prototypes
void dsp_simd_init_fir_filter(fir_filter_simd_t* filter, const int16_t* coeffs, uint8_t num_taps);
void dsp_simd_init_iq_correction(iq_correction_simd_t* corr, int16_t gain_i, int16_t gain_q, 
                                 int16_t offset_i, int16_t offset_q, int16_t phase_corr);

// SIMD-optimized DSP processing functions
int32_t dsp_simd_fir_filter_sample(fir_filter_simd_t* filter, int16_t input);
void dsp_simd_fir_filter_block(fir_filter_simd_t* filter, const int16_t* input, 
                               int32_t* output, uint16_t block_size);
void dsp_simd_iq_correct_sample(iq_correction_simd_t* corr, iq_sample_t* sample);
void dsp_simd_iq_correct_block(iq_correction_simd_t* corr, iq_sample_t* samples, 
                               uint16_t block_size);

// SIMD-optimized utility functions
int32_t dsp_simd_dot_product(const int16_t* a, const int16_t* b, uint16_t length);
void dsp_simd_vector_scale(const int16_t* input, int16_t* output, 
                           int16_t scale, uint16_t length);
void dsp_simd_complex_multiply(const iq_sample_t* a, const iq_sample_t* b, 
                              iq_sample_t* result, uint16_t length);

// Performance monitoring
typedef struct {
    uint32_t cycles_fir_sample;
    uint32_t cycles_fir_block;
    uint32_t cycles_iq_correction;
    uint32_t samples_processed;
} dsp_simd_perf_t;

void dsp_simd_reset_perf_counters(dsp_simd_perf_t* perf);
void dsp_simd_get_perf_counters(dsp_simd_perf_t* perf);

#endif // DSP_SIMD_H
