/*
 * Copyright 2012-2022 Great Scott Gadgets <info@greatscottgadgets.com>
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
#include "hackrf_ui.h"
#include "hackrf_core.h"
#include "mixer.h"
#include "max2831.h"
#include "max2837.h"
#include "max2839.h"
#include "sgpio.h"
#include "operacake.h"
#include "platform_detect.h"

#define FREQ_ONE_MHZ (1000ULL * 1000)

#ifndef PRALINE

	#define MIN_LP_FREQ_MHZ (0)
	#define MAX_LP_FREQ_MHZ (2170ULL)

	#define ABS_MIN_BYPASS_FREQ_MHZ (2000ULL)
	#define MIN_BYPASS_FREQ_MHZ     (MAX_LP_FREQ_MHZ)
	#define MAX_BYPASS_FREQ_MHZ     (2740ULL)
	#define ABS_MAX_BYPASS_FREQ_MHZ (3000ULL)

	#define MIN_HP_FREQ_MHZ  (MAX_BYPASS_FREQ_MHZ)
	#define MID1_HP_FREQ_MHZ (3600ULL)
	#define MID2_HP_FREQ_MHZ (5100ULL)
	#define MAX_HP_FREQ_MHZ  (7250ULL)

	#define MIN_LO_FREQ_HZ (84375000ULL)
	#define MAX_LO_FREQ_HZ (5400000000ULL)

#else

	#define MIN_LP_FREQ_MHZ (0)
	#define MAX_LP_FREQ_MHZ (2320ULL)

	#define ABS_MIN_BYPASS_FREQ_MHZ (2000ULL)
	#define MIN_BYPASS_FREQ_MHZ     (MAX_LP_FREQ_MHZ)
	#define MAX_BYPASS_FREQ_MHZ     (2580ULL)
	#define ABS_MAX_BYPASS_FREQ_MHZ (3000ULL)

	#define MIN_HP_FREQ_MHZ (MAX_BYPASS_FREQ_MHZ)
	#define MAX_HP_FREQ_MHZ (7250ULL)

	#define MIN_LO_FREQ_HZ (84375000ULL)
	#define MAX_LO_FREQ_HZ (5400000000ULL)

#endif

#ifndef PRALINE
static uint32_t max2837_freq_nominal_hz = 2560000000;

/*
 * Set freq/tuning between 0MHz to 7250 MHz (less than 16bits really used)
 * hz between 0 to 999999 Hz (not checked)
 * return false on error or true if success.
 */
bool set_freq(const uint64_t freq)
{
	bool success;
	uint64_t mixer_freq_hz;
	uint64_t real_mixer_freq_hz;

	const uint32_t freq_mhz = freq / FREQ_ONE_MHZ;

	success = true;

	max283x_mode_t prior_max283x_mode = max283x_mode(&max283x);
	max283x_set_mode(&max283x, MAX283x_MODE_STANDBY);
	if (freq_mhz < MAX_LP_FREQ_MHZ) {
		rf_path_set_filter(&rf_path, RF_PATH_FILTER_LOW_PASS);
	#ifdef RAD1O
		max2837_freq_nominal_hz = 2300 * FREQ_ONE_MHZ;
	#else
		/* IF is graduated from 2650 MHz to 2340 MHz */
		max2837_freq_nominal_hz = (2650 * FREQ_ONE_MHZ) - (freq / 7);
	#endif
		mixer_freq_hz = max2837_freq_nominal_hz + freq;
		/* Set Freq and read real freq */
		real_mixer_freq_hz = mixer_set_frequency(&mixer, mixer_freq_hz);
		max283x_set_frequency(&max283x, real_mixer_freq_hz - freq);
		sgpio_cpld_set_mixer_invert(&sgpio_config, 1);
	} else if ((freq_mhz >= MIN_BYPASS_FREQ_MHZ) && (freq_mhz < MAX_BYPASS_FREQ_MHZ)) {
		rf_path_set_filter(&rf_path, RF_PATH_FILTER_BYPASS);
		/* mixer_freq_mhz <= not used in Bypass mode */
		max283x_set_frequency(&max283x, freq);
		sgpio_cpld_set_mixer_invert(&sgpio_config, 0);
	} else if ((freq_mhz >= MIN_HP_FREQ_MHZ) && (freq_mhz <= MAX_HP_FREQ_MHZ)) {
		if (freq_mhz < MID1_HP_FREQ_MHZ) {
			/* IF is graduated from 2170 MHz to 2740 MHz */
			max2837_freq_nominal_hz = (MIN_BYPASS_FREQ_MHZ * FREQ_ONE_MHZ) +
				(((freq - (MAX_BYPASS_FREQ_MHZ * FREQ_ONE_MHZ)) * 57) /
				 86);
		} else if (freq_mhz < MID2_HP_FREQ_MHZ) {
			/* IF is graduated from 2350 MHz to 2650 MHz */
			max2837_freq_nominal_hz = (2350 * FREQ_ONE_MHZ) +
				((freq - (MID1_HP_FREQ_MHZ * FREQ_ONE_MHZ)) / 5);
		} else {
			/* IF is graduated from 2500 MHz to 2738 MHz */
			max2837_freq_nominal_hz = (2500 * FREQ_ONE_MHZ) +
				((freq - (MID2_HP_FREQ_MHZ * FREQ_ONE_MHZ)) / 9);
		}
		rf_path_set_filter(&rf_path, RF_PATH_FILTER_HIGH_PASS);
		mixer_freq_hz = freq - max2837_freq_nominal_hz;
		/* Set Freq and read real freq */
		real_mixer_freq_hz = mixer_set_frequency(&mixer, mixer_freq_hz);
		max283x_set_frequency(&max283x, freq - real_mixer_freq_hz);
		sgpio_cpld_set_mixer_invert(&sgpio_config, 0);
	} else {
		/* Error freq_mhz too high */
		success = false;
	}
	max283x_set_mode(&max283x, prior_max283x_mode);
	if (success) {
		hackrf_ui()->set_frequency(freq);
	#ifdef HACKRF_ONE
		operacake_set_range(freq_mhz);
	#endif
	}
	return success;
}

