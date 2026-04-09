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

#ifndef DSP_BENCHMARK_H
#define DSP_BENCHMARK_H

#include <stdint.h>
#include <stdbool.h>

// Benchmark test types
typedef enum {
    DSP_BENCHMARK_FIR_FILTER,
    DSP_BENCHMARK_IQ_CORRECTION,
    DSP_BENCHMARK_DOT_PRODUCT
} dsp_benchmark_test_t;

// Single benchmark result
typedef struct {
    dsp_benchmark_test_t test_type;
    uint32_t simd_cycles_per_sample;
    uint32_t traditional_cycles_per_sample;
    float performance_improvement;
} dsp_benchmark_result_t;

// Complete benchmark results
typedef struct {
    dsp_benchmark_result_t fir_filter;
    dsp_benchmark_result_t iq_correction;
    dsp_benchmark_result_t dot_product;
    float overall_improvement;
} dsp_benchmark_results_t;

// Benchmark function prototypes
void dsp_benchmark_init(void);
void dsp_benchmark_fir_filter(dsp_benchmark_result_t* result);
void dsp_benchmark_iq_correction(dsp_benchmark_result_t* result);
void dsp_benchmark_dot_product(dsp_benchmark_result_t* result);
void dsp_benchmark_run_all(dsp_benchmark_results_t* results);

#endif // DSP_BENCHMARK_H
