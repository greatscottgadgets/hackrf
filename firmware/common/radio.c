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

#include "radio.h"

#include <string.h>

#include <libopencm3/cm3/nvic.h>

#include "fixed_point.h"
#include "hackrf_core.h"
#include "hackrf_ui.h"
#include "mixer.h"
#include "max283x.h"
#include "platform_detect.h"
#include "rf_path.h"
#include "sgpio.h"
#include "transceiver_mode.h"
#include "tuning.h"
#if defined(PRALINE)
	#include "fpga.h"
	#include "tune_config.h"
#endif
#if defined(HACKRF_ONE) || defined(PRALINE)
	#include "operacake.h"
#endif

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

#define MIN_MCU_RATE     SR_FP_KHZ(200)
#define MAX_MCU_RATE     SR_FP_KHZ(21800)
#define DEFAULT_MCU_RATE SR_FP_KHZ(10000)

static bool radio_update_sample_rate(radio_t* const radio, uint64_t* bank)
{
	fp_28_36_t rate, afe_rate, previous_rate, previous_afe_rate;
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
		n = compute_resample_log(rate / SR_FP_ONE_HZ, requested_n);
		if (n != radio->config[RADIO_BANK_APPLIED][RADIO_RESAMPLE_TX]) {
#ifdef PRALINE
			fpga_set_tx_interpolation_ratio(&fpga, n);
#endif
			radio->config[RADIO_BANK_APPLIED][RADIO_RESAMPLE_TX] = n;
		}
		break;
	default:
		n = compute_resample_log(rate / SR_FP_ONE_HZ, requested_n);
		if (n != radio->config[RADIO_BANK_APPLIED][RADIO_RESAMPLE_RX]) {
#ifdef PRALINE
			fpga_set_rx_decimation_ratio(&fpga, n);
#endif
			radio->config[RADIO_BANK_APPLIED][RADIO_RESAMPLE_RX] = n;
		}
	}
	new_n = (n != previous_n);

	if (radio->sample_rate_cb) {
		afe_rate = rate << n;
		afe_rate = radio->sample_rate_cb(afe_rate, false);
		previous_rate = radio->config[RADIO_BANK_APPLIED][RADIO_SAMPLE_RATE];
		if ((previous_n == RADIO_UNSET) || previous_rate == RADIO_UNSET) {
			previous_afe_rate = RADIO_UNSET;
		} else {
			previous_afe_rate = previous_rate << previous_n;
		}
		new_afe_rate = (afe_rate != previous_afe_rate);
		if (new_afe_rate) {
			afe_rate = radio->sample_rate_cb(afe_rate, true);
		}
	} else {
		return false;
	}
	rate = afe_rate >> n;
	new_rate = (rate != previous_rate);
	if (new_rate) {
		radio->config[RADIO_BANK_APPLIED][RADIO_SAMPLE_RATE] = rate;
		if (rate != RADIO_UNSET) {
			/* Round to the nearest Hz for display. */
			const uint32_t rate_hz =
				(rate + (SR_FP_ONE_HZ >> 1)) / SR_FP_ONE_HZ;
			hackrf_ui()->set_sample_rate(rate_hz);
		}
	}

	return (new_afe_rate || new_rate || new_n);
}

/*
 * Compute frequency offset in 1/(2**24) Hz for a given rotation. This is an
 * unsigned value that does not indicate direction of frequency shift.
 */
static fp_40_24_t compute_offset(radio_t* const radio, uint64_t opmode, uint64_t rotation)
{
	uint64_t n;
	fp_40_24_t offset = 0;
	const fp_28_36_t mcu_rate = radio->config[RADIO_BANK_APPLIED][RADIO_SAMPLE_RATE];

	switch (opmode) {
	case TRANSCEIVER_MODE_TX:
	case TRANSCEIVER_MODE_SS:
		n = radio->config[RADIO_BANK_APPLIED][RADIO_RESAMPLE_TX];
		break;
	default:
		n = radio->config[RADIO_BANK_APPLIED][RADIO_RESAMPLE_RX];
	}
	if ((rotation != 0) && (rotation != RADIO_UNSET) && (mcu_rate != RADIO_UNSET) &&
	    (n != RADIO_UNSET)) {
		const fp_40_24_t afe_rate = mcu_rate >> (12 - n);
		if (rotation & 0x80) {
			rotation = 0x100 - rotation;
		}
		offset = (afe_rate * rotation) >> 8;
	}
	return offset;
}

