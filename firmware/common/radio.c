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

#include <libopencm3/dispatch/nvic.h>

#include "hackrf_core.h"
#include "tuning.h"
#include "rf_path.h"
#include "fpga.h"
#include "platform_detect.h"
#include "radio.h"
#include "fixed_point.h"
#include "hackrf_ui.h"

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

void radio_init(radio_t* const radio)
{
	for (uint8_t bank = 0; bank < RADIO_NUM_BANKS; bank++) {
		for (uint8_t reg = 0; reg < RADIO_NUM_REGS; reg++) {
			radio->config[bank][reg] = RADIO_UNSET;
		}
	}
	radio->config[RADIO_BANK_APPLIED][RADIO_OPMODE] = TRANSCEIVER_MODE_OFF;
	radio->config[RADIO_BANK_ACTIVE][RADIO_OPMODE] = TRANSCEIVER_MODE_OFF;
	radio->config[RADIO_BANK_IDLE][RADIO_OPMODE] = TRANSCEIVER_MODE_OFF;
	radio->config[RADIO_BANK_RX][RADIO_OPMODE] = TRANSCEIVER_MODE_RX;
	radio->config[RADIO_BANK_TX][RADIO_OPMODE] = TRANSCEIVER_MODE_TX;
	radio->regs_dirty = 0;
}

