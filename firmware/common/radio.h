/*
 * Copyright 2012-2025 Great Scott Gadgets <info@greatscottgadgets.com>
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

#ifndef __RADIO_H__
#define __RADIO_H__

#include "hackrf_core.h"
#include "radio_types.h"

// TODO these should be distinct for each platform and ideally checked at compile time
#define RADIO_CHANNEL_COUNT     1
#define RADIO_SAMPLE_RATE_COUNT 4
#define RADIO_FILTER_COUNT      5
#define RADIO_FREQUENCY_COUNT   4
#define RADIO_GAIN_COUNT        4
#define RADIO_ANTENNA_COUNT     1
#define RADIO_CLOCK_COUNT       2
#define RADIO_MODE_COUNT        6

/**
 * Bandwidth configuration.
 */
#ifndef PRALINE
typedef struct {
	uint32_t mcu_rate;
	uint32_t mcu_div;
	uint32_t baseband_hz;
} radio_mode_sample_rate_t;
#else
typedef struct {
	uint32_t mcu_rate;            // how fast are we pushing packets from fpga to usb
	uint32_t mcu_div;             // PLL divisor for mcu_rate
	uint32_t baseband_hz;         // max2831 rx or tx low pass filter
	uint8_t resampling_ratio;     // fpga interpolation/decimation 3 bits (0-7)
	bool rx_narrowband_aa_enable; // narrowband aa filter (on/off)
	max2831_rx_hpf_freq_t
		rx_baseband_hpf_hz; // max2831 corner frequency (100hz, 4 kHz, 30 kHz, 600 kHz)
} radio_mode_sample_rate_t;
#endif

/**
 * Center Frequency configuration.
 */
#ifndef PRALINE
typedef struct {
	uint64_t hz;
	uint64_t if_hz;
	uint64_t lo_hz;
	rf_path_filter_t rf_path_filter;
} radio_mode_frequency_t;
#else
typedef struct {
	uint64_t hz;                                     // effective center frequency
	uint64_t if_hz;                                  // intermediate frequency
	uint64_t lo_hz;                                  // local oscillator frequency
	rf_path_filter_t rf_path_filter;                 // image rejection filter path
	fpga_quarter_shift_mode_t rx_quarter_shift_mode; // off, up, down
} radio_mode_frequency_t;
#endif

/**
 * Radio configuration.
 *
 * TODO make a final decision on whether we're going to vary the type for HackRF One vs Pro
 */
typedef struct {
	// sample rate elements
	radio_sample_rate_t sample_rate[RADIO_SAMPLE_RATE_COUNT];

	// filter elements
	radio_filter_t filter[RADIO_FILTER_COUNT];

	// gain elements
	radio_gain_t gain[RADIO_GAIN_COUNT];

	// frequency elements
	radio_frequency_t frequency[RADIO_FREQUENCY_COUNT];

	// antenna elements
	radio_antenna_t antenna[RADIO_ANTENNA_COUNT];

	// clock elements
	radio_clock_t clock[RADIO_CLOCK_COUNT];

	// trigger elements
	hw_sync_mode_t trigger_mode;

	// currently active radio mode
	radio_mode_t mode;
} radio_config_t;

/**
 * Radio channel configuration.
 */
typedef struct radio_channel_t {
	radio_chan_id id;
	radio_config_t config;
	radio_config_t mode[RADIO_MODE_COUNT];
	clock_source_t clock_source;
} radio_channel_t;

/**
 * Radio root.
 */
typedef struct radio_t {
	radio_channel_t channel[RADIO_CHANNEL_COUNT];
} radio_t;

extern radio_t radio;

/**
 * API Notes
 *
 * - All radio_set_*() functions return a radio_error_t
 * - radio_set_*() functions interact with configuration & hardware state as follows:
 *   0. if the specified radio mode is the same as the current radio mode then both the
 *      configuration and hardware state will be updated.
 *   1. if the active radio mode is RADIO_MODE_OFF only the configuration will
 *      be updated, the hardware state will remain unaffected.
 *   2. if the specified radio mode is different to the current radio mode only the
 *      configuration will be updated, the hardware state will remain unaffected.
 *   3. if the specified radio mode is RADIO_MODE_ACTIVE then the configuration
 *      of the active mode will be updated and the hardware state will be updated.
 *   4. if the specified radio mode is RADIO_MODE_ALL then the configuration
 *      for all modes will be updated and the hardware state will be updated.
 * - this makes it possible to maintain multiple channel configurations and
 *   switch between them with a single call to radio_switch_mode()
 * - for all functions returning lists, the caller is responsible for
 *   managing the list buffer.
 */

/**
 * Returns the sample rate range supported by the specified radio mode.
 */
radio_error_t radio_supported_sample_rate(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	radio_range_t* range);

/**
 * Automatically configure the radio for the given sample rate and mode.
 */
radio_error_t radio_set_mode_sample_rate(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const uint32_t rate_num,
	const uint32_t rate_denom);

/**
 * Automatically configure the radio for the given center frequency and mode.
 */
radio_error_t radio_set_mode_frequency(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const uint64_t hz);

/**
 * Configure the radio with the given center frequency settings.
 */
radio_error_t radio_set_mode_frequency_explicit(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_mode_frequency_t config);

/**
 * Returns the mode's center frequency configuration.
 */
radio_mode_frequency_t radio_get_mode_frequency(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode);

/**
 * Applies the current stored configuration values for the requested mode.
 */
radio_error_t radio_switch_mode(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode);

/**
 * Returns the currently active mode for the radio.
 */
radio_mode_t radio_get_mode(radio_t* radio, const radio_chan_id chan_id);

/**
 * Returns the currently active transceiver direction of the radio.
 */
radio_direction_t radio_get_direction(radio_t* radio, const radio_chan_id chan_id);

/**
 * Returns the currently active clock source of the radio.
 */
clock_source_t radio_get_clock_source(radio_t* radio, const radio_chan_id chan_id);

/**
 * Trigger mode
 */
radio_error_t radio_set_trigger_mode(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const hw_sync_mode_t hw_sync_mode);

hw_sync_mode_t radio_get_trigger_mode(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode);

/**
 * Sample Rate elements
 */
radio_error_t radio_set_sample_rate_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_sample_rate_id element,
	const radio_sample_rate_t value);

radio_sample_rate_t radio_get_sample_rate_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_sample_rate_id element);

/**
 * Filter elements
 */
radio_error_t radio_set_filter_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_filter_id element,
	const radio_filter_t filter);

radio_filter_t radio_get_filter_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_filter_id element);

radio_error_t radio_supported_filter_element_bandwidths(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_filter_id element,
	uint32_t* list,
	size_t* length);

/**
 * Frequency elements
 */
radio_error_t radio_set_frequency_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_frequency_id element,
	const radio_frequency_t frequency);

radio_frequency_t radio_get_frequency_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_frequency_id element);

/**
 * Gain elements
 */
radio_error_t radio_set_gain_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_gain_id element,
	const radio_gain_t gain);

radio_gain_t radio_get_gain_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_gain_id element);

/**
 * Antenna elements
 */
radio_error_t radio_set_antenna_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_antenna_id element,
	const radio_antenna_t value);

radio_antenna_t radio_get_antenna_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_antenna_id element);

/**
 * Clock elements
 */
radio_error_t radio_set_clock_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_clock_id element,
	const radio_clock_t value);

radio_clock_t radio_get_clock_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_clock_id element);

#endif /*__RADIO_H__*/
