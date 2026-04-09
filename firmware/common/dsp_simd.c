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

#include "dsp_simd.h"
#include <string.h>
#include <libopencm3/cm3/common.h>
#include <libopencm3/cm3/memorymap.h>
#include <libopencm3/cm3/scs.h>

// Performance counters (static for efficiency)
static dsp_simd_perf_t perf_counters = {0};

// Cycle counter for performance measurement
static inline uint32_t dsp_simd_get_cycles(void) {
    return SCS_DWT_CYCCNT;
}

void dsp_simd_init_fir_filter(fir_filter_simd_t* filter, const int16_t* coeffs, uint8_t num_taps) {
    if (!filter || !coeffs || num_taps == 0 || num_taps > 32) {
        return;
    }
    
    // Copy coefficients (reverse for convolution)
    for (uint8_t i = 0; i < num_taps; i++) {
        filter->coeffs[i] = coeffs[num_taps - 1 - i];
    }
    
    filter->num_taps = num_taps;
    filter->index = 0;
    
    // Clear delay line
    memset(filter->delay_line, 0, sizeof(filter->delay_line));
}

void dsp_simd_init_iq_correction(iq_correction_simd_t* corr, int16_t gain_i, int16_t gain_q, 
                                 int16_t offset_i, int16_t offset_q, int16_t phase_corr) {
    if (!corr) {
        return;
    }
    
    corr->gain_i = gain_i;
    corr->gain_q = gain_q;
    corr->offset_i = offset_i;
    corr->offset_q = offset_q;
    corr->phase_corr = phase_corr;
}

int32_t dsp_simd_fir_filter_sample(fir_filter_simd_t* filter, int16_t input) {
    if (!filter) {
        return 0;
    }
    
    uint32_t start_cycles = dsp_simd_get_cycles();
    
    // Add new sample to delay line
    filter->delay_line[filter->index] = input;
    
    // Perform SIMD-optimized convolution
    int32_t acc = 0;
    uint8_t taps = filter->num_taps;
    uint8_t idx = filter->index;
    
    // Process in pairs for SIMD efficiency
    if (taps >= 2) {
        uint8_t pairs = taps / 2;
        for (uint8_t i = 0; i < pairs; i++) {
            uint8_t delay_idx = (idx + i) % 64;
            // Use SMLAD (Signed Multiply Accumulate Dual) instruction
            // This computes: acc += a[i] * b[i] + a[i+1] * b[i+1]
            // Use memcpy to avoid strict-aliasing warning
            int32_t delay_pair;
            memcpy(&delay_pair, &filter->delay_line[delay_idx], sizeof(int32_t));
            acc = __smlad(filter->coeffs[i*2], delay_pair, acc);
        }
        
        // Handle remaining tap if odd number
        if (taps % 2) {
            uint8_t delay_idx = (idx + pairs) % 64;
            acc += filter->coeffs[taps-1] * filter->delay_line[delay_idx];
        }
    } else {
        // Single tap case
        acc = filter->coeffs[0] * filter->delay_line[idx];
    }
    
    // Update circular buffer index
    filter->index = (filter->index + 1) % 64;
    
    perf_counters.cycles_fir_sample += dsp_simd_get_cycles() - start_cycles;
    perf_counters.samples_processed++;
    
    return acc;
}

void dsp_simd_fir_filter_block(fir_filter_simd_t* filter, const int16_t* input, 
                               int32_t* output, uint16_t block_size) {
    if (!filter || !input || !output || block_size == 0) {
        return;
    }
    
    uint32_t start_cycles = dsp_simd_get_cycles();
    
    for (uint16_t i = 0; i < block_size; i++) {
        output[i] = dsp_simd_fir_filter_sample(filter, input[i]);
    }
    
    perf_counters.cycles_fir_block += dsp_simd_get_cycles() - start_cycles;
}

