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

#ifndef __RF_CONFIG_H__
#define __RF_CONFIG_H__

#include <stdint.h>
#include <stdbool.h>

#include "fixed_point.h"

typedef enum {
	RADIO_OK = 1,
	RADIO_ERR_INVALID_PARAM = -2,
	RADIO_ERR_INVALID_BANK = -3,
	RADIO_ERR_INVALID_REGISTER = -4,
	RADIO_ERR_UNSUPPORTED_OPERATION = -10,
	RADIO_ERR_UNIMPLEMENTED = -19,
	RADIO_ERR_OTHER = -9999,
} radio_error_t;

/* radio configuration modes */
typedef enum {
	RADIO_CONFIG_LEGACY = 0,
	RADIO_CONFIG_STANDARD = 1,
	RADIO_CONFIG_EXT_PRECISION_RX = 2,
	RADIO_CONFIG_EXT_PRECISION_TX = 3,
	RADIO_CONFIG_HALF_PRECISION = 4,
} radio_config_mode_t;

typedef struct {
	uint32_t num;
	uint32_t div;
	uint32_t hz;
} radio_sample_rate_t;

// legacy type, moved from hackrf_core
typedef enum {
	CLOCK_SOURCE_HACKRF = 0,
	CLOCK_SOURCE_EXTERNAL = 1,
	CLOCK_SOURCE_PORTAPACK = 2,
} clock_source_t;

typedef enum {
	TRANSCEIVER_MODE_OFF = 0,
	TRANSCEIVER_MODE_RX = 1,
	TRANSCEIVER_MODE_TX = 2,
	TRANSCEIVER_MODE_SS = 3,
	TRANSCEIVER_MODE_CPLD_UPDATE = 4,
	TRANSCEIVER_MODE_RX_SWEEP = 5,
} transceiver_mode_t;

/**
 * Configurable registers stored as uint64_t. Any register may be set to
 * RADIO_UNSET. When not RADIO_UNSET, some registers are read as a specific type
 * as noted.
 */
typedef enum {
	/**
	 * Radio transceiver operating mode of type transceiver_mode_t.
	 */
	RADIO_OPMODE = 0,
	/**
	 * Center frequency (as seen by MCU/host) in 1/(2**24) Hz.
	 */
	RADIO_FREQUENCY_RF = 1,
	/**
	 * Intermediate frequency in 1/(2**24) Hz that the quadrature
	 * transceiver is tuned to. Overrides RADIO_FREQUENCY_RF if specified.
	 */
	RADIO_FREQUENCY_IF = 2,
	/**
	 * Local oscillator frequency in 1/(2**24) Hz of the front-end mixer.
	 * Overrides RADIO_FREQUENCY_RF if specified.
	 */
	RADIO_FREQUENCY_LO = 3,
	/**
	 * Image reject path of type rf_path_filter_t. Overrides
	 * RADIO_FREQUENCY_RF if specified.
	 */
	RADIO_IMAGE_REJECT = 4,
	/**
	 * Digital frequency conversion of type uint8_t in tau/256 steps with
	 * respect to AFE clock.
	 */
	RADIO_ROTATION = 5,
	/**
	 * Sample rate (as seen by MCU/host) in 1/(2**24) Hz.
	 */
	RADIO_SAMPLE_RATE = 6,
	/**
	 * Base two logarithm of TX decimation ratio (0 means a ratio of 1).
	 */
	RADIO_RESAMPLE_TX = 7,
	/**
	 * Base two logarithm of RX decimation ratio (0 means a ratio of 1).
	 */
	RADIO_RESAMPLE_RX = 8,
	/**
	 * TX RF amplifier enable of type bool.
	 */
	RADIO_GAIN_TX_RF = 9,
	/**
	 * TX IF amplifier gain in dB.
	 */
	RADIO_GAIN_TX_IF = 10,
	/**
	 * RX RF amplifier enable of type bool.
	 */
	RADIO_GAIN_RX_RF = 11,
	/**
	 * RX IF amplifier gain in dB.
	 */
	RADIO_GAIN_RX_IF = 12,
	/**
	 * RX baseband amplifier gain in dB.
	 */
	RADIO_GAIN_RX_BB = 13,
	/**
	 * TX baseband bandwidth in Hz. This controls analog baseband filter
	 * settings but is specified as the desired bandwidth centered in
	 * digital baseband as seen by the MCU/host.
	 */
	RADIO_BB_BANDWIDTH_TX = 14,
	/**
	 * RX baseband bandwidth in Hz. This controls analog baseband filter
	 * settings but is specified as the desired bandwidth centered in
	 * digital baseband as seen by the MCU/host.
	 */
	RADIO_BB_BANDWIDTH_RX = 15,
	/**
	 * Quadrature transceiver TX baseband LPF bandwidth in Hz.  If no
	 * rotation is performed, this is set to match RADIO_BB_BANDWIDTH.
	 * Currently unused.
	 */
	RADIO_XCVR_TX_LPF = 16,
	/**
	 * Quadrature transceiver RX baseband LPF bandwidth in Hz.  If no
	 * rotation is performed, this is set to match RADIO_BB_BANDWIDTH.
	 * Currently unused.
	 */
	RADIO_XCVR_RX_LPF = 17,
	/**
	 * Quadrature transceiver RX baseband HPF bandwidth in Hz. Currently
	 * unused.
	 */
	RADIO_XCVR_RX_HPF = 18,
	/**
	 * Narrowband RX analog baseband LPF enable of type bool. Currently unused.
	 */
	RADIO_RX_NARROW_LPF = 19,
	/**
	 * RF port bias tee enable of type bool.
	 */
	RADIO_BIAS_TEE = 20,
	/**
	 * Trigger input enable of type bool.
	 */
	RADIO_TRIGGER = 21,
	/**
	 * DC block enable of type bool.
	 */
	RADIO_DC_BLOCK = 22,
} radio_register_t;