static inline void mark_dirty(radio_t* const radio, radio_register_t reg)
{
	radio->regs_dirty |= (1 << reg);
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
		mark_dirty(radio, reg);
		/* fall through */
	case RADIO_BANK_IDLE:
	case RADIO_BANK_RX:
	case RADIO_BANK_TX:
		radio->config[bank][reg] = value;
		break;
	case RADIO_BANK_ALL:
		for (uint8_t b = 1; b < RADIO_NUM_BANKS; b++) {
			radio->config[b][reg] = value;
		}
		mark_dirty(radio, reg);
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

static bool radio_update_direction(radio_t* const radio, uint64_t* bank)
{
	const uint64_t requested = bank[RADIO_OPMODE];

	if (requested == RADIO_UNSET) {
		return false;
	}

	rf_path_direction_t direction;
	switch (bank[RADIO_OPMODE]) {
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
	radio->config[RADIO_BANK_APPLIED][RADIO_OPMODE] = requested;
	rf_path_set_direction(&rf_path, direction);
	return true;
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

	if (requested_n != RADIO_UNSET) {
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

#define MIN_MCU_RATE     (200000ULL * FP_ONE_HZ)
#define MAX_MCU_RATE     (21800000ULL * FP_ONE_HZ)
#define DEFAULT_MCU_RATE (10000000ULL * FP_ONE_HZ)

static fp_40_24_t applied_afe_rate = RADIO_UNSET;

static bool radio_update_sample_rate(radio_t* const radio, uint64_t* bank)
{
	fp_40_24_t rate;
	uint64_t previous_n;
	uint8_t n = 0;
	bool new_afe_rate = false;
	bool new_rate = false;
	bool new_n = false;

	const uint64_t requested_rate = bank[RADIO_SAMPLE_RATE];
	const uint64_t requested_n = bank[RADIO_RESAMPLE_RX];

	if (requested_rate != RADIO_UNSET) {
		rate = MIN(requested_rate, MAX_MCU_RATE);
		rate = MAX(rate, MIN_MCU_RATE);
	} else {
		rate = radio->config[RADIO_BANK_APPLIED][RADIO_SAMPLE_RATE];
		if (rate == RADIO_UNSET) {
			rate = DEFAULT_MCU_RATE;
		}
	}

	const uint64_t previous_opmode = radio->config[RADIO_BANK_APPLIED][RADIO_OPMODE];
	uint64_t opmode = bank[RADIO_OPMODE];
	if (opmode == RADIO_UNSET) {
		opmode = previous_opmode;
	}
	switch (previous_opmode) {
	case TRANSCEIVER_MODE_TX:
	case TRANSCEIVER_MODE_SS:
		previous_n = radio->config[RADIO_BANK_APPLIED][RADIO_RESAMPLE_TX];
		break;
	default:
		previous_n = radio->config[RADIO_BANK_APPLIED][RADIO_RESAMPLE_RX];
	}
	switch (opmode) {
	case TRANSCEIVER_MODE_TX:
	case TRANSCEIVER_MODE_SS:
		if (n != radio->config[RADIO_BANK_APPLIED][RADIO_RESAMPLE_TX]) {
#ifdef PRALINE
			fpga_set_tx_interpolation_ratio(&fpga, n);
#endif
			radio->config[RADIO_BANK_APPLIED][RADIO_RESAMPLE_TX] = n;
		}
		break;
	default:
		/*
		 * Resampling is enabled only in RX mode to work around a
		 * spectrum inversion bug with TX interpolation.
		 */
		n = compute_resample_log(rate / FP_ONE_HZ, requested_n);
		if (n != radio->config[RADIO_BANK_APPLIED][RADIO_RESAMPLE_RX]) {
#ifdef PRALINE
			fpga_set_rx_decimation_ratio(&fpga, n);
#endif
			radio->config[RADIO_BANK_APPLIED][RADIO_RESAMPLE_RX] = n;
		}
	}
	new_n = (n != previous_n);

	if (radio->sample_rate_cb) {
		fp_40_24_t afe_rate = rate << n;
		afe_rate = radio->sample_rate_cb(afe_rate, false);
		new_afe_rate = (afe_rate != applied_afe_rate);
		if (new_afe_rate) {
			afe_rate = radio->sample_rate_cb(afe_rate, true);
			applied_afe_rate = afe_rate;
			rate = afe_rate >> n;
		}
	} else {
		rate = RADIO_UNSET;
	}
	new_rate = (rate != radio->config[RADIO_BANK_APPLIED][RADIO_SAMPLE_RATE]);
	if (new_rate) {
		radio->config[RADIO_BANK_APPLIED][RADIO_SAMPLE_RATE] = rate;
		if (rate != RADIO_UNSET) {
			/* Round to the nearest Hz for display. */
			const uint32_t rate_hz = (rate + (FP_ONE_HZ >> 1)) / FP_ONE_HZ;
			hackrf_ui()->set_sample_rate(rate_hz);
		}
	}

	return (new_afe_rate || new_rate || new_n);
}

#define DEFAULT_RF (2450ULL * FP_ONE_MHZ)

static uint64_t applied_offset = RADIO_UNSET;

#ifdef PRALINE
static const tune_config_t* select_tune_config(uint64_t opmode)
{
	switch (opmode) {
	case TRANSCEIVER_MODE_TX:
	case TRANSCEIVER_MODE_SS:
		return praline_tune_config_tx;
	case TRANSCEIVER_MODE_RX_SWEEP:
		return praline_tune_config_rx_sweep;
	default:
		return praline_tune_config_rx;
	}
}
#endif

static bool radio_update_frequency(radio_t* const radio, uint64_t* bank)
{
	uint8_t rotation = 0;
	uint64_t requested_rf = bank[RADIO_FREQUENCY_RF];
	const uint64_t requested_if = bank[RADIO_FREQUENCY_IF];
	const uint64_t requested_lo = bank[RADIO_FREQUENCY_LO];
	const uint64_t requested_img_reject = bank[RADIO_IMAGE_REJECT];

	bool set_if = (requested_if != RADIO_UNSET);
	bool set_lo = (requested_lo != RADIO_UNSET);
	bool set_img_reject = (requested_img_reject != RADIO_UNSET);

	if (set_if && set_lo && set_img_reject) {
		bool new_if =
			(requested_if !=
			 radio->config[RADIO_BANK_APPLIED][RADIO_FREQUENCY_IF]);
		bool new_lo =
			(requested_lo !=
			 radio->config[RADIO_BANK_APPLIED][RADIO_FREQUENCY_LO]);
		bool new_img_reject =
			(requested_img_reject !=
			 radio->config[RADIO_BANK_APPLIED][RADIO_IMAGE_REJECT]);
		if (new_if || new_lo || new_img_reject) {
			set_freq_explicit(
				requested_if / FP_ONE_HZ,
				requested_lo / FP_ONE_HZ,
				requested_img_reject);
			radio->config[RADIO_BANK_APPLIED][RADIO_FREQUENCY_IF] =
				requested_if;
			radio->config[RADIO_BANK_APPLIED][RADIO_FREQUENCY_LO] =
				requested_lo;
			radio->config[RADIO_BANK_APPLIED][RADIO_IMAGE_REJECT] =
				requested_img_reject;
			radio->config[RADIO_BANK_APPLIED][RADIO_FREQUENCY_RF] =
				RADIO_UNSET;
		}
#ifdef PRALINE
		const uint64_t requested_rotation = bank[RADIO_ROTATION];
		if (requested_rotation != RADIO_UNSET) {
			rotation = requested_rotation;
		} else if (radio->config[RADIO_BANK_APPLIED][RADIO_ROTATION] != RADIO_UNSET) {
			return true;
		}
		fpga_set_rx_quarter_shift_mode(&fpga, rotation >> 6);
#endif
		radio->config[RADIO_BANK_APPLIED][RADIO_ROTATION] = rotation;
		return true;
	}

	if (requested_rf == RADIO_UNSET) {
		if (radio->config[RADIO_BANK_APPLIED][RADIO_FREQUENCY_RF] ==
		    RADIO_UNSET) {
			requested_rf = DEFAULT_RF;
		} else {
			return false;
		}
	}

	bool new_rf =
		(radio->config[RADIO_BANK_APPLIED][RADIO_FREQUENCY_RF] != requested_rf);
	uint64_t requested_rf_hz = requested_rf / FP_ONE_HZ;
#ifdef PRALINE
	if (applied_afe_rate == RADIO_UNSET) {
		return false;
	}
	uint64_t opmode = bank[RADIO_OPMODE];
	if (opmode == RADIO_UNSET) {
		opmode = radio->config[RADIO_BANK_APPLIED][RADIO_OPMODE];
	}
	const tune_config_t* tune_config = select_tune_config(opmode);
	const tune_config_t* applied_tune_config =
		select_tune_config(radio->config[RADIO_BANK_APPLIED][RADIO_OPMODE]);
	bool new_config = (applied_tune_config != tune_config);
	while ((tune_config->rf_range_end_mhz != 0) || (tune_config->if_mhz != 0)) {
		if ((requested_rf_hz == 0) ||
		    (tune_config->rf_range_end_mhz > (requested_rf_hz / FREQ_ONE_MHZ))) {
			break;
		}
		tune_config++;
	}
	bool new_rotation =
		(radio->config[RADIO_BANK_APPLIED][RADIO_ROTATION] !=
		 ((uint64_t) tune_config->shift << 6));
	if (new_rotation) {
		fpga_set_rx_quarter_shift_mode(&fpga, tune_config->shift);
		radio->config[RADIO_BANK_APPLIED][RADIO_ROTATION] =
			tune_config->shift << 6;
	}
	fp_40_24_t offset = applied_afe_rate / 4;
	bool new_offset = (applied_offset != offset);
	if (new_rotation || new_offset || new_config || new_rf) {
		tuning_set_frequency(tune_config, requested_rf_hz, offset / FP_ONE_HZ);
		applied_offset = offset;
	}
#else
	if (new_rf) {
		set_freq(requested_rf_hz);
	}
#endif
	radio->config[RADIO_BANK_APPLIED][RADIO_FREQUENCY_RF] = requested_rf;
	radio->config[RADIO_BANK_APPLIED][RADIO_FREQUENCY_IF] = RADIO_UNSET;
	radio->config[RADIO_BANK_APPLIED][RADIO_FREQUENCY_LO] = RADIO_UNSET;
	radio->config[RADIO_BANK_APPLIED][RADIO_IMAGE_REJECT] = RADIO_UNSET;
	return true;
}

static uint32_t auto_bandwidth(radio_t* const radio, uint64_t opmode)
{
	uint64_t rotation = radio->config[RADIO_BANK_APPLIED][RADIO_ROTATION];

	uint32_t sample_rate_hz = DEFAULT_MCU_RATE / FP_ONE_HZ;
	if (radio->config[RADIO_BANK_APPLIED][RADIO_SAMPLE_RATE] != RADIO_UNSET) {
		sample_rate_hz =
			radio->config[RADIO_BANK_APPLIED][RADIO_SAMPLE_RATE] / FP_ONE_HZ;
	}

	uint32_t offset_hz = 0;
	if ((rotation != 0) && (rotation != RADIO_UNSET) &&
	    (applied_offset != RADIO_UNSET)) {
		offset_hz = applied_offset / FP_ONE_HZ;
	}

	const uint32_t bb_bandwidth = (sample_rate_hz * 3) / 4;
	const uint32_t lpf_bandwidth = bb_bandwidth + offset_hz * 2;

	switch (opmode) {
	case TRANSCEIVER_MODE_TX:
	case TRANSCEIVER_MODE_SS:
		radio->config[RADIO_BANK_APPLIED][RADIO_BB_BANDWIDTH_TX] = bb_bandwidth;
		break;
	default:
		radio->config[RADIO_BANK_APPLIED][RADIO_BB_BANDWIDTH_RX] = bb_bandwidth;
	}
	return lpf_bandwidth;
}

static bool radio_update_bandwidth(radio_t* const radio, uint64_t* bank)
{
	bool new_bw = false;
	uint64_t opmode = bank[RADIO_OPMODE];
	if (opmode == RADIO_UNSET) {
		opmode = radio->config[RADIO_BANK_APPLIED][RADIO_OPMODE];
	}

#ifdef PRALINE
	/* Praline legacy mode always sets baseband bandwidth automatically. */
	(void) bank;
	uint32_t lpf_bandwidth = auto_bandwidth(radio, opmode);

	switch (opmode) {
	case TRANSCEIVER_MODE_TX:
	case TRANSCEIVER_MODE_SS:
		if (radio->config[RADIO_BANK_APPLIED][RADIO_XCVR_TX_LPF] !=
		    lpf_bandwidth) {
			max2831_set_lpf_bandwidth(&max283x, lpf_bandwidth);
			radio->config[RADIO_BANK_APPLIED][RADIO_XCVR_TX_LPF] =
				lpf_bandwidth;
			new_bw = true;
		}
		break;
	default:
		if (radio->config[RADIO_BANK_APPLIED][RADIO_XCVR_RX_LPF] !=
		    lpf_bandwidth) {
			max2831_set_lpf_bandwidth(&max283x, lpf_bandwidth);
			radio->config[RADIO_BANK_APPLIED][RADIO_XCVR_RX_LPF] =
				lpf_bandwidth;
			new_bw = true;
		}
		bool narrow_lpf_enable = false;
		bool applied_narrow_lpf_enable =
			radio->config[RADIO_BANK_APPLIED][RADIO_RX_NARROW_LPF];
		if (lpf_bandwidth <= 1750000) {
			narrow_lpf_enable = true;
		}
		if (applied_narrow_lpf_enable != narrow_lpf_enable) {
			narrowband_filter_set(narrow_lpf_enable);
			radio->config[RADIO_BANK_APPLIED][RADIO_RX_NARROW_LPF] =
				narrow_lpf_enable;
			new_bw = true;
		}
		/* Always set HPF bandwidth to 30 kHz for now. */
		const max2831_rx_hpf_freq_t hpf_bandwidth = MAX2831_RX_HPF_30_KHZ;
		if (radio->config[RADIO_BANK_APPLIED][RADIO_XCVR_RX_HPF] !=
		    hpf_bandwidth) {
			max2831_set_rx_hpf_frequency(&max283x, hpf_bandwidth);
			radio->config[RADIO_BANK_APPLIED][RADIO_XCVR_RX_HPF] =
				hpf_bandwidth;
			new_bw = true;
		}
	}
#else
	uint64_t lpf_bandwidth;
	lpf_bandwidth = bank[RADIO_XCVR_TX_LPF];
	if (lpf_bandwidth == RADIO_UNSET) {
		lpf_bandwidth = bank[RADIO_XCVR_RX_LPF];
	}
	if (lpf_bandwidth == RADIO_UNSET) {
		lpf_bandwidth = bank[RADIO_BB_BANDWIDTH_TX];
	}
	if (lpf_bandwidth == RADIO_UNSET) {
		lpf_bandwidth = bank[RADIO_BB_BANDWIDTH_RX];
	}
	if (lpf_bandwidth == RADIO_UNSET) {
		lpf_bandwidth = radio->config[RADIO_BANK_APPLIED][RADIO_XCVR_TX_LPF];
	}
	if (lpf_bandwidth == RADIO_UNSET) {
		lpf_bandwidth = auto_bandwidth(radio, opmode);
	}

	if (radio->config[RADIO_BANK_APPLIED][RADIO_XCVR_TX_LPF] != lpf_bandwidth) {
		max283x_set_lpf_bandwidth(&max283x, lpf_bandwidth);
		radio->config[RADIO_BANK_APPLIED][RADIO_BB_BANDWIDTH_RX] = lpf_bandwidth;
		radio->config[RADIO_BANK_APPLIED][RADIO_BB_BANDWIDTH_TX] = lpf_bandwidth;
		radio->config[RADIO_BANK_APPLIED][RADIO_XCVR_TX_LPF] = lpf_bandwidth;
		radio->config[RADIO_BANK_APPLIED][RADIO_XCVR_RX_LPF] = lpf_bandwidth;
		new_bw = true;
	}
#endif
	return new_bw;
}

#define DEFAULT_GAIN_RF (0)
#define DEFAULT_GAIN_IF (16)
#define DEFAULT_GAIN_BB (12)

static bool radio_update_gain(radio_t* const radio, uint64_t* bank)
{
	bool new_gain = false;
	uint64_t gain;
	uint64_t opmode = bank[RADIO_OPMODE];
	if (opmode == RADIO_UNSET) {
		opmode = radio->config[RADIO_BANK_APPLIED][RADIO_OPMODE];
	}

	/*
	 * Because control signals are shared by the two RF amps, the setting
	 * for the active amp is reapplied at every opportunity. This ensures
	 * override of any setting from a previous operating mode.
	 */
	switch (opmode) {
	case TRANSCEIVER_MODE_TX:
	case TRANSCEIVER_MODE_SS:
		gain = bank[RADIO_GAIN_TX_RF];
		if (gain == RADIO_UNSET) {
			gain = DEFAULT_GAIN_RF;
		}
		rf_path_set_lna(&rf_path, gain);
		if (radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_TX_RF] != gain) {
			new_gain = true;
		}
		radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_TX_RF] = gain;
		radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_RX_RF] = 0;
		break;
	case TRANSCEIVER_MODE_RX:
	case TRANSCEIVER_MODE_RX_SWEEP:
		gain = bank[RADIO_GAIN_RX_RF];
		if (gain == RADIO_UNSET) {
			gain = DEFAULT_GAIN_RF;
		}
		rf_path_set_lna(&rf_path, gain);
		if (radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_RX_RF] != gain) {
			new_gain = true;
		}
		radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_RX_RF] = gain;
		radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_TX_RF] = 0;
		break;
	default:
		rf_path_set_lna(&rf_path, 0);
		radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_TX_RF] = 0;
		radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_RX_RF] = 0;
	}

	gain = bank[RADIO_GAIN_TX_IF];
	if ((gain != RADIO_UNSET) &&
	    (gain != radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_TX_IF])) {
#ifdef PRALINE
		max2831_set_txvga_gain(&max283x, gain);
#else
		max283x_set_txvga_gain(&max283x, gain);
#endif
		radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_TX_IF] = gain;
		new_gain = true;
	} else if (radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_TX_IF] == RADIO_UNSET) {
#ifdef PRALINE
		max2831_set_txvga_gain(&max283x, DEFAULT_GAIN_IF);
#else
		max283x_set_txvga_gain(&max283x, DEFAULT_GAIN_IF);
#endif
		radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_TX_IF] = DEFAULT_GAIN_IF;
		new_gain = true;
	}

	gain = bank[RADIO_GAIN_RX_IF];
	if ((gain != RADIO_UNSET) &&
	    (gain != radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_TX_IF])) {
#ifdef PRALINE
		max2831_set_lna_gain(&max283x, gain);
#else
		max283x_set_lna_gain(&max283x, gain);
#endif
		radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_TX_IF] = gain;
		new_gain = true;
	} else if (radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_TX_IF] == RADIO_UNSET) {
#ifdef PRALINE
		max2831_set_lna_gain(&max283x, DEFAULT_GAIN_IF);
#else
		max283x_set_lna_gain(&max283x, DEFAULT_GAIN_IF);
#endif
		radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_TX_IF] = DEFAULT_GAIN_IF;
		new_gain = true;
	}

	gain = bank[RADIO_GAIN_RX_BB];
	if ((gain != RADIO_UNSET) &&
	    (gain != radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_RX_BB])) {
#ifdef PRALINE
		max2831_set_vga_gain(&max283x, gain);
#else
		max283x_set_vga_gain(&max283x, gain);
#endif
		radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_RX_BB] = gain;
		new_gain = true;
	} else if (radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_RX_BB] == RADIO_UNSET) {
#ifdef PRALINE
		max2831_set_vga_gain(&max283x, DEFAULT_GAIN_BB);
#else
		max283x_set_vga_gain(&max283x, DEFAULT_GAIN_BB);
#endif
		radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_RX_BB] = DEFAULT_GAIN_BB;
		new_gain = true;
	}

	return new_gain;
}

