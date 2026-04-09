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

#include "usb_api_dsp_simd.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <dsp_simd.h>
#include <streaming_simd.h>
#include <dsp_benchmark.h>
#include <usb.h>
#include <usb_queue.h>
#include <usb_request.h>
#include <usb_type.h>

#include "usb_bulk_buffer.h"
#include "usb_endpoint.h"

// DSP configuration structure
typedef struct {
    bool simd_enabled;
    bool filters_enabled;
    bool iq_correction_enabled;
    uint8_t current_filter_taps;
    uint16_t performance_counter;
} dsp_simd_config_t;

static dsp_simd_config_t dsp_config = {
    .simd_enabled = true,
    .filters_enabled = false,
    .iq_correction_enabled = false,
    .current_filter_taps = 8,
    .performance_counter = 0
};

// Default filter coefficients for demonstration
static const int16_t default_lowpass_coeffs[8] = {
    256, 512, 768, 1024, 768, 512, 256, 128
};

static const int16_t default_highpass_coeffs[8] = {
    128, -256, 512, -768, 1024, -768, 512, -256
};

usb_request_status_t usb_vendor_request_dsp_simd_enable(
    usb_endpoint_t* const endpoint,
    const usb_transfer_stage_t stage)
{
    if (stage == USB_TRANSFER_STAGE_SETUP) {
        dsp_config.simd_enabled = endpoint->setup.value != 0;
        
        if (dsp_config.simd_enabled) {
            streaming_simd_init();
        }
        
        usb_transfer_schedule_ack(endpoint->in);
    }
    return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_dsp_simd_filter_config(
    usb_endpoint_t* const endpoint,
    const usb_transfer_stage_t stage)
{
    if (stage == USB_TRANSFER_STAGE_SETUP) {
        const uint8_t filter_type = endpoint->setup.value & 0xFF;
        const uint8_t num_taps = (endpoint->setup.value >> 8) & 0xFF;
        
        if (num_taps > 0 && num_taps <= 32) {
            const int16_t* coeffs = NULL;
            
            switch (filter_type) {
            case 0: // Low-pass filter
                coeffs = default_lowpass_coeffs;
                break;
            case 1: // High-pass filter
                coeffs = default_highpass_coeffs;
                break;
            default:
                coeffs = default_lowpass_coeffs;
                break;
            }
            
            streaming_simd_set_tx_filter(coeffs, num_taps);
            streaming_simd_set_rx_filter(coeffs, num_taps);
            dsp_config.current_filter_taps = num_taps;
            dsp_config.filters_enabled = true;
        }
        
        usb_transfer_schedule_ack(endpoint->in);
    }
    return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_dsp_simd_iq_config(
    usb_endpoint_t* const endpoint,
    const usb_transfer_stage_t stage)
{
    if (stage == USB_TRANSFER_STAGE_SETUP) {
        // I/Q correction parameters: gain_i, gain_q, offset_i, offset_q, phase_corr
        // All values are in Q15 format
        streaming_simd_set_iq_correction(
            4096,  // Unity gain for I (1.0 in Q15)
            4096,  // Unity gain for Q (1.0 in Q15)
            (int16_t)endpoint->setup.value,      // I offset
            (int16_t)endpoint->setup.index,      // Q offset
            0      // No phase correction
        );
        
        dsp_config.iq_correction_enabled = true;
        usb_transfer_schedule_ack(endpoint->in);
    }
    return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_dsp_simd_benchmark(
    usb_endpoint_t* const endpoint,
    const usb_transfer_stage_t stage)
{
    if (stage == USB_TRANSFER_STAGE_SETUP) {
        dsp_benchmark_results_t results;
        dsp_benchmark_run_all(&results);
        
        // Return benchmark results to host
        uint32_t* results_ptr = (uint32_t*)endpoint->buffer;
        results_ptr[0] = (uint32_t)(results.fir_filter.performance_improvement * 100);
        results_ptr[1] = (uint32_t)(results.iq_correction.performance_improvement * 100);
        results_ptr[2] = (uint32_t)(results.dot_product.performance_improvement * 100);
        results_ptr[3] = (uint32_t)(results.overall_improvement * 100);
        
        usb_transfer_schedule_block(
            endpoint->in,
            endpoint->buffer,
            16, // 4 * uint32_t
            NULL,
            NULL
        );
    } else if (stage == USB_TRANSFER_STAGE_DATA) {
        usb_transfer_schedule_ack(endpoint->in);
    }
    return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request_dsp_simd_status(
    usb_endpoint_t* const endpoint,
    const usb_transfer_stage_t stage)
{
    if (stage == USB_TRANSFER_STAGE_SETUP) {
        // Return current DSP configuration status
        uint32_t status = 0;
        status |= (dsp_config.simd_enabled ? 0x01 : 0);
        status |= (dsp_config.filters_enabled ? 0x02 : 0);
        status |= (dsp_config.iq_correction_enabled ? 0x04 : 0);
        status |= ((uint32_t)dsp_config.current_filter_taps << 8);
        
        endpoint->buffer[0] = status & 0xFF;
        endpoint->buffer[1] = (status >> 8) & 0xFF;
        endpoint->buffer[2] = (status >> 16) & 0xFF;
        endpoint->buffer[3] = (status >> 24) & 0xFF;
        
        usb_transfer_schedule_block(
            endpoint->in,
            endpoint->buffer,
            4,
            NULL,
            NULL
        );
    } else if (stage == USB_TRANSFER_STAGE_DATA) {
        usb_transfer_schedule_ack(endpoint->in);
    }
    return USB_REQUEST_STATUS_OK;
}

// Performance monitoring function - can be called periodically
void dsp_simd_update_performance(void) {
    if (dsp_config.simd_enabled) {
        dsp_simd_perf_t perf;
        streaming_simd_get_performance(&perf);
        
        // Update performance counter (could be used for adaptive processing)
        if (perf.samples_processed > 0) {
            uint32_t avg_cycles = perf.cycles_fir_sample / perf.samples_processed;
            if (avg_cycles > 100) {
                // If processing is taking too long, consider reducing filter complexity
                dsp_config.performance_counter++;
            }
        }
    }
}
