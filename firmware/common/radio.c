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

#include "hackrf_core.h"
#include "tuning.h"

#include "radio.h"

radio_error_t radio_set_sample_rate(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_sample_rate_id element,
	radio_sample_rate_t sample_rate)
{
	// we only support the clock generator at the moment
	if (element != RADIO_SAMPLE_RATE_CLOCKGEN) {
		return RADIO_ERR_INVALID_ELEMENT;
	}

	radio_config_t* config = &radio->channel[chan_id].config;

	/*
	 * Store the sample rate rounded to the nearest Hz for convenience.
	 */
	if (sample_rate.div == 0) {
		return RADIO_ERR_INVALID_PARAM;
	}
	sample_rate.hz = (sample_rate.num + (sample_rate.div >> 1)) / sample_rate.div;

	if (config->mode == TRANSCEIVER_MODE_OFF) {
		config->sample_rate[element] = sample_rate;
		return RADIO_OK;
	}

#ifdef PRALINE
	#define MAX_AFE_RATE 40000000
	#define MAX_N        5
	uint8_t n = 0; // resampling ratio is 2**n
	if ((config->mode == TRANSCEIVER_MODE_RX) ||
	    (config->mode == TRANSCEIVER_MODE_RX_SWEEP)) {
		n = 1;
		uint32_t afe_rate_x2 = 4 * sample_rate.hz;
		while ((afe_rate_x2 <= MAX_AFE_RATE) && (n < MAX_N)) {
			afe_rate_x2 <<= 1;
			n++;
		}
		fpga_set_rx_decimation_ratio(&fpga, n);
	}
	config->resampling_n = n;
	bool ok = sample_rate_frac_set(sample_rate.num << n, sample_rate.div);
	if (ok) {
		config->sample_rate[element] = sample_rate;
		radio_channel_t* channel = &radio->channel[chan_id];
		radio_frequency_t frequency =
			radio_get_frequency(radio, channel->id, RADIO_FREQUENCY_RF);
		ok = radio_set_frequency(
			radio,
			channel->id,
			RADIO_FREQUENCY_RF,
			frequency);
	}
#else
	bool ok = sample_rate_frac_set(sample_rate.num, sample_rate.div);
#endif
	if (!ok) {
		return RADIO_ERR_INVALID_PARAM;
	}

	config->sample_rate[element] = sample_rate;
	return RADIO_OK;
}

radio_sample_rate_t radio_get_sample_rate(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_sample_rate_id element)
{
	return radio->channel[chan_id].config.sample_rate[element];
}

radio_error_t radio_set_filter(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_filter_id element,
	radio_filter_t filter)
{
	// we only support the baseband filter at the moment
	if (element != RADIO_FILTER_BASEBAND) {
		return RADIO_ERR_INVALID_ELEMENT;
	}

	radio_config_t* config = &radio->channel[chan_id].config;

	if (config->mode == TRANSCEIVER_MODE_OFF) {
		config->filter[element] = filter;
		return RADIO_OK;
	}

#ifndef PRALINE
	max283x_set_lpf_bandwidth(&max283x, filter.hz);
#else
	uint32_t lpf_bandwidth =
		(config->sample_rate[RADIO_SAMPLE_RATE_CLOCKGEN].hz * 3) / 4;
	uint32_t offset = 0;
	if (config->shift != FPGA_QUARTER_SHIFT_MODE_NONE) {
		offset = (config->sample_rate[RADIO_SAMPLE_RATE_CLOCKGEN].hz
			  << config->resampling_n) /
			4;
	}
	lpf_bandwidth += offset * 2;
	max2831_set_lpf_bandwidth(&max283x, lpf_bandwidth);
#endif

	config->filter[element] = filter;
	return RADIO_OK;
}

radio_filter_t radio_get_filter(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_filter_id element)
{
	return radio->channel[chan_id].config.filter[element];
}

