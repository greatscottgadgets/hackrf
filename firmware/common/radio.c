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

radio_error_t radio_supported_sample_rate(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	radio_range_t* range)
{
	(void) radio;   // TODO
	(void) chan_id; // TODO
	(void) mode;    // TODO
	(void) range;   // TODO

	// TODO implement real values
	range->start = 1000000;
	range->end = 20000000;

	return RADIO_OK;
}

radio_error_t radio_set_mode_sample_rate(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const uint32_t rate_num,
	const uint32_t rate_denom)
{
	(void) mode; // TODO

	radio_config_t* config = &radio->channel[chan_id].config;

	radio_sample_rate_t sample_rate = {
		.num = rate_num,
		.denom = rate_denom,
	};

	if (config->mode == RADIO_MODE_OFF) {
		config->sample_rate[RADIO_SAMPLE_RATE] = sample_rate;
		return RADIO_OK;
	}

	// #1 set MCU sample rate
	// TODO hackrf_set_sample_rate_frac/manual calls into usb_api_transceiver which
	//      multiplies the requested frequency by 2. We need to make sure
	//      usb_api_radio.c does as well! Or just move the multiplication here.
	bool ok = sample_rate_frac_set(sample_rate.num, sample_rate.denom);
	if (!ok) {
		return RADIO_ERR_INVALID_PARAM;
	}
	config->sample_rate[RADIO_SAMPLE_RATE_MCU] = sample_rate;

	// #2 TODO @mossmann set baseband lpf

	// #3 TODO @mossmann set resampling ratio

	// #4 TODO @mossmann set narrowband aa filter

	// #5 TODO @mossmann set baseband hpf corner frequency

	// update saved configuration
	config->sample_rate[RADIO_SAMPLE_RATE] = sample_rate;
	// TODO config->filter[RADIO_FILTER_BASEBAND_LPF] =
	// TODO config->sample_rate[RADIO_SAMPLE_RATE_RX_DECIMATION_RATIO] =
	//  or  config->sample_rate[RADIO_SAMPLE_RATE_TX_INTERPOLATION_RATIO] =
	// TODO config->filter[RADIO_FILTER_RX_NARROWBAND_LPF] =
	// TODO config->filter[RADIO_FILTER_RX_BASEBAND_HPF] =

	return RADIO_OK;
}

radio_error_t radio_set_mode_frequency(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const uint64_t frequency_hz)
{
	(void) mode; // TODO

	radio_config_t* config = &radio->channel[chan_id].config;

	if (config->mode == RADIO_MODE_OFF) {
		config->frequency[RADIO_FREQUENCY] =
			(radio_frequency_t){.hz = frequency_hz};
		return RADIO_OK;
	}

	// #0 get tuning configuration table
	const tune_config_t* table = NULL;
#ifndef PRALINE
	switch (config->mode) {
	case RADIO_MODE_RX:
	case RADIO_MODE_RX_SWEEP:
	case RADIO_MODE_TX:
		table = max283x_tune_config;
		break;
	default:
		return RADIO_ERR_INVALID_MODE;
	}
#else
	switch (config->mode) {
	case RADIO_MODE_RX:
		table = max2831_tune_config_rx;
		break;
	case RADIO_MODE_RX_SWEEP:
		table = max2831_tune_config_rx_sweep;
		break;
	case RADIO_MODE_TX:
		table = max2831_tune_config_tx;
		break;
	default:
		return RADIO_ERR_INVALID_MODE;
	}
#endif

	// #1 tune the radio
	radio_mode_frequency_t tuned_configuration;
	bool success = tuning_set_frequency(table, frequency_hz, &tuned_configuration);
	if (!success) {
		return RADIO_ERR_INVALID_PARAM;
	}

	// update saved configuration
	config->frequency[RADIO_FREQUENCY].hz = tuned_configuration.hz;
	config->frequency[RADIO_FREQUENCY_LO].hz = tuned_configuration.lo_hz;
	config->frequency[RADIO_FREQUENCY_IF].hz = tuned_configuration.if_hz;
	config->filter[RADIO_FILTER_RF_PATH].mode = tuned_configuration.rf_path_filter;
#ifdef PRALINE
	config->frequency[RADIO_FREQUENCY_RX_QUARTER_SHIFT].mode =
		tuned_configuration.rx_quarter_shift_mode;
#endif

	return RADIO_OK;
}