static bool radio_update_bias_tee(radio_t* const radio, uint64_t* bank)
{
	if (detected_platform() == BOARD_ID_JAWBREAKER) {
		return false;
	}

	const uint64_t requested = bank[RADIO_BIAS_TEE];
	const bool enable = requested;

	if (requested == RADIO_UNSET) {
		return false;
	}

	if (radio->config[RADIO_BANK_APPLIED][RADIO_BIAS_TEE] == (uint64_t) enable) {
		return false;
	}

	rf_path_set_antenna(&rf_path, enable);
	radio->config[RADIO_BANK_APPLIED][RADIO_BIAS_TEE] = enable;
	return true;
}

static bool radio_update_trigger(radio_t* const radio, uint64_t* bank)
{
	const uint64_t requested = bank[RADIO_TRIGGER];
	bool enable = requested;

	if (requested == RADIO_UNSET) {
		enable = false;
	}

	if (radio->config[RADIO_BANK_APPLIED][RADIO_TRIGGER] == (uint64_t) enable) {
		return false;
	}

	trigger_enable(enable);
	radio->config[RADIO_BANK_APPLIED][RADIO_TRIGGER] = enable;
	return true;
}

static bool radio_update_dc_block(radio_t* const radio, uint64_t* bank)
{
#ifndef PRALINE
	(void) radio;
	(void) bank;
	return false;
#else
	const uint64_t requested = bank[RADIO_DC_BLOCK];
	bool enable = requested;

	if (requested == RADIO_UNSET) {
		enable = true;
	}

	if (radio->config[RADIO_BANK_APPLIED][RADIO_DC_BLOCK] == (uint64_t) enable) {
		return false;
	}

	fpga_set_rx_dc_block_enable(&fpga, enable);
	radio->config[RADIO_BANK_APPLIED][RADIO_DC_BLOCK] = enable;
	return true;
#endif
}