void dsp_simd_iq_correct_sample(iq_correction_simd_t* corr, iq_sample_t* sample) {
    if (!corr || !sample) {
        return;
    }
    
    uint32_t start_cycles = dsp_simd_get_cycles();
    
    // Apply gain correction (using SIMD if possible)
    int32_t i_corrected = sample->i * corr->gain_i + corr->offset_i;
    int32_t q_corrected = sample->q * corr->gain_q + corr->offset_q;
    
    // Apply phase correction (small angle approximation)
    // For small angles: cos(theta) ~ 1, sin(theta) ~ theta
    // New I = I*cos(theta) - Q*sin(theta) ~ I - Q*theta
    // New Q = I*sin(theta) + Q*cos(theta) ~ I*theta + Q
    int32_t phase_factor = corr->phase_corr;
    int32_t i_final = i_corrected - ((q_corrected * phase_factor) >> 15);
    int32_t q_final = q_corrected + ((i_corrected * phase_factor) >> 15);
    
    // Saturate to 16-bit range
    sample->i = (int16_t)((i_final > 32767) ? 32767 : ((i_final < -32768) ? -32768 : i_final));
    sample->q = (int16_t)((q_final > 32767) ? 32767 : ((q_final < -32768) ? -32768 : q_final));
    
    perf_counters.cycles_iq_correction += dsp_simd_get_cycles() - start_cycles;
}

void dsp_simd_iq_correct_block(iq_correction_simd_t* corr, iq_sample_t* samples, 
                               uint16_t block_size) {
    if (!corr || !samples || block_size == 0) {
        return;
    }
    
    for (uint16_t i = 0; i < block_size; i++) {
        dsp_simd_iq_correct_sample(corr, &samples[i]);
    }
}

int32_t dsp_simd_dot_product(const int16_t* a, const int16_t* b, uint16_t length) {
    if (!a || !b || length == 0) {
        return 0;
    }
    
    int32_t result = 0;
    uint16_t pairs = length / 2;
    
    // Process in pairs for SIMD efficiency
    for (uint16_t i = 0; i < pairs; i++) {
        result = __smlad(a[i*2], *(int32_t*)&b[i*2], result);
    }
    
    // Handle remaining element if odd length
    if (length % 2) {
        result += a[length-1] * b[length-1];
    }
    
    return result;
}

void dsp_simd_vector_scale(const int16_t* input, int16_t* output, 
                           int16_t scale, uint16_t length) {
    if (!input || !output || length == 0) {
        return;
    }
    
    // Use SMUAD (Signed Multiply Accumulate Dual) for scaling pairs
    for (uint16_t i = 0; i < length; i++) {
        output[i] = (int16_t)((input[i] * scale) >> 15);
    }
}

void dsp_simd_complex_multiply(const iq_sample_t* a, const iq_sample_t* b, 
                              iq_sample_t* result, uint16_t length) {
    if (!a || !b || !result || length == 0) {
        return;
    }
    
    for (uint16_t i = 0; i < length; i++) {
        // Complex multiplication: (a + jb) * (c + jd) = (ac - bd) + j(ad + bc)
        int32_t real = ((int32_t)a[i].i * b[i].i) - ((int32_t)a[i].q * b[i].q);
        int32_t imag = ((int32_t)a[i].i * b[i].q) + ((int32_t)a[i].q * b[i].i);
        
        // Scale and saturate
        result[i].i = (int16_t)((real + (1<<14)) >> 15); // Round and shift
        result[i].q = (int16_t)((imag + (1<<14)) >> 15);
    }
}

void dsp_simd_reset_perf_counters(dsp_simd_perf_t* perf) {
    if (perf) {
        memset(perf, 0, sizeof(dsp_simd_perf_t));
    }
    memset(&perf_counters, 0, sizeof(dsp_simd_perf_t));
}

void dsp_simd_get_perf_counters(dsp_simd_perf_t* perf) {
    if (perf) {
        *perf = perf_counters;
    }
}