radio_error_t radio_set_mode_frequency_explicit(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_mode_frequency_t frequency)
{
	(void) mode; // TODO

	radio_config_t* config = &radio->channel[chan_id].config;

	bool success = tuning_set_frequency_explicit(frequency);
	if (!success) {
		return RADIO_ERR_INVALID_CONFIG;
	}

	// update configuration with explicit values
	config->frequency[RADIO_FREQUENCY].hz = 0;
	config->frequency[RADIO_FREQUENCY_LO].hz = frequency.lo_hz;
	config->frequency[RADIO_FREQUENCY_IF].hz = frequency.if_hz;
	config->filter[RADIO_FILTER_RF_PATH].mode = frequency.rf_path_filter;
#ifdef PRALINE
	config->frequency[RADIO_FREQUENCY_RX_QUARTER_SHIFT].mode =
		frequency.rx_quarter_shift_mode;
#endif

	return RADIO_OK;
}

radio_mode_frequency_t radio_get_mode_frequency(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode)
{
	(void) mode; // TODO

	radio_config_t* config = &radio->channel[chan_id].config;

	radio_mode_frequency_t frequency_config = {
		.hz = config->frequency[RADIO_FREQUENCY].hz,
		.lo_hz = config->frequency[RADIO_FREQUENCY_LO].hz,
		.if_hz = config->frequency[RADIO_FREQUENCY_IF].hz,
		.rf_path_filter = config->filter[RADIO_FILTER_RF_PATH].mode,
#ifdef PRALINE
		.rx_quarter_shift_mode =
			config->frequency[RADIO_FREQUENCY_RX_QUARTER_SHIFT].mode,
#endif
	};

	return frequency_config;
}

radio_error_t radio_switch_mode(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode)
{
	radio_error_t result;
	radio_channel_t* channel = &radio->channel[chan_id];
	radio_config_t* config = &channel->config;

	// configure firmware direction from mode (but don't configure the hardware yet!)
	rf_path_direction_t rf_path_direction;
	switch (mode) {
	case RADIO_MODE_RX:
	case RADIO_MODE_RX_SWEEP:
		rf_path_direction = RF_PATH_DIRECTION_RX;
		break;
	case RADIO_MODE_TX:
		rf_path_direction = RF_PATH_DIRECTION_TX;
		break;
	default:
		rf_path_set_direction(&rf_path, RF_PATH_DIRECTION_OFF);
		return RADIO_OK;
	}
	config->mode = mode;

	// set sample rate
	radio_sample_rate_t sample_rate = radio_get_sample_rate_element(
		radio,
		channel->id,
		config->mode,
		RADIO_SAMPLE_RATE);
	result = radio_set_mode_sample_rate(
		radio,
		channel->id,
		config->mode,
		sample_rate.num,
		sample_rate.denom);
	if (result != RADIO_OK) {
		return result;
	}

	// set baseband low pass filter bandwidth
	// TODO this should only be called if RADIO_FILTER_BASEBAND_LPF is different to auto-tuned value
	radio_filter_t filter = radio_get_filter_element(
		radio,
		channel->id,
		config->mode,
		RADIO_FILTER_BASEBAND_LPF);
	result = radio_set_filter_element(
		radio,
		channel->id,
		config->mode,
		RADIO_FILTER_BASEBAND_LPF,
		filter);
	if (result != RADIO_OK) {
		return result;
	}

	// set rf_amp enable
	radio_gain_t enable = radio_get_gain_element(
		radio,
		channel->id,
		config->mode,
		RADIO_GAIN_RF_AMP);
	result = radio_set_gain_element(
		radio,
		channel->id,
		config->mode,
		RADIO_GAIN_RF_AMP,
		enable);
	if (result != RADIO_OK) {
		return result;
	}

	// set gain
	radio_gain_t gain;
	if (config->mode == RADIO_MODE_RX || config->mode == RADIO_MODE_RX_SWEEP) {
		gain = radio_get_gain_element(
			radio,
			channel->id,
			config->mode,
			RADIO_GAIN_RX_LNA);
		result = radio_set_gain_element(
			radio,
			channel->id,
			config->mode,
			RADIO_GAIN_RX_LNA,
			gain);
		if (result != RADIO_OK) {
			return result;
		}

		gain = radio_get_gain_element(
			radio,
			channel->id,
			config->mode,
			RADIO_GAIN_RX_VGA);
		result = radio_set_gain_element(
			radio,
			channel->id,
			config->mode,
			RADIO_GAIN_RX_VGA,
			gain);
		if (result != RADIO_OK) {
			return result;
		}

	} else if (config->mode == RADIO_MODE_TX) {
		gain = radio_get_gain_element(
			radio,
			channel->id,
			config->mode,
			RADIO_GAIN_TX_VGA);
		result = radio_set_gain_element(
			radio,
			channel->id,
			config->mode,
			RADIO_GAIN_TX_VGA,
			gain);
		if (result != RADIO_OK) {
			return result;
		}
	}

	// set antenna
	radio_antenna_t bias_tee = radio_get_antenna_element(
		radio,
		channel->id,
		config->mode,
		RADIO_ANTENNA_BIAS_TEE);
	result = radio_set_antenna_element(
		radio,
		channel->id,
		config->mode,
		RADIO_ANTENNA_BIAS_TEE,
		bias_tee);
	if (result != RADIO_OK) {
		return result;
	}

#ifdef PRALINE
	// set rx dc block
	filter = radio_get_filter_element(
		radio,
		channel->id,
		config->mode,
		RADIO_FILTER_RX_DC_BLOCK);
	result = radio_set_filter_element(
		radio,
		channel->id,
		config->mode,
		RADIO_FILTER_RX_DC_BLOCK,
		filter);
	if (result != RADIO_OK) {
		return result;
	}
#endif

	// set tuning frequency
	radio_frequency_t frequency = radio_get_frequency_element(
		radio,
		channel->id,
		config->mode,
		RADIO_FREQUENCY);
	if (frequency.hz != 0) {
		result = radio_set_mode_frequency(
			radio,
			channel->id,
			config->mode,
			frequency.hz);
	} else {
		radio_mode_frequency_t frequency_config;
		frequency_config =
			radio_get_mode_frequency(radio, channel->id, config->mode);
		result = radio_set_mode_frequency_explicit(
			radio,
			channel->id,
			config->mode,
			frequency_config);
	}
	if (result != RADIO_OK) {
		return result;
	}

	// finally, set the rf path direction
	rf_path_set_direction(&rf_path, rf_path_direction);

	return RADIO_OK;
}

