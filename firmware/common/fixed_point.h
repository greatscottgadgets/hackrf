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

#pragma once

#include <stdint.h>

/* 40.24 fixed-point */
typedef uint64_t fp_40_24_t;

/* one million in 40.24 fixed-point */
#define FP_ONE_MHZ ((1000ULL * 1000ULL) << 24)

/* one thousand in 40.24 fixed-point */
#define FP_ONE_KHZ ((1000ULL) << 24)

/* one in 40.24 fixed-point */
#define FP_ONE_HZ (1 << 24)

#define FP_MHZ(mhz) (mhz##ULL * FP_ONE_MHZ)
#define FP_KHZ(khz) (khz##ULL * FP_ONE_KHZ)
#define FP_HZ(hz)   (hz##ULL * FP_ONE_HZ)

/* 28.36 fixed-point */
typedef uint64_t fp_28_36_t;

/* one million in 28.36 fixed-point */
#define SR_FP_ONE_MHZ ((1000ULL * 1000ULL) << 36)

/* one thousand in 28.36 fixed-point */
#define SR_FP_ONE_KHZ (1000ULL << 36)

/* one in 28.36 fixed-point */
#define SR_FP_ONE_HZ (1ULL << 36)

#define SR_FP_MHZ(mhz) (mhz##ULL * SR_FP_ONE_MHZ)
#define SR_FP_KHZ(khz) (khz##ULL * SR_FP_ONE_KHZ)
#define SR_FP_HZ(hz)   (hz##ULL * SR_FP_ONE_HZ)