radio_error_t radio_set_gain(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_gain_id element,
	radio_gain_t gain)
{
	if (element > RADIO_GAIN_COUNT) {
		return RADIO_ERR_INVALID_ELEMENT;
	}

	radio_config_t* config = &radio->channel[chan_id].config;

	if (config->mode == TRANSCEIVER_MODE_OFF) {
		config->gain[element] = gain;
		return RADIO_OK;
	}

	uint8_t real_db;
	switch (element) {
	case RADIO_GAIN_RF_AMP:
		rf_path_set_lna(&rf_path, gain.enable);
		break;
	case RADIO_GAIN_RX_LNA:
#ifndef PRALINE
		real_db = max283x_set_lna_gain(&max283x, gain.db);
#else
		real_db = max2831_set_lna_gain(&max283x, gain.db);
#endif
		if (real_db == 0) {
			return RADIO_ERR_INVALID_PARAM;
		}
		break;
	case RADIO_GAIN_RX_VGA:
#ifndef PRALINE
		real_db = max283x_set_vga_gain(&max283x, gain.db);
#else
		real_db = max2831_set_vga_gain(&max283x, gain.db);
#endif
		if (real_db == 0) {
			return RADIO_ERR_INVALID_PARAM;
		}
		break;
	case RADIO_GAIN_TX_VGA:
#ifndef PRALINE
		real_db = max283x_set_txvga_gain(&max283x, gain.db);
#else
		real_db = max2831_set_txvga_gain(&max283x, gain.db);
#endif
		if (real_db == 0) {
			return RADIO_ERR_INVALID_PARAM;
		}
		break;
	}

	config->gain[element] = gain;
	return RADIO_OK;
}

radio_gain_t radio_get_gain(radio_t* radio, radio_chan_id chan_id, radio_gain_id element)
{
	return radio->channel[chan_id].config.gain[element];
}

static uint64_t applied_freq = 0xffffffffffffffff;

radio_error_t radio_set_frequency(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_frequency_id element,
	radio_frequency_t frequency)
{
	// we only support setting the final rf frequency at the moment
	if (element != RADIO_FREQUENCY_RF) {
		return RADIO_ERR_INVALID_ELEMENT;
	}

	radio_config_t* config = &radio->channel[chan_id].config;

	if (config->mode == TRANSCEIVER_MODE_OFF) {
		config->frequency[element] = frequency;
		return RADIO_OK;
	}

	// explicit
	if (frequency.if_hz || frequency.lo_hz) {
		bool ok = set_freq_explicit(
			frequency.if_hz,
			frequency.lo_hz,
			frequency.path);
#ifdef PRALINE
		if (ok) {
			fpga_set_rx_quarter_shift_mode(
				&fpga,
				FPGA_QUARTER_SHIFT_MODE_NONE);
			config->shift = FPGA_QUARTER_SHIFT_MODE_NONE;
			radio_channel_t* channel = &radio->channel[chan_id];
			radio_filter_t filter = radio_get_filter(
				radio,
				channel->id,
				RADIO_FILTER_BASEBAND);
			ok = radio_set_filter(
				radio,
				channel->id,
				RADIO_FILTER_BASEBAND,
				filter);
		}
#endif
		if (!ok) {
			return RADIO_ERR_INVALID_PARAM;
		}

		config->frequency[element] = frequency;
		return RADIO_OK;
	}

	// auto-tune
	if (frequency.hz == applied_freq) {
		return RADIO_OK;
	}
	applied_freq = frequency.hz;
	bool ok;
#ifndef PRALINE
	switch (config->mode) {
	case TRANSCEIVER_MODE_RX:
	case TRANSCEIVER_MODE_RX_SWEEP:
	case TRANSCEIVER_MODE_TX:
		ok = set_freq(frequency.hz);
		break;
	default:
		return RADIO_ERR_INVALID_CONFIG;
	}
#else
	const tune_config_t* tune_config;
	switch (config->mode) {
	case TRANSCEIVER_MODE_RX:
		tune_config = praline_tune_config_rx;
		break;
	case TRANSCEIVER_MODE_RX_SWEEP:
		tune_config = praline_tune_config_rx_sweep;
		break;
	case TRANSCEIVER_MODE_TX:
		tune_config = praline_tune_config_tx;
		break;
	default:
		return RADIO_ERR_INVALID_CONFIG;
	}
	bool found = false;
	for (; (tune_config->rf_range_end_mhz != 0) || (tune_config->if_mhz != 0);
	     tune_config++) {
		if ((frequency.hz == 0) ||
		    (tune_config->rf_range_end_mhz > (frequency.hz / FREQ_ONE_MHZ))) {
			found = true;
			break;
		}
	}
	if (!found) {
		return RADIO_ERR_INVALID_PARAM;
	}

	fpga_set_rx_quarter_shift_mode(&fpga, tune_config->shift);
	config->shift = tune_config->shift;
	uint32_t offset = (config->sample_rate[RADIO_SAMPLE_RATE_CLOCKGEN].hz
			   << config->resampling_n) /
		4;
	ok = tuning_set_frequency(tune_config, frequency.hz, offset);
	if (ok) {
		radio_channel_t* channel = &radio->channel[chan_id];
		radio_filter_t filter =
			radio_get_filter(radio, channel->id, RADIO_FILTER_BASEBAND);
		ok = radio_set_filter(radio, channel->id, RADIO_FILTER_BASEBAND, filter);
	}
#endif
	if (!ok) {
		return RADIO_ERR_INVALID_PARAM;
	}

	config->frequency[element] = frequency;
	return RADIO_OK;
}

