/*
 * Copyright 2025 Great Scott Gadgets <info@greatscottgadgets.com>
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
#include "m0_state.h"
#include "streaming.h"
#include "selftest.h"
#include "fpga.h"
#include "delay.h"

// USB buffer used during selftests.
#define USB_BULK_BUFFER_SIZE 0x8000
extern uint8_t usb_bulk_buffer[USB_BULK_BUFFER_SIZE];

static int rx_samples(const unsigned int num_samples, uint32_t max_cycles)
{
	uint32_t cycle_count = 0;
	int rc = 0;

	m0_set_mode(M0_MODE_RX);
	m0_state.shortfall_limit = 0;
	baseband_streaming_enable(&sgpio_config);
	while (m0_state.m0_count < num_samples) {
		cycle_count++;
		if ((max_cycles > 0) && (cycle_count >= max_cycles)) {
			rc = -1;
			break;
		}
	}
	baseband_streaming_disable(&sgpio_config);
	m0_set_mode(M0_MODE_IDLE);

	return rc;
}

bool fpga_spi_selftest(void)
{
	// Skip if FPGA configuration failed.
	if (selftest.fpga_image_load != PASSED) {
		selftest.fpga_spi = SKIPPED;
		return false;
	}

	// Test writing a register and reading it back.
	uint8_t reg = 5;
	uint8_t write_value = 0xA5;
	ssp1_set_mode_ice40();
	ice40_spi_write(&ice40, reg, write_value);
	uint8_t read_value = ice40_spi_read(&ice40, reg);
	ssp1_set_mode_max283x();

	// Update selftest result.
	selftest.fpga_spi = (read_value == write_value) ? PASSED : FAILED;
	if (selftest.fpga_spi != PASSED) {
		selftest.report.pass = false;
	}

	return selftest.fpga_spi == PASSED;
}

static uint8_t lfsr_advance(uint8_t v)
{
	const uint8_t feedback = ((v >> 3) ^ (v >> 4) ^ (v >> 5) ^ (v >> 7)) & 1;
	return (v << 1) | feedback;
}

bool fpga_sgpio_selftest(void)
{
	bool timeout = false;

	// Skip if FPGA configuration failed or its SPI bus is not working.
	if ((selftest.fpga_image_load != PASSED) || (selftest.fpga_spi != PASSED)) {
		selftest.sgpio_rx = SKIPPED;
		return false;
	}

	// Enable PRBS mode.
	fpga_set_prbs_enable(&fpga, true);

	// Stream 512 samples from the FPGA.
	sgpio_configure(&sgpio_config, SGPIO_DIRECTION_RX);
	if (rx_samples(512, 10000) == -1) {
		timeout = true;
	}

	// Disable PRBS mode.
	fpga_set_prbs_enable(&fpga, false);

	// Generate sequence from first value and compare.
	bool seq_in_sync = (usb_bulk_buffer[0] != 0);
	uint8_t seq = lfsr_advance(usb_bulk_buffer[0]);
	for (int i = 1; i < 512; ++i) {
		if (usb_bulk_buffer[i] != seq) {
			seq_in_sync = false;
			break;
		}
		seq = lfsr_advance(seq);
	}

	// Update selftest result.
	if (seq_in_sync) {
		selftest.sgpio_rx = PASSED;
	} else if (timeout) {
		selftest.sgpio_rx = TIMEOUT;
	} else {
		selftest.sgpio_rx = FAILED;
	}
	if (selftest.sgpio_rx != PASSED) {
		selftest.report.pass = false;
	}

	return selftest.sgpio_rx == PASSED;
}

static void measure_tone(int8_t* samples, size_t len, struct xcvr_measurements* results)
{
	results->zcs_i = 0;
	results->zcs_q = 0;
	results->max_mag_i = 0;
	results->max_mag_q = 0;
	results->avg_mag_sq_i = 0;
	results->avg_mag_sq_q = 0;
	uint8_t last_sign_i = 0;
	uint8_t last_sign_q = 0;
	for (size_t i = 0; i < len; i += 2) {
		int8_t sample_i = samples[i];
		int8_t sample_q = samples[i + 1];
		uint8_t sign_i = sample_i < 0 ? 1 : 0;
		uint8_t sign_q = sample_q < 0 ? 1 : 0;
		results->zcs_i += sign_i ^ last_sign_i;
		results->zcs_q += sign_q ^ last_sign_q;
		last_sign_i = sign_i;
		last_sign_q = sign_q;
		uint8_t mag_i = sign_i ? -sample_i : sample_i;
		uint8_t mag_q = sign_q ? -sample_q : sample_q;
		if (mag_i > results->max_mag_i)
			results->max_mag_i = mag_i;
		if (mag_q > results->max_mag_q)
			results->max_mag_q = mag_q;
		results->avg_mag_sq_i += mag_i * mag_i;
		results->avg_mag_sq_q += mag_q * mag_q;
	}
	results->avg_mag_sq_i /= (len / 2);
	results->avg_mag_sq_q /= (len / 2);
}

static bool in_range(int value, int expected, int error)
{
	int max = expected * (100 + error) / 100;
	int min = expected * (100 - error) / 100;
	return (value > min) && (value < max);
}

bool fpga_if_xcvr_selftest(void)
{
	bool timeout = false;

	// Skip if FPGA configuration failed or its SPI bus is not working.
	if ((selftest.fpga_image_load != PASSED) || (selftest.fpga_spi != PASSED)) {
		selftest.xcvr_loopback = SKIPPED;
		return false;
	}

	const size_t num_samples = USB_BULK_BUFFER_SIZE / 2;

	// Set common RX path and gateware settings for the measurements.
	fpga_set_tx_nco_pstep(&fpga, 64);    // NCO phase increment
	fpga_set_tx_nco_enable(&fpga, true); // TX enable
	rf_path_set_direction(&rf_path, RF_PATH_DIRECTION_RX_CALIBRATION);
	max2831_set_lna_gain(&max283x, 16);
	max2831_set_vga_gain(&max283x, 36);
	max2831_set_frequency(&max283x, 2500000000);

	// Capture 1: 4 Msps, tone at 0.5 MHz, narrowband filter OFF
	sample_rate_frac_set(4000000 * 2, 1);
	delay_us_at_mhz(1000, 204);
	if (rx_samples(num_samples, 2000000) == -1) {
		timeout = true;
	}
	measure_tone(
		(int8_t*) usb_bulk_buffer,
		num_samples,
		&selftest.xcvr_measurements[0]);

	// Capture 2: 4 Msps, tone at 0.5 MHz, narrowband filter ON
	narrowband_filter_set(1);
	delay_us_at_mhz(1000, 204);
	if (rx_samples(num_samples, 2000000) == -1) {
		timeout = true;
	}
	measure_tone(
		(int8_t*) usb_bulk_buffer,
		num_samples,
		&selftest.xcvr_measurements[1]);

	// Capture 3: 20 Msps, tone at 5 MHz, narrowband filter OFF
	fpga_set_tx_nco_pstep(&fpga, 255);
	sample_rate_frac_set(20000000 * 2, 1);
	narrowband_filter_set(0);
	delay_us_at_mhz(1000, 204);
	if (rx_samples(num_samples, 2000000) == -1) {
		timeout = true;
	}
	measure_tone(
		(int8_t*) usb_bulk_buffer,
		num_samples,
		&selftest.xcvr_measurements[2]);

	// Capture 4: 20 Msps, tone at 5 MHz, narrowband filter ON
	narrowband_filter_set(1);
	delay_us_at_mhz(1000, 204);
	if (rx_samples(num_samples, 2000000) == -1) {
		timeout = true;
	}
	measure_tone(
		(int8_t*) usb_bulk_buffer,
		num_samples,
		&selftest.xcvr_measurements[3]);

	// Restore default settings.
	sample_rate_set(10000000);
	rf_path_set_direction(&rf_path, RF_PATH_DIRECTION_OFF);
	narrowband_filter_set(0);
	fpga_init(&fpga);

	if (timeout) {
		selftest.xcvr_loopback = TIMEOUT;
		selftest.report.pass = false;
		return false;
	}

	unsigned int expected_zcs;
	bool i_in_range;
	bool q_in_range;
	bool i_energy_in_range;
	bool q_energy_in_range;

	// Capture 0:
	// Count zero crossings.
	// N/2 samples/channel * 2 zcs/cycle / 16 samples/cycle = N/16 zcs/channel
	expected_zcs = num_samples / 16;
	i_in_range = in_range(selftest.xcvr_measurements[0].zcs_i, expected_zcs, 5);
	q_in_range = in_range(selftest.xcvr_measurements[0].zcs_q, expected_zcs, 5);
	i_energy_in_range = (selftest.xcvr_measurements[0].avg_mag_sq_i > 500) &&
		(selftest.xcvr_measurements[0].avg_mag_sq_i < 2500);
	q_energy_in_range = (selftest.xcvr_measurements[0].avg_mag_sq_q > 500) &&
		(selftest.xcvr_measurements[0].avg_mag_sq_q < 2500);
	bool capture_0_test =
		i_in_range && q_in_range && i_energy_in_range && q_energy_in_range;

	// Capture 1:
	// Count zero crossings.
	expected_zcs = num_samples / 16;
	i_in_range = in_range(selftest.xcvr_measurements[1].zcs_i, expected_zcs, 5);
	q_in_range = in_range(selftest.xcvr_measurements[1].zcs_q, expected_zcs, 5);
	i_energy_in_range = (selftest.xcvr_measurements[1].avg_mag_sq_i > 500) &&
		(selftest.xcvr_measurements[1].avg_mag_sq_i < 2500);
	q_energy_in_range = (selftest.xcvr_measurements[1].avg_mag_sq_q > 500) &&
		(selftest.xcvr_measurements[1].avg_mag_sq_q < 2500);
	bool capture_1_test =
		i_in_range && q_in_range && i_energy_in_range && q_energy_in_range;

	// Capture 2:
	// Count zero crossings.
	expected_zcs = num_samples / 4;
	i_in_range = in_range(selftest.xcvr_measurements[2].zcs_i, expected_zcs, 5);
	q_in_range = in_range(selftest.xcvr_measurements[2].zcs_q, expected_zcs, 5);
	i_energy_in_range = (selftest.xcvr_measurements[2].avg_mag_sq_i > 400) &&
		(selftest.xcvr_measurements[2].avg_mag_sq_i < 2000);
	q_energy_in_range = (selftest.xcvr_measurements[2].avg_mag_sq_q > 400) &&
		(selftest.xcvr_measurements[2].avg_mag_sq_q < 2000);
	bool capture_2_test =
		i_in_range && q_in_range && i_energy_in_range && q_energy_in_range;

	// Capture 3:
	i_energy_in_range = (selftest.xcvr_measurements[3].avg_mag_sq_i < 100);
	q_energy_in_range = (selftest.xcvr_measurements[3].avg_mag_sq_q < 100);
	bool capture_3_test = i_energy_in_range && q_energy_in_range;

	// Update selftest result.
	selftest.xcvr_loopback =
		(capture_0_test && capture_1_test && capture_2_test && capture_3_test) ?
		PASSED :
		FAILED;
	if (selftest.xcvr_loopback != PASSED) {
		selftest.report.pass = false;
	}

	return selftest.xcvr_loopback == PASSED;
}