radio_mode_t radio_get_mode(radio_t* radio, const radio_chan_id chan_id)
{
	return radio->channel[chan_id].config.mode;
}

radio_direction_t radio_get_direction(radio_t* radio, const radio_chan_id chan_id)
{
	radio_config_t* config = &radio->channel[chan_id].config;

	switch (config->mode) {
	case RADIO_MODE_RX:
	case RADIO_MODE_RX_SWEEP:
		return RADIO_DIRECTION_RX;
	case RADIO_MODE_TX:
		return RADIO_DIRECTION_TX;
	default:
		return RADIO_DIRECTION_NONE;
	}
}

clock_source_t radio_get_clock_source(radio_t* radio, const radio_chan_id chan_id)
{
	return radio->channel[chan_id].clock_source;
}

radio_error_t radio_set_trigger_mode(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const hw_sync_mode_t value)
{
	(void) mode; // TODO

	radio_config_t* config = &radio->channel[chan_id].config;

	hw_sync_enable(value);

	config->trigger_mode = value;
	return RADIO_OK;
}

hw_sync_mode_t radio_get_trigger_mode(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode)
{
	(void) mode; // TODO

	return radio->channel[chan_id].config.trigger_mode;
}

radio_error_t radio_set_sample_rate_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_sample_rate_id element,
	const radio_sample_rate_t value)
{
	(void) mode; // TODO

	radio_config_t* config = &radio->channel[chan_id].config;

	if (config->mode == RADIO_MODE_OFF) {
		config->sample_rate[element] = value;
		return RADIO_OK;
	}

	// TODO handle individual elements, taking care not
	// to override existing auto-tuned values

	switch (element) {
	case RADIO_SAMPLE_RATE:
		// TODO if we supported this, it would be the same as radio_set_mode_sample_rate()
		return RADIO_ERR_INVALID_PARAM;
	case RADIO_SAMPLE_RATE_MCU:
		if (!sample_rate_frac_set(value.num, value.denom)) {
			return RADIO_ERR_INVALID_PARAM;
		}
		break;
#ifndef PRALINE
	case RADIO_SAMPLE_RATE_RX_DECIMATION_RATIO:
	case RADIO_SAMPLE_RATE_TX_INTERPOLATION_RATIO:
		return RADIO_ERR_UNSUPPORTED_OPERATION;
#else
	case RADIO_SAMPLE_RATE_RX_DECIMATION_RATIO:
		fpga_set_rx_decimation_ratio(&fpga, value.num);
		break;
	case RADIO_SAMPLE_RATE_TX_INTERPOLATION_RATIO:
		fpga_set_tx_interpolation_ratio(&fpga, value.num);
		break;
#endif
	}

	config->sample_rate[element] = value;
	return RADIO_OK;
}

