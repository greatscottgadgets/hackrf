/*
 * Copyright 2012-2026 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2012 Jared Boone
 * Copyright 2013 Benjamin Vernoux
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

#include "tuning.h"

#include "fixed_point.h"
#include "platform_detect.h"

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#ifdef PRALINE
	#define MIN_BYPASS_FREQ_MHZ (2320ULL)
	#define MAX_BYPASS_FREQ_MHZ (2580ULL)
#else
	#define MIN_BYPASS_FREQ_MHZ (2170ULL)
	#define MAX_BYPASS_FREQ_MHZ (2740ULL)
#endif

#define MIN_LP_FREQ_MHZ (0)
#define MAX_LP_FREQ_MHZ (MIN_BYPASS_FREQ_MHZ)

#define ABS_MIN_BYPASS_FREQ_MHZ (2000ULL)
#define ABS_MAX_BYPASS_FREQ_MHZ (3000ULL)

#define MIN_HP_FREQ_MHZ  (MAX_BYPASS_FREQ_MHZ)
#define MID1_HP_FREQ_MHZ (3600ULL)
#define MID2_HP_FREQ_MHZ (5100ULL)
#define MAX_HP_FREQ_MHZ  (7250ULL)

#define MIN_LO_FREQ_HZ (84375000ULL)
#define MAX_LO_FREQ_HZ (5400000000ULL)

fp_40_24_t select_graduated_if(fp_40_24_t freq_rf, rf_path_filter_t img_reject)
{
	fp_40_24_t freq_if;

	switch (img_reject) {
	case RF_PATH_FILTER_LOW_PASS:
		if (detected_platform() == BOARD_ID_RAD1O) {
			freq_if = 2300 * FP_ONE_MHZ;
		} else {
			/* IF is graduated from 2650 MHz to 2340 MHz */
			freq_if = (2650 * FP_ONE_MHZ) - (freq_rf / 7);
		}
		break;
	case RF_PATH_FILTER_HIGH_PASS:
		if (freq_rf < (MID1_HP_FREQ_MHZ * FP_ONE_MHZ)) {
			/* IF is graduated from 2170 MHz to 2740 MHz */
			freq_if = (MIN_BYPASS_FREQ_MHZ * FP_ONE_MHZ) +
				(((freq_rf - (MAX_BYPASS_FREQ_MHZ * FP_ONE_MHZ)) * 57) /
				 86);
		} else if (freq_rf < (MID2_HP_FREQ_MHZ * FP_ONE_MHZ)) {
			/* IF is graduated from 2350 MHz to 2650 MHz */
			freq_if = (2350 * FP_ONE_MHZ) +
				((freq_rf - (MID1_HP_FREQ_MHZ * FP_ONE_MHZ)) / 5);
		} else {
			/* IF is graduated from 2500 MHz to 2738 MHz */
			freq_if = (2500 * FP_ONE_MHZ) +
				((freq_rf - (MID2_HP_FREQ_MHZ * FP_ONE_MHZ)) / 9);
		}
		break;
	default:
		freq_if = freq_rf;
	}
	return freq_if;
}

rf_path_filter_t select_img_reject(fp_40_24_t freq_rf)
{
	if (freq_rf > (MIN_HP_FREQ_MHZ * FP_ONE_MHZ)) {
		return RF_PATH_FILTER_HIGH_PASS;
	} else if (freq_rf >= (MIN_BYPASS_FREQ_MHZ * FP_ONE_MHZ)) {
		return RF_PATH_FILTER_BYPASS;
	} else {
		return RF_PATH_FILTER_LOW_PASS;
	}
}

fp_40_24_t restrict_rf(fp_40_24_t freq_rf, rf_path_filter_t img_reject)
{
	fp_40_24_t min_rf = 0;
	fp_40_24_t max_rf = MAX_HP_FREQ_MHZ * FP_ONE_MHZ;

	switch (img_reject) {
	case RF_PATH_FILTER_LOW_PASS:
		max_rf = MAX_LP_FREQ_MHZ * FP_ONE_MHZ;
		break;
	case RF_PATH_FILTER_HIGH_PASS:
		min_rf = MIN_HP_FREQ_MHZ * FP_ONE_MHZ;
		break;
	default:
		min_rf = MIN_BYPASS_FREQ_MHZ * FP_ONE_MHZ;
		max_rf = MAX_BYPASS_FREQ_MHZ * FP_ONE_MHZ;
	}
	freq_rf = MIN(freq_rf, max_rf);
	freq_rf = MAX(freq_rf, min_rf);

	return freq_rf;
}
