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

#include "dsp_benchmark.h"
#include "dsp_simd.h"
#include <string.h>
#include <stdio.h>
#include <libopencm3/cm3/common.h>
#include <libopencm3/cm3/memorymap.h>
#include <libopencm3/cm3/scs.h>

// Test data for benchmarking
static int16_t test_input[1024];
static int32_t test_output_32[1024];
static const int16_t test_coeffs[8] = {128, 64, 32, 16, 8, 4, 2, 1};

// Traditional (non-SIMD) FIR filter implementation for comparison
static int32_t traditional_fir_filter(const int16_t* coeffs, const int16_t* input, 
                                     uint8_t num_taps, uint16_t input_len) {
    int32_t output = 0;
    for (uint8_t i = 0; i < num_taps; i++) {
        if (i < input_len) {
            output += coeffs[i] * input[input_len - 1 - i];
        }
    }
    return output;
}

static void traditional_iq_correction(iq_sample_t* sample, int16_t gain_i, int16_t gain_q,
                                     int16_t offset_i, int16_t offset_q, int16_t phase_corr) {
    // Apply gain correction
    int32_t i_corrected = sample->i * gain_i + offset_i;
    int32_t q_corrected = sample->q * gain_q + offset_q;
    
    // Apply phase correction
    int32_t i_final = i_corrected - ((q_corrected * phase_corr) >> 15);
    int32_t q_final = q_corrected + ((i_corrected * phase_corr) >> 15);
    
    // Saturate
    sample->i = (int16_t)((i_final > 32767) ? 32767 : ((i_final < -32768) ? -32768 : i_final));
    sample->q = (int16_t)((q_final > 32767) ? 32767 : ((q_final < -32768) ? -32768 : q_final));
}

// Cycle counter for benchmarking
static inline uint32_t benchmark_get_cycles(void) {
    return SCS_DWT_CYCCNT;
}

void dsp_benchmark_init(void) {
    // Initialize test data with pseudo-random values
    for (uint16_t i = 0; i < 1024; i++) {
        test_input[i] = (int16_t)(i * 0x1234 + 0x5678);
    }
    
    // Enable DWT cycle counter for accurate timing
    // Enable trace and cycle counter
    SCS_DEMCR |= SCS_DEMCR_TRCENA;
    SCS_DWT_CTRL |= SCS_DWT_CTRL_CYCCNTENA;
}

void dsp_benchmark_fir_filter(dsp_benchmark_result_t* result) {
    if (!result) return;
    
    fir_filter_simd_t simd_filter;
    dsp_simd_init_fir_filter(&simd_filter, test_coeffs, 8);
    
    const uint16_t iterations = 1000;
    const uint16_t block_size = 128;
    
    // Benchmark SIMD implementation
    uint32_t start_cycles = benchmark_get_cycles();
    for (uint16_t iter = 0; iter < iterations; iter++) {
        for (uint16_t i = 0; i < block_size; i++) {
            test_output_32[i] = dsp_simd_fir_filter_sample(&simd_filter, test_input[i]);
        }
    }
    uint32_t simd_cycles = benchmark_get_cycles() - start_cycles;
    
    // Benchmark traditional implementation
    start_cycles = benchmark_get_cycles();
    for (uint16_t iter = 0; iter < iterations; iter++) {
        for (uint16_t i = 0; i < block_size; i++) {
            test_output_32[i] = traditional_fir_filter(test_coeffs, &test_input[i > 7 ? i - 7 : 0], 
                                                      8, i > 7 ? 8 : i + 1);
        }
    }
    uint32_t traditional_cycles = benchmark_get_cycles() - start_cycles;
    
    // Calculate results
    result->simd_cycles_per_sample = simd_cycles / (iterations * block_size);
    result->traditional_cycles_per_sample = traditional_cycles / (iterations * block_size);
    result->performance_improvement = (float)traditional_cycles / (float)simd_cycles;
    result->test_type = DSP_BENCHMARK_FIR_FILTER;
}

