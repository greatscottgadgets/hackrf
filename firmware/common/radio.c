/*
 * Copyright 2025-2026 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include "hackrf_core.h"
#include "tuning.h"
#include "rf_path.h"
#include "fpga.h"
#include "platform_detect.h"
#include "radio.h"

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

static volatile uint32_t regs_dirty = 0;

void radio_init(radio_t* const radio)
{
	for (uint8_t bank = 0; bank < RADIO_NUM_BANKS; bank++) {
		for (uint8_t reg = 0; reg < RADIO_NUM_REGS; reg++) {
			radio->config[bank][reg] = RADIO_AUTO;
		}
	}
	radio->config[RADIO_BANK_IDLE][RADIO_OPMODE] = TRANSCEIVER_MODE_OFF;
	radio->config[RADIO_BANK_RX][RADIO_OPMODE] = TRANSCEIVER_MODE_RX;
	radio->config[RADIO_BANK_TX][RADIO_OPMODE] = TRANSCEIVER_MODE_TX;
	radio->active_bank = RADIO_BANK_IDLE;
}

static inline void mark_dirty(radio_register_t reg)
{
	regs_dirty |= (1 << reg);
}

radio_error_t radio_reg_write(
	radio_t* const radio,
	const radio_register_bank_t bank,
	const radio_register_t reg,
	const uint64_t value)
{
	if (reg > RADIO_NUM_REGS) {
		return RADIO_ERR_INVALID_REGISTER;
	}

	switch (bank) {
	case RADIO_BANK_ACTIVE:
		radio->config[radio->active_bank][reg] = value;
		mark_dirty(reg);
		break;
	case RADIO_BANK_IDLE:
	case RADIO_BANK_RX:
	case RADIO_BANK_TX:
		radio->config[bank][reg] = value;
		if (radio->active_bank == bank) {
			mark_dirty(reg);
		}
		break;
	case RADIO_BANK_ALL:
		for (uint8_t b = 1; b < RADIO_NUM_BANKS; b++) {
			radio->config[b][reg] = value;
		}
		mark_dirty(reg);
		break;
	default:
		return RADIO_ERR_INVALID_BANK;
	}

	radio_update(radio);
	return RADIO_OK;
}

uint64_t radio_reg_read(
	radio_t* const radio,
	const radio_register_bank_t bank,
	const radio_register_t reg)
{
	return radio->config[bank][reg];
}

static bool radio_update_direction(radio_t* const radio, uint64_t* tmp_bank)
{
	const uint64_t requested = tmp_bank[RADIO_OPMODE];

	if (requested == RADIO_AUTO) {
		/* Leave the active configuration unchanged. */
		return false;
	}

	rf_path_direction_t direction;
	switch (tmp_bank[RADIO_OPMODE]) {
	case TRANSCEIVER_MODE_TX:
	case TRANSCEIVER_MODE_SS:
		direction = RF_PATH_DIRECTION_TX;
		break;
	case TRANSCEIVER_MODE_RX:
	case TRANSCEIVER_MODE_RX_SWEEP:
		direction = RF_PATH_DIRECTION_RX;
		break;
	default:
		direction = RF_PATH_DIRECTION_OFF;
	}
	radio->config[RADIO_BANK_ACTIVE][RADIO_OPMODE] = requested;
	rf_path_set_direction(&rf_path, direction);
	return true;
}

/*
 * Convert sample rate in units of 1/(2**24) Hz to fraction.
 */
static inline uint64_t frac_sample_rate(const uint64_t rate)
{
	uint64_t num = rate;
	uint64_t denom = 1 << 24;
	while ((num & 1) == 0) {
		num >>= 1;
		denom >>= 1;
		if (denom == 1) {
			break;
		}
	}
	return (denom << 32) | (num & 0xffffffff);
}

static inline uint32_t numerator(const uint64_t frac)
{
	return frac & 0xffffffff;
}

static inline uint32_t denominator(const uint64_t frac)
{
	return frac >> 32;
}

/*
 * Convert fractional sample rate to units of 1/(2**24) Hz.
 */