radio_frequency_t radio_get_frequency(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_frequency_id element)
{
	return radio->channel[chan_id].config.frequency[element];
}

radio_error_t radio_set_antenna(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_antenna_id element,
	radio_antenna_t value)
{
	if (element > RADIO_ANTENNA_COUNT) {
		return RADIO_ERR_INVALID_ELEMENT;
	}

	radio_config_t* config = &radio->channel[chan_id].config;

	if (config->mode == TRANSCEIVER_MODE_OFF) {
		config->antenna[element] = value;
		return RADIO_OK;
	}

	switch (element) {
	case RADIO_ANTENNA_BIAS_TEE:
		rf_path_set_antenna(
			&rf_path,
			config->antenna[RADIO_ANTENNA_BIAS_TEE].enable);
		break;
	}

	config->antenna[element] = value;
	return RADIO_OK;
}

radio_antenna_t radio_get_antenna(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_antenna_id element)
{
	return radio->channel[chan_id].config.antenna[element];
}

radio_error_t radio_set_clock(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_clock_id element,
	radio_clock_t value)
{
	radio_config_t* config = &radio->channel[chan_id].config;

	if (element > RADIO_CLOCK_COUNT) {
		return RADIO_ERR_INVALID_ELEMENT;
	}

	// CLKIN is not supported as it is automatically detected from hardware state
	if (element == RADIO_CLOCK_CLKIN) {
		return RADIO_ERR_UNSUPPORTED_OPERATION;
	}

	si5351c_clkout_enable(&clock_gen, value.enable);

	config->clock[element] = value;
	return RADIO_OK;
}

radio_clock_t radio_get_clock(
	radio_t* radio,
	radio_chan_id chan_id,
	radio_clock_id element)
{
	if (element == RADIO_CLOCK_CLKIN) {
		return (radio_clock_t){
			.enable = si5351c_clkin_signal_valid(&clock_gen),
		};
	}

	return radio->channel[chan_id].config.clock[element];
}

radio_error_t radio_set_trigger_enable(radio_t* radio, radio_chan_id chan_id, bool enable)
{
	radio_config_t* config = &radio->channel[chan_id].config;

	config->trigger_enable = enable;
	return RADIO_OK;
}

bool radio_get_trigger_enable(radio_t* radio, radio_chan_id chan_id)
{
	return radio->channel[chan_id].config.trigger_enable;
}

transceiver_mode_t radio_get_mode(radio_t* radio, radio_chan_id chan_id)
{
	return radio->channel[chan_id].config.mode;
}

rf_path_direction_t radio_get_direction(radio_t* radio, radio_chan_id chan_id)
{
	radio_config_t* config = &radio->channel[chan_id].config;

	switch (config->mode) {
	case TRANSCEIVER_MODE_RX:
	case TRANSCEIVER_MODE_RX_SWEEP:
		return RF_PATH_DIRECTION_RX;
	case TRANSCEIVER_MODE_TX:
		return RF_PATH_DIRECTION_TX;
	default:
		return RF_PATH_DIRECTION_OFF;
	}
}

