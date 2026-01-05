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

#ifndef __RF_CONFIG_H__
#define __RF_CONFIG_H__

#include <stdint.h>
#include <stdbool.h>

#include "rf_path.h"
#include "fpga.h"

typedef enum {
	RADIO_OK = 1,
	RADIO_ERR_INVALID_PARAM = -2,
	RADIO_ERR_INVALID_CONFIG = -3,
	RADIO_ERR_INVALID_CHANNEL = -4,
	RADIO_ERR_INVALID_ELEMENT = -5,
	RADIO_ERR_UNSUPPORTED_OPERATION = -10,
	RADIO_ERR_UNIMPLEMENTED = -19,
	RADIO_ERR_OTHER = -9999,
} radio_error_t;

typedef enum {
	RADIO_CHANNEL0 = 0x00,
} radio_chan_id;

typedef enum {
	RADIO_FILTER_BASEBAND = 0x00,
} radio_filter_id;

typedef enum {
	RADIO_SAMPLE_RATE_CLOCKGEN = 0x00,
} radio_sample_rate_id;

typedef enum {
	RADIO_FREQUENCY_RF = 0x00,
	RADIO_FREQUENCY_IF = 0x01,
	RADIO_FREQUENCY_OF = 0x02,
} radio_frequency_id;

typedef enum {
	RADIO_GAIN_RF_AMP = 0x00,
	RADIO_GAIN_RX_LNA = 0x01,
	RADIO_GAIN_RX_VGA = 0x02,
	RADIO_GAIN_TX_VGA = 0x03,
} radio_gain_id;

typedef enum {
	RADIO_ANTENNA_BIAS_TEE = 0x00,
} radio_antenna_id;

typedef enum {
	RADIO_CLOCK_CLKIN = 0x00,
	RADIO_CLOCK_CLKOUT = 0x01,
} radio_clock_id;

typedef struct {
	uint32_t num;
	uint32_t div;
	uint32_t hz;
} radio_sample_rate_t;

typedef struct {
	uint64_t hz;
} radio_filter_t;

typedef struct {
	union {
		bool enable;
		uint8_t db;
	};
} radio_gain_t;

typedef struct {
	uint64_t hz;    // desired frequency
	uint64_t if_hz; // intermediate frequency
	uint64_t lo_hz; // front-end local oscillator frequency
	uint8_t path;   // image rejection filter path
} radio_frequency_t;

typedef struct {
	bool enable;
} radio_antenna_t;

typedef struct {
	bool enable;
} radio_clock_t;

// legacy type, moved from hackrf_core
typedef enum {
	CLOCK_SOURCE_HACKRF = 0,
	CLOCK_SOURCE_EXTERNAL = 1,
	CLOCK_SOURCE_PORTAPACK = 2,
} clock_source_t;

// legacy type, moved from usb_api_transceiver
typedef enum {
	TRANSCEIVER_MODE_OFF = 0,
	TRANSCEIVER_MODE_RX = 1,
	TRANSCEIVER_MODE_TX = 2,
	TRANSCEIVER_MODE_SS = 3,
	TRANSCEIVER_MODE_CPLD_UPDATE = 4,
	TRANSCEIVER_MODE_RX_SWEEP = 5,
} transceiver_mode_t;

#define RADIO_CHANNEL_COUNT     1
#define RADIO_SAMPLE_RATE_COUNT 1
#define RADIO_FILTER_COUNT      1
#define RADIO_FREQUENCY_COUNT   4
#define RADIO_GAIN_COUNT        4
#define RADIO_ANTENNA_COUNT     1
#define RADIO_CLOCK_COUNT       2
#define RADIO_MODE_COUNT        6

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
	bool trigger_enable;

	// currently active transceiver mode
	transceiver_mode_t mode;

#ifdef PRALINE
	// resampling ratio is 2**n
	uint8_t resampling_n;

	// quarter-rate shift configuration for offset tuning
	fpga_quarter_shift_mode_t shift;
#endif

} radio_config_t;

typedef struct radio_channel_t {
	radio_chan_id id;
	radio_config_t config;
	radio_config_t mode[RADIO_MODE_COUNT];
	clock_source_t clock_source;
} radio_channel_t;

typedef struct radio_t {
	radio_channel_t channel[RADIO_CHANNEL_COUNT];
} radio_t;

/**
 * API Notes
 *
 * - All radio_set_*() functions return a radio_error_t
 * - radio_set_*() functions work as follows:
 *   - if the channel mode is TRANSCEIVER_MODE_OFF only the configuration will
 *     be updated, the hardware state will remain unaffected.
 *   - if the channel is something other than TRANSCEIVER_MODE_OFF both the
 *     configuration and hardware state will be updated.
 * - this makes it possible to maintain multiple channel configurations and
 *   switch between them with a single call to radio_switch_mode()
 */

radio_error_t radio_set_sample_rate(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_sample_rate_id element,
	radio_sample_rate_t sample_rate);
radio_sample_rate_t radio_get_sample_rate(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_sample_rate_id element);

radio_error_t radio_set_filter(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_filter_id element,
	radio_filter_t filter);
radio_filter_t radio_get_filter(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_filter_id element);

radio_error_t radio_set_frequency(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_frequency_id element,
	radio_frequency_t frequency);
radio_frequency_t radio_get_frequency(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_frequency_id element);

radio_error_t radio_set_gain(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_gain_id element,
	radio_gain_t gain);
radio_gain_t radio_get_gain(radio_t* radio, radio_chan_id chan_id, radio_gain_id element);

radio_error_t radio_set_antenna(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_antenna_id element,
	radio_antenna_t value);
radio_antenna_t radio_get_antenna(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_antenna_id element);

radio_error_t radio_set_clock(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_clock_id element,
	radio_clock_t value);
radio_clock_t radio_get_clock(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_clock_id element);

radio_error_t radio_set_trigger_enable(radio_t* radio, radio_chan_id chan_id, bool enable);
bool radio_get_trigger_enable(radio_t* radio, radio_chan_id chan_id);

transceiver_mode_t radio_get_mode(radio_t* radio, radio_chan_id chan_id);
rf_path_direction_t radio_get_direction(radio_t* radio, radio_chan_id chan_id);
clock_source_t radio_get_clock_source(radio_t* radio, radio_chan_id chan_id);

// apply the current channel configuration and switch to the given transceiver mode
radio_error_t radio_switch_mode(
	radio_t* radio,
	radio_chan_id chan_id,
	transceiver_mode_t mode);

#endif /*__RF_CONFIG_H__*/
