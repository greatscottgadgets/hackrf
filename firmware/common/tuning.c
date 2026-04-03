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

#define MID1_HP_FREQ FP_MHZ(3600)
#define MID2_HP_FREQ FP_MHZ(5100)
#define MAX_HP_FREQ  FP_MHZ(7250)

static fp_40_24_t min_bypass_freq = FP_MHZ(2170);
static fp_40_24_t max_bypass_freq = FP_MHZ(2740);

void tuning_setup(void)
{
#if defined(PRALINE) || defined(UNIVERSAL)
	if (detected_platform() == BOARD_ID_PRALINE) {
		min_bypass_freq = FP_MHZ(2320);
		max_bypass_freq = FP_MHZ(2580);
	}
#endif
}

fp_40_24_t select_graduated_if(fp_40_24_t freq_rf, rf_path_filter_t img_reject)
{
	fp_40_24_t freq_if;

	switch (img_reject) {
	case RF_PATH_FILTER_LOW_PASS:
		if (detected_platform() == BOARD_ID_RAD1O) {
			freq_if = FP_MHZ(2300);
		} else {
			/* IF is graduated from 2650 MHz to 2340 MHz */
			freq_if = FP_MHZ(2650) - (freq_rf / 7);
		}
		break;
	case RF_PATH_FILTER_HIGH_PASS:
		if (freq_rf < MID1_HP_FREQ) {
			/* IF is graduated from 2170 MHz to 2740 MHz */
			freq_if = min_bypass_freq +
				(((freq_rf - (max_bypass_freq)) * 57) / 86);
		} else if (freq_rf < MID2_HP_FREQ) {
			/* IF is graduated from 2350 MHz to 2650 MHz */
			freq_if = FP_MHZ(2350) + ((freq_rf - (MID1_HP_FREQ)) / 5);
		} else {
			/* IF is graduated from 2500 MHz to 2738 MHz */
			freq_if = FP_MHZ(2500) + ((freq_rf - (MID2_HP_FREQ)) / 9);
		}
		break;
	default:
		freq_if = freq_rf;
	}
	return freq_if;
}

rf_path_filter_t select_img_reject(fp_40_24_t freq_rf)
{
	if (freq_rf > max_bypass_freq) {
		return RF_PATH_FILTER_HIGH_PASS;
	} else if (freq_rf >= min_bypass_freq) {
		return RF_PATH_FILTER_BYPASS;
	} else {
		return RF_PATH_FILTER_LOW_PASS;
	}
}

fp_40_24_t restrict_rf(fp_40_24_t freq_rf, rf_path_filter_t img_reject)
{
	fp_40_24_t min_rf = 0;
	fp_40_24_t max_rf = MAX_HP_FREQ;

	switch (img_reject) {
	case RF_PATH_FILTER_LOW_PASS:
		max_rf = min_bypass_freq;
		break;
	case RF_PATH_FILTER_HIGH_PASS:
		min_rf = max_bypass_freq;
		break;
	default:
		min_rf = min_bypass_freq;
		max_rf = max_bypass_freq;
	}
	freq_rf = MIN(freq_rf, max_rf);
	freq_rf = MAX(freq_rf, min_rf);

	return freq_rf;
}