bool radio_update(radio_t* const radio)
{
	uint64_t tmp_bank[RADIO_NUM_REGS];
	nvic_disable_irq(NVIC_USB0_IRQ);
	uint32_t dirty = radio->regs_dirty;
	uint32_t changed = 0;
	if (dirty == 0) {
		nvic_enable_irq(NVIC_USB0_IRQ);
		return false;
	}
	radio->regs_dirty = 0;
	memcpy(&tmp_bank[0], &(radio->config[RADIO_BANK_ACTIVE][0]), sizeof(tmp_bank));
	nvic_enable_irq(NVIC_USB0_IRQ);

	if ((dirty & RADIO_REG_GROUP_RATE) ||
	    ((detected_platform() == BOARD_ID_PRALINE) &&
	     (dirty & (1 << RADIO_OPMODE)))) {
		if (radio_update_sample_rate(radio, &tmp_bank[0])) {
			changed |= RADIO_REG_GROUP_RATE;
		}
	}
	if ((dirty & RADIO_REG_GROUP_FREQ) ||
	    ((detected_platform() == BOARD_ID_PRALINE) &&
	     ((changed & RADIO_REG_GROUP_RATE) || (dirty & (1 << RADIO_OPMODE))))) {
		if (radio_update_frequency(radio, &tmp_bank[0])) {
			changed |= RADIO_REG_GROUP_FREQ;
		}
	}
	if ((dirty & RADIO_REG_GROUP_BW) ||
	    ((detected_platform() == BOARD_ID_PRALINE) &&
	     (changed & (RADIO_REG_GROUP_RATE | RADIO_REG_GROUP_FREQ)))) {
		if (radio_update_bandwidth(radio, &tmp_bank[0])) {
			changed |= RADIO_REG_GROUP_BW;
		}
	}
	if (dirty & RADIO_REG_GROUP_GAIN) {
		if (radio_update_gain(radio, &tmp_bank[0])) {
			changed |= RADIO_REG_GROUP_GAIN;
		}
	}
	if (dirty & ((1 << RADIO_BIAS_TEE) | (1 << RADIO_OPMODE))) {
		if (radio_update_bias_tee(radio, &tmp_bank[0])) {
			changed |= RADIO_BIAS_TEE;
		}
	}
	if (dirty & (1 << RADIO_TRIGGER)) {
		if (radio_update_trigger(radio, &tmp_bank[0])) {
			changed |= RADIO_TRIGGER;
		}
	}
	if (dirty & (1 << RADIO_DC_BLOCK)) {
		if (radio_update_dc_block(radio, &tmp_bank[0])) {
			changed |= RADIO_DC_BLOCK;
		}
	}
	if (dirty & (1 << RADIO_OPMODE)) {
		if (radio_update_direction(radio, &tmp_bank[0])) {
			changed |= RADIO_OPMODE;
		}
	}

	if (radio->update_cb) {
		radio->update_cb(changed);
	}
	return (changed != 0);
}

void radio_switch_opmode(radio_t* const radio, const transceiver_mode_t mode)
{
	radio_register_bank_t source_bank;
	uint64_t value, previous;

	switch (mode) {
	case TRANSCEIVER_MODE_RX_SWEEP:
	case TRANSCEIVER_MODE_RX:
		source_bank = RADIO_BANK_RX;
		break;
	case TRANSCEIVER_MODE_TX:
	case TRANSCEIVER_MODE_SS:
		source_bank = RADIO_BANK_TX;
		break;
	default:
		source_bank = RADIO_BANK_IDLE;
	}

	nvic_disable_irq(NVIC_USB0_IRQ);
	for (uint8_t reg = 0; reg < RADIO_NUM_REGS; reg++) {
		value = radio->config[source_bank][reg];
		previous = radio->config[RADIO_BANK_ACTIVE][reg];
		if ((value != RADIO_UNSET) && (value != previous)) {
			radio->config[RADIO_BANK_ACTIVE][reg] = value;
			mark_dirty(radio, reg);
		}
	}

	mark_dirty(radio, RADIO_OPMODE);
	nvic_enable_irq(NVIC_USB0_IRQ);
	radio_update(radio);
}