static inline uint64_t round_sample_rate(const uint64_t frac)
{
	uint64_t num = (uint64_t) numerator(frac) << 24;
	uint32_t denom = denominator(frac);
	if (denom == 0) {
		denom = 1;
	}
	return (num + (denom >> 1)) / denom;
}

#define MAX_AFE_RATE_HZ (40000000UL)

static inline uint8_t compute_resample_log(
	const uint32_t sample_rate_hz,
	const uint64_t requested_n)
{
	if (detected_platform() != BOARD_ID_PRALINE) {
		return 0;
	}
	const uint8_t min_n = 1;
	uint8_t max_n = 5;

	if (requested_n != RADIO_AUTO) {
		max_n = MIN(max_n, requested_n);
	}
	uint8_t n = min_n; // resampling ratio is 2**n
	uint32_t afe_rate_x2 = 4 * sample_rate_hz;
	while ((afe_rate_x2 <= MAX_AFE_RATE_HZ) && (n < max_n)) {
		afe_rate_x2 <<= 1;
		n++;
	}
	return n;
}

#define MIN_MCU_RATE (200000ULL << 24)
#define MAX_MCU_RATE (21800000ULL << 24)

static bool radio_update_sample_rate(radio_t* const radio, uint64_t* tmp_bank)
{
	uint8_t n = 0;
	const uint64_t active_frac =
		radio->config[RADIO_BANK_ACTIVE][RADIO_SAMPLE_RATE_FRAC];
	const uint32_t active_rate = radio->config[RADIO_BANK_ACTIVE][RADIO_SAMPLE_RATE];

	const uint64_t requested_frac = tmp_bank[RADIO_SAMPLE_RATE_FRAC];
	const uint64_t requested_rate = tmp_bank[RADIO_SAMPLE_RATE];
	const uint64_t requested_n = tmp_bank[RADIO_RESAMPLE_LOG];

	uint64_t frac = active_frac;
	uint64_t rate = active_rate;
	if (requested_frac != RADIO_AUTO) {
		frac = requested_frac;
		if (round_sample_rate(frac) > MAX_MCU_RATE) {
			frac = frac_sample_rate(MAX_MCU_RATE);
		}
		if (round_sample_rate(frac) < MIN_MCU_RATE) {
			frac = frac_sample_rate(MIN_MCU_RATE);
		}
		rate = round_sample_rate(frac);
	} else if (requested_rate != RADIO_AUTO) {
		rate = MIN(requested_rate, MAX_MCU_RATE);
		rate = MAX(rate, MIN_MCU_RATE);
		frac = frac_sample_rate(rate);
	}

	/*
	 * Resampling is enabled only in RX mode to work around a spectrum
	 * inversion bug with TX interpolation.
	 */
	if ((radio->config[radio->active_bank][RADIO_OPMODE] == TRANSCEIVER_MODE_RX) ||
	    (radio->config[radio->active_bank][RADIO_OPMODE] ==
	     TRANSCEIVER_MODE_RX_SWEEP)) {
		n = compute_resample_log(rate >> 24, requested_n);
	}

	radio->config[RADIO_BANK_ACTIVE][RADIO_SAMPLE_RATE_FRAC] = frac;
	radio->config[RADIO_BANK_ACTIVE][RADIO_SAMPLE_RATE] = rate;
	radio->config[RADIO_BANK_ACTIVE][RADIO_RESAMPLE_LOG] = n;

#ifdef PRALINE
	switch (radio->config[radio->active_bank][RADIO_OPMODE]) {
	case TRANSCEIVER_MODE_TX:
	case TRANSCEIVER_MODE_SS:
		fpga_set_tx_interpolation_ratio(&fpga, n);
		break;
	default:
		fpga_set_rx_decimation_ratio(&fpga, n);
	}
#endif

	sample_rate_frac_set(numerator(frac) << n, denominator(frac));
	return true;
}