clock_source_t radio_get_clock_source(radio_t* radio, radio_chan_id chan_id)
{
	return radio->channel[chan_id].clock_source;
}

radio_error_t radio_switch_mode(
	radio_t* radio,
	radio_chan_id chan_id,
	transceiver_mode_t mode)
{
	radio_error_t result;
	radio_channel_t* channel = &radio->channel[chan_id];
	radio_config_t* config = &channel->config;

	// configure firmware direction from mode (but don't configure the hardware yet!)
	rf_path_direction_t direction;
	switch (mode) {
	case TRANSCEIVER_MODE_RX:
	case TRANSCEIVER_MODE_RX_SWEEP:
		direction = RF_PATH_DIRECTION_RX;
		break;
	case TRANSCEIVER_MODE_TX:
		direction = RF_PATH_DIRECTION_TX;
		break;
	default:
		rf_path_set_direction(&rf_path, RF_PATH_DIRECTION_OFF);
		config->mode = mode;
		return RADIO_OK;
	}
	config->mode = mode;

	// sample rate
	radio_sample_rate_t sample_rate =
		radio_get_sample_rate(radio, channel->id, RADIO_SAMPLE_RATE_CLOCKGEN);
	result = radio_set_sample_rate(
		radio,
		channel->id,
		RADIO_SAMPLE_RATE_CLOCKGEN,
		sample_rate);
	if (result != RADIO_OK) {
		return result;
	}

	/*
	 * Because of offset tuning on Praline, the sample rate can affect the
	 * tuning configuration, so radio_set_sample_rate() calls
	 * radio_set_frequency(). Also because of offset tuning, the tuning
	 * configuration can affect the baseband filter bandwidth (in addition
	 * to the filter bandwidth being automatically based on the sample
	 * rate), so radio_set_frequency() calls radio_set_filter().
	 */
#ifndef PRALINE
	// tuning frequency
	radio_frequency_t frequency =
		radio_get_frequency(radio, channel->id, RADIO_FREQUENCY_RF);
	result = radio_set_frequency(radio, channel->id, RADIO_FREQUENCY_RF, frequency);
	if (result != RADIO_OK) {
		return result;
	}

	// baseband filter bandwidth
	radio_filter_t filter =
		radio_get_filter(radio, channel->id, RADIO_FILTER_BASEBAND);
	result = radio_set_filter(radio, channel->id, RADIO_FILTER_BASEBAND, filter);
	if (result != RADIO_OK) {
		return result;
	}
#endif

	// rf_amp enable
	radio_gain_t enable = radio_get_gain(radio, channel->id, RADIO_GAIN_RF_AMP);
	result = radio_set_gain(radio, channel->id, RADIO_GAIN_RF_AMP, enable);
	if (result != RADIO_OK) {
		return result;
	}

	// gain
	radio_gain_t gain;
	if (config->mode == TRANSCEIVER_MODE_RX ||
	    config->mode == TRANSCEIVER_MODE_RX_SWEEP) {
		gain = radio_get_gain(radio, channel->id, RADIO_GAIN_RX_LNA);
		result = radio_set_gain(radio, channel->id, RADIO_GAIN_RX_LNA, gain);
		if (result != RADIO_OK) {
			return result;
		}

		gain = radio_get_gain(radio, channel->id, RADIO_GAIN_RX_VGA);
		result = radio_set_gain(radio, channel->id, RADIO_GAIN_RX_VGA, gain);
		if (result != RADIO_OK) {
			return result;
		}

	} else if (config->mode == TRANSCEIVER_MODE_TX) {
		gain = radio_get_gain(radio, channel->id, RADIO_GAIN_TX_VGA);
		result = radio_set_gain(radio, channel->id, RADIO_GAIN_TX_VGA, gain);
		if (result != RADIO_OK) {
			return result;
		}
	}

	// antenna
	radio_antenna_t bias_tee =
		radio_get_antenna(radio, channel->id, RADIO_ANTENNA_BIAS_TEE);
	result = radio_set_antenna(radio, channel->id, RADIO_ANTENNA_BIAS_TEE, bias_tee);
	if (result != RADIO_OK) {
		return result;
	}

	// finally, set the rf path direction
	rf_path_set_direction(&rf_path, direction);

	return RADIO_OK;
}