radio_sample_rate_t radio_get_sample_rate_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_sample_rate_id element)
{
	(void) mode; // TODO

	return radio->channel[chan_id].config.sample_rate[element];
}

radio_error_t radio_set_filter_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_filter_id element,
	const radio_filter_t value)
{
	(void) mode; // TODO

	radio_config_t* config = &radio->channel[chan_id].config;

	if (config->mode == RADIO_MODE_OFF) {
		config->filter[element] = value;
		return RADIO_OK;
	}

	uint32_t real_hz;
	switch (element) {
	case RADIO_FILTER_BASEBAND_LPF:
#ifndef PRALINE
		real_hz = max283x_set_lpf_bandwidth(&max283x, value.hz);
#else
		real_hz = max2831_set_lpf_bandwidth(&max283x, value.hz);
#endif
		if (real_hz == 0) {
			return RADIO_ERR_INVALID_PARAM;
		}
		config->filter[element] = (radio_filter_t){.hz = real_hz};
		break;

	case RADIO_FILTER_RF_PATH:
		// TODO only get, I don't think we want to let folk set this. or maybe ?
		break;

	case RADIO_FILTER_RX_BASEBAND_HPF:
#ifdef PRALINE
		max2831_set_rx_hpf_frequency(&max283x, value.mode);
		config->filter[element] = (radio_filter_t){.mode = value.mode & 0b11};
#endif
		break;

	case RADIO_FILTER_RX_NARROWBAND_LPF:
#ifdef PRALINE
		narrowband_filter_set(value.enable);
		config->filter[element] = (radio_filter_t){.enable = value.enable & 0b1};
#endif
		break;

	case RADIO_FILTER_RX_DC_BLOCK:
#ifdef PRALINE
		fpga_set_rx_dc_block_enable(&fpga, value.enable);
		config->filter[element] = (radio_filter_t){.enable = value.enable & 0b1};
#endif
		break;
	}

	return RADIO_OK;
}

radio_filter_t radio_get_filter_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_filter_id element)
{
	(void) mode; // TODO

	return radio->channel[chan_id].config.filter[element];
}

radio_error_t radio_supported_filter_element_bandwidths(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_filter_id element,
	uint32_t* list,
	size_t* length)
{
	(void) radio;
	(void) chan_id;

#ifndef PRALINE
	(void) mode;
	const max2837_ft_t* ft = NULL;
	switch (element) {
	case RADIO_FILTER_BASEBAND_LPF:
		ft = max2837_ft;
		break;
	case RADIO_FILTER_RF_PATH:
		// TODO do we want to allow this to be set from outside?
		*length = 0;
		return RADIO_OK;
	case RADIO_FILTER_RX_BASEBAND_HPF:
	case RADIO_FILTER_RX_NARROWBAND_LPF:
	case RADIO_FILTER_RX_DC_BLOCK:
		// not supported on HackRF One
		*length = 0;
		return RADIO_OK;
	}
#else
	const max2831_ft_t* ft = NULL;
	switch (element) {
	case RADIO_FILTER_BASEBAND_LPF:
		if (mode == RADIO_MODE_RX || mode == RADIO_MODE_RX_SWEEP) {
			ft = max2831_rx_ft;
		} else {
			ft = max2831_tx_ft;
		}
		break;
	case RADIO_FILTER_RF_PATH:
		// TODO do we want to allow this to be set from outside?
		*length = 0;
		return RADIO_OK;
	case RADIO_FILTER_RX_BASEBAND_HPF:
		// TODO do we want to allow this to be set from outside?
		*length = 0;
		return RADIO_OK;
	case RADIO_FILTER_RX_NARROWBAND_LPF:
		list[0] = 1500000; // Measured bandwidth is 1.5 MHz.
		*length = 1;
		return RADIO_OK;
	case RADIO_FILTER_RX_DC_BLOCK:
		// TODO should we return [0, 1] ?
		*length = 0;
		return RADIO_OK;
	}
#endif

	size_t count = 0;
	for (; ft->bandwidth_hz != 0; ft++, count++) {
		list[count] = ft->bandwidth_hz;
	}
	*length = count;

	return RADIO_OK;
}