static bool radio_update_frequency(radio_t* const radio, uint64_t* tmp_bank)
{
	const uint64_t requested_rf = tmp_bank[RADIO_FREQUENCY_RF];
	const uint64_t requested_if = tmp_bank[RADIO_FREQUENCY_IF];
	const uint64_t requested_lo = tmp_bank[RADIO_FREQUENCY_LO];
	const uint64_t requested_img_reject = tmp_bank[RADIO_IMAGE_REJECT];

	bool auto_if = (requested_if == RADIO_AUTO);
	bool auto_lo = (requested_lo == RADIO_AUTO);
	bool auto_img_reject = (requested_img_reject == RADIO_AUTO);

	if (!auto_if && !auto_lo && !auto_img_reject) {
		radio->config[RADIO_BANK_ACTIVE][RADIO_FREQUENCY_IF] = requested_if;
		radio->config[RADIO_BANK_ACTIVE][RADIO_FREQUENCY_LO] = requested_lo;
		radio->config[RADIO_BANK_ACTIVE][RADIO_IMAGE_REJECT] =
			requested_img_reject;
		radio->config[RADIO_BANK_ACTIVE][RADIO_FREQUENCY_RF] = RADIO_AUTO;
		set_freq_explicit(
			requested_if >> 24,
			requested_lo >> 24,
			requested_img_reject);
#ifdef PRALINE
		const uint64_t requested_rotation = tmp_bank[RADIO_ROTATION];
		uint8_t rotation = requested_rotation;
		if (requested_rotation == RADIO_AUTO) {
			rotation = 0;
		}
		radio->config[RADIO_BANK_ACTIVE][RADIO_ROTATION] = rotation;
		fpga_set_rx_quarter_shift_mode(&fpga, rotation >> 6);
#endif
		return true;
	}

	if (requested_rf == RADIO_AUTO) {
		/* Leave the active configuration unchanged. */
		return false;
	}

	radio->config[RADIO_BANK_ACTIVE][RADIO_FREQUENCY_RF] = requested_rf;
	radio->config[RADIO_BANK_ACTIVE][RADIO_FREQUENCY_IF] = RADIO_AUTO;
	radio->config[RADIO_BANK_ACTIVE][RADIO_FREQUENCY_LO] = RADIO_AUTO;
	radio->config[RADIO_BANK_ACTIVE][RADIO_IMAGE_REJECT] = RADIO_AUTO;
	uint64_t requested_rf_hz = requested_rf >> 24;
#ifdef PRALINE
	const tune_config_t* tune_config;
	switch (radio->config[radio->active_bank][RADIO_OPMODE]) {
	case TRANSCEIVER_MODE_TX:
	case TRANSCEIVER_MODE_SS:
		tune_config = praline_tune_config_tx;
		break;
	case TRANSCEIVER_MODE_RX_SWEEP:
		tune_config = praline_tune_config_rx_sweep;
		break;
	default:
		tune_config = praline_tune_config_rx;
	}
	for (; (tune_config->rf_range_end_mhz != 0) || (tune_config->if_mhz != 0);
	     tune_config++) {
		if ((requested_rf_hz == 0) ||
		    (tune_config->rf_range_end_mhz > (requested_rf_hz / FREQ_ONE_MHZ))) {
			break;
		}
	}
	radio->config[RADIO_BANK_ACTIVE][RADIO_ROTATION] = tune_config->shift << 6;
	fpga_set_rx_quarter_shift_mode(&fpga, tune_config->shift);
	uint32_t offset_hz = ((radio->config[RADIO_BANK_ACTIVE][RADIO_SAMPLE_RATE] >> 24)
			      << radio->config[RADIO_BANK_ACTIVE][RADIO_RESAMPLE_LOG]) /
		4;
	tuning_set_frequency(tune_config, requested_rf_hz, offset_hz);
#else
	set_freq(requested_rf_hz);
#endif
	return true;
}