/*
 * Convert center frequency of analog baseband (as seen by ADC/DAC) to center
 * frequency of digital baseband (as seen by MCU).
 */
static fp_40_24_t digital_from_analog_rf(
	radio_t* const radio,
	const fp_40_24_t arf,
	uint64_t opmode,
	uint64_t rotation)
{
	fp_40_24_t offset = compute_offset(radio, opmode, rotation);
	fp_40_24_t drf = arf;
	if (rotation > 0x80) {
		if (offset > arf) {
			drf = offset - arf;
		} else {
			drf = arf - offset;
		}
	} else if ((rotation & 0x7f) > 0x00) {
		drf = arf + offset;
	}
	return drf;
}

#ifdef PRALINE
/*
 * Convert center frequency of digital baseband (as seen by MCU) to center
 * frequency of analog baseband (as seen by ADC/DAC).
 */
static fp_40_24_t analog_from_digital_rf(
	radio_t* const radio,
	const fp_40_24_t drf,
	uint64_t opmode,
	uint64_t rotation)
{
	fp_40_24_t offset = compute_offset(radio, opmode, rotation);
	fp_40_24_t arf = drf;
	if (rotation > 0x80) {
		arf = drf + offset;
	} else if ((rotation & 0x7f) > 0x00) {
		if (offset > drf) {
			arf = offset - drf;
		} else {
			arf = drf - offset;
		}
	}
	return arf;
}