void dsp_benchmark_iq_correction(dsp_benchmark_result_t* result) {
    if (!result) return;
    
    iq_correction_simd_t simd_corr;
    dsp_simd_init_iq_correction(&simd_corr, 4096, 4096, 100, -50, 256);
    
    const uint16_t iterations = 1000;
    const uint16_t block_size = 256; // Number of I/Q pairs
    
    // Benchmark SIMD implementation
    uint32_t start_cycles = benchmark_get_cycles();
    for (uint16_t iter = 0; iter < iterations; iter++) {
        dsp_simd_iq_correct_block(&simd_corr, (iq_sample_t*)test_input, block_size);
    }
    uint32_t simd_cycles = benchmark_get_cycles() - start_cycles;
    
    // Benchmark traditional implementation
    start_cycles = benchmark_get_cycles();
    for (uint16_t iter = 0; iter < iterations; iter++) {
        for (uint16_t i = 0; i < block_size; i++) {
            traditional_iq_correction((iq_sample_t*)&test_input[i*2], 4096, 4096, 100, -50, 256);
        }
    }
    uint32_t traditional_cycles = benchmark_get_cycles() - start_cycles;
    
    // Calculate results
    result->simd_cycles_per_sample = simd_cycles / (iterations * block_size);
    result->traditional_cycles_per_sample = traditional_cycles / (iterations * block_size);
    result->performance_improvement = (float)traditional_cycles / (float)simd_cycles;
    result->test_type = DSP_BENCHMARK_IQ_CORRECTION;
}

void dsp_benchmark_dot_product(dsp_benchmark_result_t* result) {
    if (!result) return;
    
    const uint16_t iterations = 1000;
    const uint16_t vector_length = 64;
    
    // Benchmark SIMD implementation
    uint32_t start_cycles = benchmark_get_cycles();
    for (uint16_t iter = 0; iter < iterations; iter++) {
        volatile int32_t simd_result = dsp_simd_dot_product(test_input, test_input, vector_length);
        (void)simd_result; // Suppress unused warning
    }
    uint32_t simd_cycles = benchmark_get_cycles() - start_cycles;
    
    // Benchmark traditional implementation
    start_cycles = benchmark_get_cycles();
    volatile int32_t traditional_result = 0;
    for (uint16_t iter = 0; iter < iterations; iter++) {
        traditional_result = 0;
        for (uint16_t i = 0; i < vector_length; i++) {
            traditional_result += test_input[i] * test_input[i];
        }
    }
    uint32_t traditional_cycles = benchmark_get_cycles() - start_cycles;
    
    // Calculate results
    result->simd_cycles_per_sample = simd_cycles / iterations;
    result->traditional_cycles_per_sample = traditional_cycles / iterations;
    result->performance_improvement = (float)traditional_cycles / (float)simd_cycles;
    result->test_type = DSP_BENCHMARK_DOT_PRODUCT;
}

void dsp_benchmark_run_all(dsp_benchmark_results_t* results) {
    if (!results) return;
    
    dsp_benchmark_init();
    
    printf("Running DSP SIMD benchmarks...\n");
    
    // Run all benchmarks
    dsp_benchmark_fir_filter(&results->fir_filter);
    printf("FIR Filter: %.2fx improvement (%.1f vs %.1f cycles)\n", 
           results->fir_filter.performance_improvement,
           (float)results->fir_filter.traditional_cycles_per_sample,
           (float)results->fir_filter.simd_cycles_per_sample);
    
    dsp_benchmark_iq_correction(&results->iq_correction);
    printf("I/Q Correction: %.2fx improvement (%.1f vs %.1f cycles)\n", 
           results->iq_correction.performance_improvement,
           (float)results->iq_correction.traditional_cycles_per_sample,
           (float)results->iq_correction.simd_cycles_per_sample);
    
    dsp_benchmark_dot_product(&results->dot_product);
    printf("Dot Product: %.2fx improvement (%.1f vs %.1f cycles)\n", 
           results->dot_product.performance_improvement,
           (float)results->dot_product.traditional_cycles_per_sample,
           (float)results->dot_product.simd_cycles_per_sample);
    
    // Calculate overall improvement
    results->overall_improvement = (results->fir_filter.performance_improvement + 
                                   results->iq_correction.performance_improvement + 
                                   results->dot_product.performance_improvement) / 3.0f;
    
    printf("Overall DSP Performance Improvement: %.2fx\n", results->overall_improvement);
}