static bool radio_update_bandwidth(radio_t* const radio, uint64_t* tmp_bank)
{
#ifdef PRALINE
	/* Praline legacy mode always sets baseband bandwidth automatically. */
	(void) tmp_bank;
	const uint32_t bb_bandwidth =
		((radio->config[RADIO_BANK_ACTIVE][RADIO_SAMPLE_RATE] >> 24) * 3) / 4;
	const uint8_t rotation = radio->config[RADIO_BANK_ACTIVE][RADIO_ROTATION];
	uint32_t offset = 0;
	if (rotation != 0) {
		const uint32_t afe_rate =
			(radio->config[RADIO_BANK_ACTIVE][RADIO_SAMPLE_RATE] >> 24)
			<< radio->config[RADIO_BANK_ACTIVE][RADIO_RESAMPLE_LOG];
		offset = afe_rate / 4;
	}
	const uint32_t lpf_bandwidth = bb_bandwidth + offset * 2;

	if (radio->config[RADIO_BANK_ACTIVE][RADIO_BB_BANDWIDTH] == bb_bandwidth) {
		return false;
	}
	radio->config[RADIO_BANK_ACTIVE][RADIO_BB_BANDWIDTH] = bb_bandwidth;
	max2831_set_lpf_bandwidth(&max283x, lpf_bandwidth);
#else
	uint64_t requested = tmp_bank[RADIO_BB_BANDWIDTH];

	if (radio->config[RADIO_BANK_ACTIVE][RADIO_BB_BANDWIDTH] == requested) {
		return false;
	}
	radio->config[RADIO_BANK_ACTIVE][RADIO_BB_BANDWIDTH] = requested;
	max283x_set_lpf_bandwidth(
		&max283x,
		radio->config[RADIO_BANK_ACTIVE][RADIO_BB_BANDWIDTH]);
#endif
	return true;
}

static bool radio_update_gain(radio_t* const radio, uint64_t* tmp_bank)
{
	const uint64_t requested_gain_rf = tmp_bank[RADIO_GAIN_RF];
	const uint64_t requested_gain_rx_if = tmp_bank[RADIO_GAIN_RX_IF];
	const uint64_t requested_gain_tx_if = tmp_bank[RADIO_GAIN_TX_IF];
	const uint64_t requested_gain_rx_bb = tmp_bank[RADIO_GAIN_RX_BB];

	/* RADIO_AUTO is interpreted as maximum gain. */
	radio->config[RADIO_BANK_ACTIVE][RADIO_GAIN_RF] = requested_gain_rf;
	radio->config[RADIO_BANK_ACTIVE][RADIO_GAIN_RX_IF] = requested_gain_rx_if;
	radio->config[RADIO_BANK_ACTIVE][RADIO_GAIN_TX_IF] = requested_gain_tx_if;
	radio->config[RADIO_BANK_ACTIVE][RADIO_GAIN_RX_BB] = requested_gain_rx_bb;

	rf_path_set_lna(&rf_path, requested_gain_rf);
#ifdef PRALINE
	max2831_set_txvga_gain(&max283x, requested_gain_tx_if);
	max2831_set_lna_gain(&max283x, requested_gain_rx_if);
	max2831_set_vga_gain(&max283x, requested_gain_rx_bb);
#else
	max283x_set_txvga_gain(&max283x, requested_gain_tx_if);
	max283x_set_lna_gain(&max283x, requested_gain_rx_if);
	max283x_set_vga_gain(&max283x, requested_gain_rx_bb);
#endif
	return true;
}

static bool radio_update_bias_tee(radio_t* const radio, uint64_t* tmp_bank)
{
	if (detected_platform() == BOARD_ID_JAWBREAKER) {
		return false;
	}

	const uint64_t requested = tmp_bank[RADIO_BIAS_TEE];
	const bool enable = requested;

	if (requested == RADIO_AUTO) {
		/* Leave the active configuration unchanged. */
		return false;
	}

	radio->config[RADIO_BANK_ACTIVE][RADIO_BIAS_TEE] = enable;
	rf_path_set_antenna(&rf_path, requested);
	return true;
}

static bool radio_update_trigger(radio_t* const radio, uint64_t* tmp_bank)
{
	const uint64_t requested = tmp_bank[RADIO_TRIGGER];
	bool enable = requested;

	/* RADIO_AUTO is interpreted as false. */
	if (requested == RADIO_AUTO) {
		enable = false;
	}

	radio->config[RADIO_BANK_ACTIVE][RADIO_TRIGGER] = enable;
	trigger_enable(enable);
	return true;
}

