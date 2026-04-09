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

#include <streaming_simd.h>
#include <dsp_simd.h>
#include <string.h>

// SIMD-optimized streaming context
typedef struct {
    fir_filter_simd_t tx_filter;
    fir_filter_simd_t rx_filter;
    iq_correction_simd_t iq_correction;
    dsp_simd_perf_t perf;
    bool filters_enabled;
    bool iq_correction_enabled;
} streaming_simd_ctx_t;

static streaming_simd_ctx_t simd_ctx = {0};

// Default filter coefficients for baseband processing
static const int16_t default_tx_coeffs[8] = {
    128, 64, 32, 16, 8, 4, 2, 1  // Simple low-pass filter
};

static const int16_t default_rx_coeffs[8] = {
    1, 2, 4, 8, 16, 32, 64, 128  // Simple high-pass filter
};

void streaming_simd_init(void) {
    // Initialize TX filter
    dsp_simd_init_fir_filter(&simd_ctx.tx_filter, default_tx_coeffs, 8);
    
    // Initialize RX filter
    dsp_simd_init_fir_filter(&simd_ctx.rx_filter, default_rx_coeffs, 8);
    
    // Initialize I/Q correction with default values (no correction)
    dsp_simd_init_iq_correction(&simd_ctx.iq_correction, 
                                4096, 4096,  // Unity gain (1.0 in Q15 format)
                                0, 0,        // No offset
                                0);          // No phase correction
    
    // Reset performance counters
    dsp_simd_reset_perf_counters(&simd_ctx.perf);
    
    // Enable processing by default
    simd_ctx.filters_enabled = true;
    simd_ctx.iq_correction_enabled = true;
}

void streaming_simd_enable_filters(bool enable) {
    simd_ctx.filters_enabled = enable;
}

void streaming_simd_enable_iq_correction(bool enable) {
    simd_ctx.iq_correction_enabled = enable;
}

void streaming_simd_set_tx_filter(const int16_t* coeffs, uint8_t num_taps) {
    if (coeffs && num_taps > 0 && num_taps <= 32) {
        dsp_simd_init_fir_filter(&simd_ctx.tx_filter, coeffs, num_taps);
    }
}

void streaming_simd_set_rx_filter(const int16_t* coeffs, uint8_t num_taps) {
    if (coeffs && num_taps > 0 && num_taps <= 32) {
        dsp_simd_init_fir_filter(&simd_ctx.rx_filter, coeffs, num_taps);
    }
}

void streaming_simd_set_iq_correction(int16_t gain_i, int16_t gain_q, 
                                     int16_t offset_i, int16_t offset_q, 
                                     int16_t phase_corr) {
    dsp_simd_init_iq_correction(&simd_ctx.iq_correction, 
                                gain_i, gain_q, offset_i, offset_q, phase_corr);
}

void streaming_simd_process_tx_samples(const int16_t* input, int16_t* output, 
                                       uint16_t block_size) {
    if (!input || !output || block_size == 0) {
        return;
    }
    
    // Apply I/Q correction first if enabled
    if (simd_ctx.iq_correction_enabled) {
        dsp_simd_iq_correct_block(&simd_ctx.iq_correction, 
                                  (iq_sample_t*)input, block_size/2);
    }
    
    // Apply TX filtering if enabled
    if (simd_ctx.filters_enabled) {
        // Process I and Q channels separately through FIR filter
        for (uint16_t i = 0; i < block_size; i += 2) {
            int32_t filtered_i = dsp_simd_fir_filter_sample(&simd_ctx.tx_filter, input[i]);
            int32_t filtered_q = dsp_simd_fir_filter_sample(&simd_ctx.tx_filter, input[i+1]);
            
            // Saturate and convert back to 16-bit
            output[i] = (int16_t)((filtered_i > 32767) ? 32767 : 
                                 ((filtered_i < -32768) ? -32768 : filtered_i));
            output[i+1] = (int16_t)((filtered_q > 32767) ? 32767 : 
                                   ((filtered_q < -32768) ? -32768 : filtered_q));
        }
    } else {
        // Just copy if filtering is disabled
        memcpy(output, input, block_size * sizeof(int16_t));
    }
}

void streaming_simd_process_rx_samples(const int16_t* input, int16_t* output, 
                                       uint16_t block_size) {
    if (!input || !output || block_size == 0) {
        return;
    }
    
    // Apply RX filtering if enabled
    if (simd_ctx.filters_enabled) {
        // Process I and Q channels separately through FIR filter
        for (uint16_t i = 0; i < block_size; i += 2) {
            int32_t filtered_i = dsp_simd_fir_filter_sample(&simd_ctx.rx_filter, input[i]);
            int32_t filtered_q = dsp_simd_fir_filter_sample(&simd_ctx.rx_filter, input[i+1]);
            
            // Saturate and convert back to 16-bit
            output[i] = (int16_t)((filtered_i > 32767) ? 32767 : 
                                 ((filtered_i < -32768) ? -32768 : filtered_i));
            output[i+1] = (int16_t)((filtered_q > 32767) ? 32767 : 
                                   ((filtered_q < -32768) ? -32768 : filtered_q));
        }
    } else {
        // Just copy if filtering is disabled
        memcpy(output, input, block_size * sizeof(int16_t));
    }
    
    // Apply I/Q correction last if enabled
    if (simd_ctx.iq_correction_enabled) {
        dsp_simd_iq_correct_block(&simd_ctx.iq_correction, 
                                  (iq_sample_t*)output, block_size/2);
    }
}

void streaming_simd_get_performance(dsp_simd_perf_t* perf) {
    if (perf) {
        dsp_simd_get_perf_counters(perf);
    }
}

void streaming_simd_reset_performance(void) {
    dsp_simd_reset_perf_counters(&simd_ctx.perf);
}
