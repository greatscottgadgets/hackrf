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

#ifndef __RADIO_TYPES_H__
#define __RADIO_TYPES_H__

typedef enum {
	RADIO_OK = 1,
	RADIO_ERR_INVALID_CHANNEL = -1,
	RADIO_ERR_INVALID_MODE = -2,
	RADIO_ERR_INVALID_ELEMENT = -3,
	RADIO_ERR_INVALID_PARAM = -4,
	RADIO_ERR_INVALID_CONFIG = -5,
	RADIO_ERR_UNSUPPORTED_OPERATION = -10,
	RADIO_ERR_UNIMPLEMENTED = -19,
	RADIO_ERR_OTHER = -9999,
} radio_error_t;

typedef enum {
	RADIO_CHANNEL0 = 0x00,
} radio_chan_id;

/**
 * Sample rate element identifiers.
 *
 * Used by @ref hackrf_get_sample_rate, to interrogate their current configuration.
 */
typedef enum {
	RADIO_SAMPLE_RATE = 0x00,                     // TODO do we want a 'top-level' ?
	RADIO_SAMPLE_RATE_MCU = 0x01,                 // s/clockgen/mcu
	RADIO_SAMPLE_RATE_RX_DECIMATION_RATIO = 0x02, // 3 bits, 2^(0-7)
	RADIO_SAMPLE_RATE_TX_INTERPOLATION_RATIO = 0x03, // 3 bits, 2^(0-7)
} radio_sample_rate_id;

/**
 * Filter element identifiers.
 *
 * Used by @ref hackrf_supported_filter_element_bandwidths, to interrogate their supported bandwidths.
 * Used by @ref hackrf_get_filter, to interrogate their current configuration.
 *
 * **Note**: @ref RADIO_FILTER_RX_NARROWBAND_AA is only available on HackRF Pro hardware.
 */
typedef enum {
	RADIO_FILTER_BASEBAND_LPF = 0x00,      // radio_range_t
	RADIO_FILTER_RF_PATH = 0x01,           // BYPASS, LOW_PASS, HIGH_PASS
	RADIO_FILTER_RX_BASEBAND_HPF = 0x02,   // 100Hz, 4kHz, 30kHz, 600kHz
	RADIO_FILTER_RX_NARROWBAND_LPF = 0x03, // on/off
	RADIO_FILTER_RX_DC_BLOCK = 0x04,       // on/off
} radio_filter_id;

/**
 * Frequency element identifiers.
 *
 * Used by @ref hackrf_get_frequency, to interrogate their current configuration.
 */
typedef enum {
	RADIO_FREQUENCY = 0x00, // TODO do we want a 'top-level' ?
	RADIO_FREQUENCY_IF = 0x01,
	RADIO_FREQUENCY_LO = 0x02,
	RADIO_FREQUENCY_RX_QUARTER_SHIFT = 0x03, // 2 bit register - auto/high/low/middle
} radio_frequency_id;

/**
 * Gain element identifiers.
 *
 * Used by @ref hackrf_get_gain, to interrogate their current configuration.
 */
typedef enum {
	RADIO_GAIN_RF_AMP = 0x00,
	RADIO_GAIN_RX_LNA = 0x01,
	RADIO_GAIN_RX_VGA = 0x02,
	RADIO_GAIN_TX_VGA = 0x03,
} radio_gain_id;

/**
 * Antenna element identifiers.
 *
 * Used by @ref hackrf_get_antenna, to interrogate their current configuration.
 */
typedef enum {
	RADIO_ANTENNA_BIAS_TEE = 0x00,
} radio_antenna_id;

/**
 * Clock element identifiers.
 */
typedef enum {
	RADIO_CLOCK_CLKIN = 0x00,
	RADIO_CLOCK_CLKOUT = 0x01,
} radio_clock_id;

/**
 * Transceiver direction.
 */
typedef enum {
	RADIO_DIRECTION_NONE = 0,
	RADIO_DIRECTION_RX = 1,
	RADIO_DIRECTION_TX = 2,
} radio_direction_t;

/**
 * Operation mode.
 */
typedef enum {
	// These currently correspond to the existing usb_transceiver_mode_t values.
	// TODO they need to be consecutive and we need a from_usb_transceiver_mode() helper.
	RADIO_MODE_OFF = 0,
	RADIO_MODE_RX = 1,
	RADIO_MODE_TX = 2,
	RADIO_MODE_RX_SWEEP = 5,
	RADIO_MODE_ACTIVE = 254,
	RADIO_MODE_ALL = 255,
} radio_mode_t;

/**
 * Bitstreams
 */
typedef enum {
	RADIO_BITSTREAM_NONE = 0,
	RADIO_BITSTREAM_STANDARD = 1,
	RADIO_BITSTREAM_HALF_PRECISION = 2,
	RADIO_BITSTREAM_EXTENDED_PRECISION_RX = 3,
	RADIO_BITSTREAM_EXTENDED_PRECISION_TX = 4,
} radio_bitstream_t;

/**
 * A range, of sorts.
 */
typedef struct {
	uint64_t start;
	uint64_t end;
} radio_range_t;

/**
 * Sample rate configuration.
 */
typedef struct {
	uint32_t num;
	uint32_t denom;
} radio_sample_rate_t;

/**
 * Filter configuration.
 */
typedef struct {
	union {
		bool enable;
		uint32_t hz;
		uint8_t mode;
	};
} radio_filter_t;

typedef struct {
	uint32_t* hz;
	uint32_t length;
} radio_frequency_list_t;

/**
 * Gain configuration.
 */
typedef struct {
	union {
		bool enable;
		uint8_t db;
	};
} radio_gain_t;

/**
 * Frequency configuration.
 */
typedef struct {
	union {
		uint64_t hz;
		uint8_t mode;
	};
} radio_frequency_t;

/**
 * Antenna configuration.
 */
typedef struct {
	bool enable;
} radio_antenna_t;

/**
 * Clock configuration.
 */
typedef struct {
	bool enable;
} radio_clock_t;

#endif /*__RADIO_TYPES_H__*/
