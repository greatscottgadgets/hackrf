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

#ifndef USB_API_DSP_SIMD_H
#define USB_API_DSP_SIMD_H

#include <usb_request.h>

// USB API for SIMD DSP functions
// These vendor requests allow host software to control SIMD-optimized DSP processing

usb_request_status_t usb_vendor_request_dsp_simd_enable(
    usb_endpoint_t* const endpoint,
    const usb_transfer_stage_t stage);

usb_request_status_t usb_vendor_request_dsp_simd_filter_config(
    usb_endpoint_t* const endpoint,
    const usb_transfer_stage_t stage);

usb_request_status_t usb_vendor_request_dsp_simd_iq_config(
    usb_endpoint_t* const endpoint,
    const usb_transfer_stage_t stage);

usb_request_status_t usb_vendor_request_dsp_simd_benchmark(
    usb_endpoint_t* const endpoint,
    const usb_transfer_stage_t stage);

usb_request_status_t usb_vendor_request_dsp_simd_status(
    usb_endpoint_t* const endpoint,
    const usb_transfer_stage_t stage);

// Performance monitoring
void dsp_simd_update_performance(void);

#endif // USB_API_DSP_SIMD_H