static const tune_config_t* select_tune_config(uint64_t opmode, fp_40_24_t freq_rf)
{
	const tune_config_t* tune_config;

	switch (opmode) {
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
	while ((tune_config->rf_range_end_mhz != 0) || (tune_config->if_mhz != 0)) {
		if ((freq_rf == 0) ||
		    (tune_config->rf_range_end_mhz > (freq_rf / FP_ONE_MHZ))) {
			break;
		}
		tune_config++;
	}
	return tune_config;
}
#endif

static bool radio_update_frequency(radio_t* const radio, uint64_t* bank)
{
	bool new_freq = false;
	bool high_lo = false;
	bool invert_spectrum = false;

	const uint64_t requested_rf = bank[RADIO_FREQUENCY_RF];
	const uint64_t requested_if = bank[RADIO_FREQUENCY_IF];
	const uint64_t requested_lo = bank[RADIO_FREQUENCY_LO];
	const uint64_t requested_img_reject = bank[RADIO_IMAGE_REJECT];
	const uint64_t requested_rotation = bank[RADIO_ROTATION];

	const uint64_t applied_rf = radio->config[RADIO_BANK_APPLIED][RADIO_FREQUENCY_RF];
	const uint64_t applied_if = radio->config[RADIO_BANK_APPLIED][RADIO_FREQUENCY_IF];
	const uint64_t applied_lo = radio->config[RADIO_BANK_APPLIED][RADIO_FREQUENCY_LO];
	const uint64_t applied_img_reject =
		radio->config[RADIO_BANK_APPLIED][RADIO_IMAGE_REJECT];
	const uint64_t applied_rotation =
		radio->config[RADIO_BANK_APPLIED][RADIO_ROTATION];

	uint64_t freq_rf = applied_rf;
	uint64_t analog_rf = applied_rf;
	uint64_t freq_if = applied_if;
	uint64_t freq_lo = applied_lo;
	uint64_t img_reject = applied_img_reject;
	uint64_t rotation = applied_rotation;

	uint64_t opmode = bank[RADIO_OPMODE];
	if (opmode == RADIO_UNSET) {
		opmode = radio->config[RADIO_BANK_APPLIED][RADIO_OPMODE];
	}

	/* Restrict requested settings to valid ranges. */
	if (requested_img_reject != RADIO_UNSET) {
		switch (requested_img_reject) {
		case RF_PATH_FILTER_LOW_PASS:
		case RF_PATH_FILTER_HIGH_PASS:
			img_reject = requested_img_reject;
			break;
		default:
			img_reject = RF_PATH_FILTER_BYPASS;
		}
	}
	if (requested_if != RADIO_UNSET) {
		freq_if = max283x_set_frequency(&max283x, requested_if, false);
	}
	if (requested_lo != RADIO_UNSET) {
		freq_lo = mixer_set_frequency(&mixer, freq_lo, false);
	}
	if (detected_platform() == BOARD_ID_PRALINE) {
		if (requested_rotation != RADIO_UNSET) {
			rotation = requested_rotation;
		}
		switch (opmode) {
		case TRANSCEIVER_MODE_RX:
		case TRANSCEIVER_MODE_RX_SWEEP:
			/* Round to nearest supported rotation. */
			rotation = (rotation + 0x20) & 0xc0;
			if (rotation == 0x80) {
				rotation = 0;
			}
			break;
		default:
			rotation = 0;
		}
	} else {
		rotation = 0;
	}

	/* Handle requested RF (auto-tune). */
	if (requested_rf != RADIO_UNSET) {
		if (requested_img_reject == RADIO_UNSET) {
			img_reject = select_img_reject(requested_rf);
		}
		freq_rf = restrict_rf(requested_rf, img_reject);

		/* Look up settings appropriate for requested RF. */
		if (detected_platform() == BOARD_ID_PRALINE) {
#ifdef PRALINE
			const tune_config_t* tune_config =
				select_tune_config(opmode, freq_rf);
			if (requested_rotation == RADIO_UNSET) {
				rotation = (uint8_t) (tune_config->shift) << 6;
			}
			analog_rf =
				analog_from_digital_rf(radio, freq_rf, opmode, rotation);
			if (requested_if == RADIO_UNSET) {
				if (tune_config->if_mhz == 0) {
					freq_if = analog_rf;
				} else {
					freq_if = tune_config->if_mhz * FP_ONE_MHZ;
				}
			}
			high_lo = tune_config->high_lo;
#endif
		} else {
			/* Use graduated or fixed IF on older platforms. */
			if (requested_if == RADIO_UNSET) {
				freq_if = select_graduated_if(freq_rf, img_reject);
			}
			high_lo = (img_reject == RF_PATH_FILTER_LOW_PASS);
			analog_rf = freq_rf;
		}

		/* Compute precise LO. This is done first because the mixer LO is set in coarse steps. */
		if (requested_lo == RADIO_UNSET) {
			switch (img_reject) {
			case RF_PATH_FILTER_LOW_PASS:
				if (high_lo) {
					freq_lo = freq_if + analog_rf;
				} else {
					freq_lo = freq_if - analog_rf;
				}
				break;
			case RF_PATH_FILTER_HIGH_PASS:
				freq_lo = analog_rf - freq_if;
				break;
			default:
				freq_lo = RADIO_UNSET;
			}
			if (freq_lo != RADIO_UNSET) {
				freq_lo = mixer_set_frequency(&mixer, freq_lo, false);
			}
		}

		/* Recompute IF, compensating for rounding of mixer LO. */
		if (requested_if == RADIO_UNSET) {
			switch (img_reject) {
			case RF_PATH_FILTER_LOW_PASS:
				if (high_lo) {
					freq_if = freq_lo - analog_rf;
				} else {
					freq_if = freq_lo + analog_rf;
				}
				break;
			case RF_PATH_FILTER_HIGH_PASS:
				freq_if = analog_rf - freq_lo;
			}
		}
	}

	/* Apply settings. */
	if ((freq_if != applied_if) && (freq_if != RADIO_UNSET)) {
		freq_if = max283x_set_frequency(&max283x, freq_if, true);
		radio->config[RADIO_BANK_APPLIED][RADIO_FREQUENCY_IF] = freq_if;
		new_freq = true;
	}
	if ((freq_lo != applied_lo) && (freq_lo != RADIO_UNSET)) {
		freq_lo = mixer_set_frequency(&mixer, freq_lo, true);
		radio->config[RADIO_BANK_APPLIED][RADIO_FREQUENCY_LO] = freq_lo;
		new_freq = true;
	}
	if ((img_reject != applied_img_reject) && (img_reject != RADIO_UNSET)) {
		rf_path_set_filter(&rf_path, img_reject, opmode);
		radio->config[RADIO_BANK_APPLIED][RADIO_IMAGE_REJECT] = img_reject;
		new_freq = true;
	}
	if ((rotation != applied_rotation) && (rotation != RADIO_UNSET)) {
#ifdef PRALINE
		fpga_set_rx_quarter_shift_mode(&fpga, rotation >> 6);
#endif
		radio->config[RADIO_BANK_APPLIED][RADIO_ROTATION] = rotation;
		new_freq = true;
	}

	/* Compute precise RF. */
	if ((img_reject != RADIO_UNSET) && (freq_if != RADIO_UNSET) &&
	    (rotation != RADIO_UNSET) &&
	    ((freq_lo != RADIO_UNSET) || (img_reject == RF_PATH_FILTER_BYPASS))) {
		switch (img_reject) {
		case RF_PATH_FILTER_LOW_PASS:
			if (freq_lo > freq_if) {
				analog_rf = freq_lo - freq_if;
				invert_spectrum = true;
			} else {
				analog_rf = freq_if - freq_lo;
			}
			break;
		case RF_PATH_FILTER_HIGH_PASS:
			analog_rf = freq_if + freq_lo;
			break;
		default:
			analog_rf = freq_if;
		}
		freq_rf = digital_from_analog_rf(radio, analog_rf, opmode, rotation);
	}
	if ((freq_rf != applied_rf) && (freq_rf != RADIO_UNSET)) {
		radio->config[RADIO_BANK_APPLIED][RADIO_FREQUENCY_RF] = freq_rf;
		new_freq = true;
	}

	if (new_freq) {
		sgpio_cpld_set_mixer_invert(&sgpio_config, invert_spectrum);
		if (opmode != TRANSCEIVER_MODE_RX_SWEEP) {
			hackrf_ui()->set_frequency(freq_rf / FP_ONE_MHZ);
		}
#if defined(PRALINE) || defined(HACKRF_ONE)
		operacake_set_range(freq_rf / FP_ONE_MHZ);
#endif
	}
	return new_freq;
}

static uint32_t auto_bandwidth(radio_t* const radio, uint64_t opmode)
{
	uint32_t offset_hz = 0;
	uint64_t rotation = radio->config[RADIO_BANK_APPLIED][RADIO_ROTATION];

	uint32_t sample_rate_hz = DEFAULT_MCU_RATE / SR_FP_ONE_HZ;
	if (radio->config[RADIO_BANK_APPLIED][RADIO_SAMPLE_RATE] != RADIO_UNSET) {
		sample_rate_hz = radio->config[RADIO_BANK_APPLIED][RADIO_SAMPLE_RATE] /
			SR_FP_ONE_HZ;
	}

	const fp_40_24_t offset = compute_offset(radio, opmode, rotation);
	offset_hz = offset >> 24;

	const uint32_t bb_bandwidth = (sample_rate_hz * 3) / 4;
	const uint32_t lpf_bandwidth = bb_bandwidth + offset_hz * 2;

	radio->config[RADIO_BANK_APPLIED][RADIO_BB_BANDWIDTH_TX] = bb_bandwidth;
	radio->config[RADIO_BANK_APPLIED][RADIO_BB_BANDWIDTH_RX] = bb_bandwidth;

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

	if (radio->config[RADIO_BANK_APPLIED][RADIO_XCVR_TX_LPF] != lpf_bandwidth) {
		max283x_set_lpf_bandwidth(&max283x, MAX283x_MODE_TX, lpf_bandwidth);
		radio->config[RADIO_BANK_APPLIED][RADIO_XCVR_TX_LPF] = lpf_bandwidth;
		new_bw = true;
	}
	if (radio->config[RADIO_BANK_APPLIED][RADIO_XCVR_RX_LPF] != lpf_bandwidth) {
		max283x_set_lpf_bandwidth(&max283x, MAX283x_MODE_RX, lpf_bandwidth);
		radio->config[RADIO_BANK_APPLIED][RADIO_XCVR_RX_LPF] = lpf_bandwidth;
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
	const max283x_rx_hpf_freq_t hpf_bandwidth = MAX283x_RX_HPF_30_KHZ;
	if (radio->config[RADIO_BANK_APPLIED][RADIO_XCVR_RX_HPF] != hpf_bandwidth) {
		max283x_set_rx_hpf_frequency(&max283x, hpf_bandwidth);
		radio->config[RADIO_BANK_APPLIED][RADIO_XCVR_RX_HPF] = hpf_bandwidth;
		new_bw = true;
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
		max283x_set_lpf_bandwidth(&max283x, MAX283x_MODE_TX, lpf_bandwidth);
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
		max283x_set_txvga_gain(&max283x, gain);
		radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_TX_IF] = gain;
		new_gain = true;
	} else if (radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_TX_IF] == RADIO_UNSET) {
		max283x_set_txvga_gain(&max283x, DEFAULT_GAIN_IF);
		radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_TX_IF] = DEFAULT_GAIN_IF;
		new_gain = true;
	}

	gain = bank[RADIO_GAIN_RX_IF];
	if ((gain != RADIO_UNSET) &&
	    (gain != radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_RX_IF])) {
		max283x_set_lna_gain(&max283x, gain);
		radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_RX_IF] = gain;
		new_gain = true;
	} else if (radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_RX_IF] == RADIO_UNSET) {
		max283x_set_lna_gain(&max283x, DEFAULT_GAIN_IF);
		radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_RX_IF] = DEFAULT_GAIN_IF;
		new_gain = true;
	}

	gain = bank[RADIO_GAIN_RX_BB];
	if ((gain != RADIO_UNSET) &&
	    (gain != radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_RX_BB])) {
		max283x_set_vga_gain(&max283x, gain);
		radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_RX_BB] = gain;
		new_gain = true;
	} else if (radio->config[RADIO_BANK_APPLIED][RADIO_GAIN_RX_BB] == RADIO_UNSET) {
		max283x_set_vga_gain(&max283x, DEFAULT_GAIN_BB);
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
	if (dirty == 0) {
		nvic_enable_irq(NVIC_USB0_IRQ);
		return false;
	}
	radio->regs_dirty = 0;
	memcpy(&tmp_bank[0], &(radio->config[RADIO_BANK_ACTIVE][0]), sizeof(tmp_bank));
	nvic_enable_irq(NVIC_USB0_IRQ);

	bool dir = false;
	bool rate = false;
	bool freq = false;
	bool bw = false;
	bool gain = false;
	bool bias = false;
	bool trig = false;
	bool dc = false;

	if ((dirty &
	     ((1 << RADIO_SAMPLE_RATE) | (1 << RADIO_RESAMPLE_TX) |
	      (1 << RADIO_RESAMPLE_RX))) ||
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
	     ((1 << RADIO_BB_BANDWIDTH_TX) | (1 << RADIO_BB_BANDWIDTH_RX) |
	      (1 << RADIO_XCVR_TX_LPF) | (1 << RADIO_XCVR_RX_LPF) |
	      (1 << RADIO_XCVR_RX_HPF) | (1 << RADIO_RX_NARROW_LPF))) ||
	    ((detected_platform() == BOARD_ID_PRALINE) && (rate || freq))) {
		bw = radio_update_bandwidth(radio, &tmp_bank[0]);
	}
	if (dirty &
	    ((1 << RADIO_GAIN_TX_RF) | (1 << RADIO_GAIN_TX_IF) | (1 << RADIO_GAIN_RX_RF) |
	     (1 << RADIO_GAIN_RX_IF) | (1 << RADIO_GAIN_RX_BB) | (1 << RADIO_OPMODE))) {
		gain = radio_update_gain(radio, &tmp_bank[0]);
	}
	if (dirty & ((1 << RADIO_BIAS_TEE) | (1 << RADIO_OPMODE))) {
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

	radio->config[RADIO_BANK_ACTIVE][RADIO_OPMODE] = mode;
	mark_dirty(radio, RADIO_OPMODE);
	nvic_enable_irq(NVIC_USB0_IRQ);
	radio_update(radio);
}