static bool radio_update_dc_block(radio_t* const radio, uint64_t* tmp_bank)
{
#ifndef PRALINE
	(void) radio;
	(void) tmp_bank;
	return false;
#else

	/* RADIO_AUTO is interpreted as true. */
	const bool enable = tmp_bank[RADIO_DC_BLOCK];

	radio->config[RADIO_BANK_ACTIVE][RADIO_DC_BLOCK] = enable;
	fpga_set_rx_dc_block_enable(&fpga, enable);
	return true;
#endif
}

bool radio_update(radio_t* const radio)
{
	uint64_t tmp_bank[RADIO_NUM_REGS];
	uint32_t dirty = regs_dirty;
	if (dirty == 0) {
		return false;
	}
	regs_dirty = 0;
	memcpy(&tmp_bank[0], &(radio->config[radio->active_bank][0]), sizeof(tmp_bank));

	bool dir = false;
	bool rate = false;
	bool freq = false;
	bool bw = false;
	bool gain = false;
	bool bias = false;
	bool trig = false;
	bool dc = false;

	if ((dirty &
	     ((1 << RADIO_SAMPLE_RATE) | (1 << RADIO_SAMPLE_RATE_FRAC) |
	      (1 << RADIO_RESAMPLE_LOG))) ||
	    ((detected_platform() == BOARD_ID_PRALINE) &&
	     (dirty & (1 << RADIO_OPMODE)))) {
		rate = radio_update_sample_rate(radio, &tmp_bank[0]);
	}
	if ((dirty &
	     ((1 << RADIO_FREQUENCY_RF) | (1 << RADIO_FREQUENCY_IF) |
	      (1 << RADIO_FREQUENCY_LO) | (1 << RADIO_IMAGE_REJECT) |
	      (1 << RADIO_ROTATION))) ||
	    ((detected_platform() == BOARD_ID_PRALINE) &&
	     (rate || (dirty & (1 << RADIO_OPMODE))))) {
		freq = radio_update_frequency(radio, &tmp_bank[0]);
	}
	if ((dirty &
	     ((1 << RADIO_BB_BANDWIDTH) | (1 << RADIO_XCVR_RX_LPF) |
	      (1 << RADIO_XCVR_TX_LPF) | (1 << RADIO_XCVR_RX_HPF) |
	      (1 << RADIO_RX_NARROW_LPF))) ||
	    ((detected_platform() == BOARD_ID_PRALINE) && (rate || freq))) {
		bw = radio_update_bandwidth(radio, &tmp_bank[0]);
	}
	if (dirty &
	    ((1 << RADIO_GAIN_RF) | (1 << RADIO_GAIN_RX_IF) | (1 << RADIO_GAIN_TX_IF) |
	     (1 << RADIO_GAIN_RX_BB) | (1 << RADIO_OPMODE))) {
		gain = radio_update_gain(radio, &tmp_bank[0]);
	}
	if (dirty & (1 << RADIO_BIAS_TEE)) {
		bias = radio_update_bias_tee(radio, &tmp_bank[0]);
	}
	if (dirty & (1 << RADIO_TRIGGER)) {
		trig = radio_update_trigger(radio, &tmp_bank[0]);
	}
	if (dirty & (1 << RADIO_DC_BLOCK)) {
		dc = radio_update_dc_block(radio, &tmp_bank[0]);
	}
	if (dirty & (1 << RADIO_OPMODE)) {
		dir = radio_update_direction(radio, &tmp_bank[0]);
	}

	return trig || dir || rate || freq || bw || gain || bias || dc;
}

void radio_switch_mode(radio_t* const radio, const transceiver_mode_t mode)
{
	switch (mode) {
	case TRANSCEIVER_MODE_RX_SWEEP:
	case TRANSCEIVER_MODE_RX:
		radio->active_bank = RADIO_BANK_RX;
		break;
	case TRANSCEIVER_MODE_TX:
	case TRANSCEIVER_MODE_SS:
		radio->active_bank = RADIO_BANK_TX;
		break;
	default:
		radio->active_bank = RADIO_BANK_IDLE;
	}

	mark_dirty(RADIO_OPMODE);
	radio_update(radio);
}