#define RADIO_NUM_REGS (23)
#define RADIO_UNSET    (0xffffffffffffffff)

/**
 * Register bank RADIO_BANK_ACTIVE stores the active configuration. Active
 * register settings are copied to the applied register when applied.
 *
 * The other three banks store settings that will be applied when switching to
 * that operating mode.
 *
 * RADIO_BANK_ALL may be used to write to all banks at once except for
 * RADIO_BANK_APPLIED.
 */
typedef enum {
	RADIO_BANK_APPLIED = 0,
	RADIO_BANK_ACTIVE = 1,
	RADIO_BANK_IDLE = 2,
	RADIO_BANK_RX = 3,
	RADIO_BANK_TX = 4,
	RADIO_BANK_ALL = 255,
} radio_register_bank_t;

#define RADIO_NUM_BANKS (5)

/**
 * A callback function must be provided that configures clock generation to
 * produce the requested sample clock frequency. The function must return the
 * configured sample rate. A boolean program argument may be set to false to
 * execute a dry run, returning the sample rate without configuring clock
 * generation.
 */
typedef fp_40_24_t (*sample_rate_fn)(const fp_40_24_t sample_rate, const bool program);

typedef struct radio_t {
	radio_config_mode_t config_mode;
	uint64_t config[RADIO_NUM_BANKS][RADIO_NUM_REGS];
	volatile uint32_t regs_dirty;
	sample_rate_fn sample_rate_cb;
} radio_t;

void radio_init(radio_t* const radio);

/**
 * Write to one or more registers. Writes to RADIO_BANK_ACTIVE are applied at
 * the next radio_update(). Writes to RADIO_BANK_APPLIED are not supported.
 */
radio_error_t radio_reg_write(
	radio_t* const radio,
	const radio_register_bank_t bank,
	const radio_register_t reg,
	const uint64_t value);

/**
 * Read from a register.
 */
uint64_t radio_reg_read(
	radio_t* const radio,
	const radio_register_bank_t bank,
	const radio_register_t reg);

/**
 * Apply changes requested in RADIO_BANK_ACTIVE.
 * Return true if any changes were applied.
 */
bool radio_update(radio_t* const radio);

/**
 * Switch to a new operating mode and apply complete configuration stored in
 * the request bank for the new mode.
 */
void radio_switch_opmode(radio_t* const radio, const transceiver_mode_t mode);

#endif /*__RF_CONFIG_H__*/
