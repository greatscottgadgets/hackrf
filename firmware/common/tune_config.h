/*
 * Copyright 2024-2025 Great Scott Gadgets <info@greatscottgadgets.com>
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

#ifndef __TUNE_CONFIG_H__
#define __TUNE_CONFIG_H__

typedef struct {
	uint16_t rf_range_end_mhz;
	uint16_t if_mhz;
	bool high_lo;
} tune_config_t;

#ifndef PRALINE
// TODO maybe one day?
static const tune_config_t max283x_tune_config[] = {};
#else
// clang-format off
/* tuning table optimized for TX */
static const tune_config_t max2831_tune_config_tx[] = {
	{ 2100, 2375, true  },
	{ 2175, 2325, false },
	{ 2320, 2525, false },
	{ 2580,    0, false },
	{ 3000, 2325, false },
	{ 3100, 2375, false },
	{ 3200, 2425, false },
	{ 3350, 2375, false },
	{ 3500, 2475, false },
	{ 3550, 2425, false },
	{ 3650, 2325, false },
	{ 3700, 2375, false },
	{ 3850, 2425, false },
	{ 3925, 2375, false },
	{ 4600, 2325, false },
	{ 4700, 2375, false },
	{ 4800, 2425, false },
	{ 5100, 2375, false },
	{ 5850, 2525, false },
	{ 6500, 2325, false },
	{ 6750, 2375, false },
	{ 6850, 2425, false },
	{ 6950, 2475, false },
	{ 7000, 2525, false },
	{ 7251, 2575, false },
	{    0,    0, false },
};

/* tuning table optimized for 20 Msps interleaved RX sweep mode */
static const tune_config_t max2831_tune_config_rx_sweep[] = {
	{  140, 2330, false },
	{  424, 2570, true },
	{  557, 2520, true },
	{  593, 2380, true },
	{  776, 2360, true },
	{  846, 2570, true },
	{  926, 2500, true },
	{ 1055, 2380, true },
	{ 1175, 2360, true },
	{ 1391, 2340, true },
	{ 1529, 2570, true },
	{ 1671, 2520, true },
	{ 1979, 2380, true },
	{ 2150, 2330, true },
	{ 2160, 2550, false },
	{ 2170, 2560, false },
	{ 2179, 2570, false },
	{ 2184, 2520, false },
	{ 2187, 2560, false },
	{ 2194, 2530, false },
	{ 2203, 2540, false },
	{ 2212, 2550, false },
	{ 2222, 2560, false },
	{ 2231, 2570, false },
	{ 2233, 2530, false },
	{ 2237, 2520, false },
	{ 2241, 2550, false },
	{ 2245, 2570, false },
	{ 2250, 2560, false },
	{ 2252, 2550, false },
	{ 2258, 2570, false },
	{ 2261, 2560, false },
	{ 2266, 2540, false },
	{ 2271, 2570, false },
	{ 2275, 2550, false },
	{ 2280, 2500, false },
	{ 2284, 2560, false },
	{ 2285, 2530, false },
	{ 2289, 2510, false },
	{ 2293, 2570, false },
	{ 2294, 2540, false },
	{ 2298, 2520, false },
	{ 2301, 2570, false },
	{ 2302, 2550, false },
	{ 2307, 2530, false },
	{ 2308, 2560, false },
	{ 2312, 2560, false },
	{ 2316, 2540, false },
	{ 2317, 2570, false },
	{ 2320, 2570, false },
	{ 2580,    0, false },
	{ 2585, 2360, false },
	{ 2588, 2340, false },
	{ 2594, 2350, false },
	{ 2606, 2330, false },
	{ 2617, 2340, false },
	{ 2627, 2350, false },
	{ 2638, 2360, false },
	{ 2649, 2370, false },
	{ 2659, 2380, false },
	{ 2664, 2350, false },
	{ 2675, 2360, false },
	{ 2686, 2370, false },
	{ 2697, 2380, false },
	{ 2705, 2350, false },
	{ 2716, 2360, false },
	{ 2728, 2370, false },
	{ 2739, 2380, false },
	{ 2757, 2330, false },
	{ 2779, 2350, false },
	{ 2790, 2360, false },
	{ 2801, 2370, false },
	{ 2812, 2380, false },
	{ 2821, 2570, false },
	{ 2831, 2520, false },
	{ 2852, 2330, false },
	{ 2874, 2350, false },
	{ 2897, 2370, false },
	{ 2913, 2510, false },
	{ 2925, 2570, false },
	{ 2937, 2530, false },
	{ 2948, 2540, false },
	{ 2960, 2550, false },
	{ 2975, 2330, false },
	{ 2988, 2340, false },
	{ 3002, 2330, false },
	{ 3014, 2360, false },
	{ 3027, 2370, false },
	{ 3041, 2500, false },
	{ 3052, 2510, false },
	{ 3064, 2520, false },
	{ 3082, 2500, false },
	{ 3107, 2520, false },
	{ 3132, 2540, false },
	{ 3157, 2560, false },
	{ 3170, 2570, false },
	{ 3192, 2500, false },
	{ 3216, 2340, false },
	{ 3270, 2330, false },
	{ 3319, 2370, false },
	{ 3341, 2340, false },
	{ 3370, 2330, false },
	{ 3400, 2350, false },
	{ 3430, 2370, false },
	{ 3464, 2520, false },
	{ 3491, 2540, false },
	{ 3519, 2560, false },
	{ 3553, 2510, false },
	{ 3595, 2540, false },
	{ 3638, 2570, false },
	{ 3665, 2540, false },
	{ 3685, 2560, false },
	{ 3726, 2330, false },
	{ 3790, 2370, false },
	{ 3910, 2350, false },
	{ 4014, 2510, false },
	{ 4123, 2380, false },
	{ 4191, 2550, false },
	{ 4349, 2510, false },
	{ 4452, 2570, false },
	{ 4579, 2500, false },
	{ 4707, 2570, false },
	{ 4831, 2560, false },
	{ 4851, 2570, false },
	{ 4871, 2560, false },
	{ 4891, 2570, false },
	{ 4911, 2540, false },
	{ 4931, 2550, false },
	{ 4951, 2560, false },
	{ 5044, 2330, false },
	{ 5065, 2340, false },
	{ 5174, 2330, false },
	{ 5285, 2380, false },
	{ 5449, 2340, false },
	{ 5574, 2510, false },
	{ 5717, 2340, false },
	{ 5892, 2530, false },
	{ 6096, 2350, false },
	{ 6254, 2560, false },
	{ 6625, 2340, false },
	{ 6764, 2540, false },
	{ 6930, 2530, false },
	{ 7251, 2570, false },
	{    0,    0, false },
};

// TODO these are just copies of max2831_tune_config_rx_sweep for now
#define max2831_tune_config_rx max2831_tune_config_rx_sweep
	// clang-format on

#endif

#endif /*__TUNE_CONFIG_H__*/