#else

bool tuning_set_frequency(const tune_config_t* cfg, const uint64_t freq)
{
	bool found;
	uint64_t mixer_freq_hz;
	uint64_t real_mixer_freq_hz;

	if (freq > (MAX_HP_FREQ_MHZ * FREQ_ONE_MHZ)) {
		return false;
	}

	const uint16_t freq_mhz = freq / FREQ_ONE_MHZ;

	found = false;
	for (; cfg->rf_range_end_mhz != 0; cfg++) {
		if (cfg->rf_range_end_mhz > freq_mhz) {
			found = true;
			break;
		}
	}

	if (!found) {
		return false;
	}

	max2831_mode_t prior_max2831_mode = max2831_mode(&max283x);
	max2831_set_mode(&max283x, MAX2831_MODE_STANDBY);

	if (cfg->if_mhz == 0) {
		rf_path_set_filter(&rf_path, RF_PATH_FILTER_BYPASS);
		max2831_set_frequency(&max283x, freq);
		sgpio_cpld_set_mixer_invert(&sgpio_config, 0);
	} else if (cfg->if_mhz > freq_mhz) {
		rf_path_set_filter(&rf_path, RF_PATH_FILTER_LOW_PASS);
		if (cfg->high_lo) {
			mixer_freq_hz = FREQ_ONE_MHZ * cfg->if_mhz + freq;
			real_mixer_freq_hz = mixer_set_frequency(&mixer, mixer_freq_hz);
			max2831_set_frequency(&max283x, real_mixer_freq_hz - freq);
			sgpio_cpld_set_mixer_invert(&sgpio_config, 1);
		} else {
			mixer_freq_hz = FREQ_ONE_MHZ * cfg->if_mhz - freq;
			real_mixer_freq_hz = mixer_set_frequency(&mixer, mixer_freq_hz);
			max2831_set_frequency(&max283x, real_mixer_freq_hz + freq);
			sgpio_cpld_set_mixer_invert(&sgpio_config, 0);
		}
	} else {
		rf_path_set_filter(&rf_path, RF_PATH_FILTER_HIGH_PASS);
		mixer_freq_hz = freq - FREQ_ONE_MHZ * cfg->if_mhz;
		real_mixer_freq_hz = mixer_set_frequency(&mixer, mixer_freq_hz);
		max2831_set_frequency(&max283x, freq - real_mixer_freq_hz);
		sgpio_cpld_set_mixer_invert(&sgpio_config, 0);
	}

	max2831_set_mode(&max283x, prior_max2831_mode);
	hackrf_ui()->set_frequency(freq);
	operacake_set_range(freq_mhz);
	return true;
}
#endif

bool set_freq_explicit(
	const uint64_t if_freq_hz,
	const uint64_t lo_freq_hz,
	const rf_path_filter_t path)
{
	if ((if_freq_hz < ((uint64_t) ABS_MIN_BYPASS_FREQ_MHZ * FREQ_ONE_MHZ)) ||
	    (if_freq_hz > ((uint64_t) ABS_MAX_BYPASS_FREQ_MHZ * FREQ_ONE_MHZ))) {
		return false;
	}

	if ((path != RF_PATH_FILTER_BYPASS) &&
	    ((lo_freq_hz < MIN_LO_FREQ_HZ) || (lo_freq_hz > MAX_LO_FREQ_HZ))) {
		return false;
	}

	if (path > 2) {
		return false;
	}

	rf_path_set_filter(&rf_path, path);
#ifdef PRALINE
	max2831_set_frequency(&max283x, if_freq_hz);
#else
	max283x_set_frequency(&max283x, if_freq_hz);
#endif
	if (lo_freq_hz > if_freq_hz) {
		sgpio_cpld_set_mixer_invert(&sgpio_config, 1);
	} else {
		sgpio_cpld_set_mixer_invert(&sgpio_config, 0);
	}
	if (path != RF_PATH_FILTER_BYPASS) {
		(void) mixer_set_frequency(&mixer, lo_freq_hz);
	}
	return true;
}