radio_error_t radio_set_frequency_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_frequency_id element,
	const radio_frequency_t value)
{
	(void) mode; // TODO
	radio_error_t result;

	radio_config_t* config = &radio->channel[chan_id].config;

	if (config->mode == RADIO_MODE_OFF) {
		config->frequency[element] = value;
		return RADIO_OK;
	}

	if (element == RADIO_FREQUENCY) {
		// TODO if we supported this, it would be the same as radio_set_mode_sample_rate()
		return RADIO_ERR_INVALID_PARAM;
	}

	// update element and get full configuration
	config->frequency[element] = value;
	radio_mode_frequency_t frequency_config =
		radio_get_mode_frequency(radio, chan_id, mode);

	// retune
	result =
		radio_set_mode_frequency_explicit(radio, chan_id, mode, frequency_config);

	return result;
}

radio_frequency_t radio_get_frequency_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_frequency_id element)
{
	(void) mode; // TODO

	return radio->channel[chan_id].config.frequency[element];
}

radio_error_t radio_set_gain_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_gain_id element,
	const radio_gain_t value)
{
	(void) mode; // TODO

	radio_config_t* config = &radio->channel[chan_id].config;

	if (config->mode == RADIO_MODE_OFF) {
		config->gain[element] = value;
		return RADIO_OK;
	}

	uint8_t real_db;
	switch (element) {
	case RADIO_GAIN_RF_AMP:
		rf_path_set_lna(&rf_path, value.enable);
		break;
	case RADIO_GAIN_RX_LNA:
#ifndef PRALINE
		real_db = max283x_set_lna_gain(&max283x, value.db);
#else
		real_db = max2831_set_lna_gain(&max283x, value.db);
#endif
		if (real_db == 0) {
			return RADIO_ERR_INVALID_PARAM;
		}
		break;
	case RADIO_GAIN_RX_VGA:
#ifndef PRALINE
		real_db = max283x_set_vga_gain(&max283x, value.db);
#else
		real_db = max2831_set_vga_gain(&max283x, value.db);
#endif
		if (real_db == 0) {
			return RADIO_ERR_INVALID_PARAM;
		}
		break;
	case RADIO_GAIN_TX_VGA:
#ifndef PRALINE
		real_db = max283x_set_txvga_gain(&max283x, value.db);
#else
		real_db = max2831_set_txvga_gain(&max283x, value.db);
#endif
		if (real_db == 0) {
			return RADIO_ERR_INVALID_PARAM;
		}
		break;
	}

	config->gain[element] = value;
	return RADIO_OK;
}

radio_gain_t radio_get_gain_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_gain_id element)
{
	(void) mode; // TODO

	return radio->channel[chan_id].config.gain[element];
}

radio_error_t radio_set_antenna_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_antenna_id element,
	const radio_antenna_t value)
{
	(void) mode; // TODO

	if (element > RADIO_ANTENNA_COUNT) {
		return RADIO_ERR_INVALID_ELEMENT;
	}

	radio_config_t* config = &radio->channel[chan_id].config;

	if (config->mode == RADIO_MODE_OFF) {
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

radio_antenna_t radio_get_antenna_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_antenna_id element)
{
	(void) mode; // TODO

	return radio->channel[chan_id].config.antenna[element];
}

radio_error_t radio_set_clock_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_clock_id element,
	const radio_clock_t value)
{
	(void) mode; // TODO

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

radio_clock_t radio_get_clock_element(
	radio_t* radio,
	const radio_chan_id chan_id,
	const radio_mode_t mode,
	const radio_clock_id element)
{
	(void) mode; // TODO

	if (element == RADIO_CLOCK_CLKIN) {
		return (radio_clock_t){
			.enable = si5351c_clkin_signal_valid(&clock_gen),
		};
	}

	return radio->channel[chan_id].config.clock[element];
}
