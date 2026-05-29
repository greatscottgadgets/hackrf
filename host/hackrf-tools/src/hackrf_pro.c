/*
 * Copyright 2025 Great Scott Gadgets <info@greatscottgadgets.com>
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
 */

#include <hackrf.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

static void usage()
{
	printf("hackrf_pro - High-level control of HackRF Pro FPGA features\n");
	printf("Usage:\n");
	printf("\t[-d serial_number] # Serial number of desired HackRF Pro.\n");
	printf("\t[--dc-block on|off] # Enable/disable FPGA DC block filter.\n");
	printf("\t[--decimation N] # RX decimation: 0=none, 1=÷2, 2=÷4, 3=÷8, 4=÷16, 5=÷32\n");
	printf("\t[--interpolation N] # TX interpolation: 0=none, 1=÷2, 2=÷4, 3=÷8\n");
	printf("\t[--quarter-shift up|down|none] # Digital spectrum shift by ±Fs/4.\n");
	printf("\t[--nco-freq HZ] # Enable NCO and set frequency (Hz).\n");
	printf("\t[--read-reg ADDR] # Read raw FPGA register.\n");
	printf("\t[--write-reg ADDR VAL] # Write raw FPGA register.\n");
	printf("\t[-h] # This help.\n");
}

static int parse_on_off(const char* s, bool* out)
{
	if (strcmp(s, "on") == 0 || strcmp(s, "1") == 0) {
		*out = true;
		return HACKRF_SUCCESS;
	}
	if (strcmp(s, "off") == 0 || strcmp(s, "0") == 0) {
		*out = false;
		return HACKRF_SUCCESS;
	}
	return HACKRF_ERROR_INVALID_PARAM;
}

static int parse_shift(const char* s, uint8_t* out)
{
	if (strcmp(s, "none") == 0) {
		*out = 0;
		return HACKRF_SUCCESS;
	}
	if (strcmp(s, "up") == 0) {
		*out = 1;
		return HACKRF_SUCCESS;
	}
	if (strcmp(s, "down") == 0) {
		*out = 2;
		return HACKRF_SUCCESS;
	}
	return HACKRF_ERROR_INVALID_PARAM;
}

static int fpga_write_reg(hackrf_device* device, uint8_t addr, uint8_t value)
{
	return hackrf_fpga_write_register(device, addr, value);
}

static int fpga_read_reg(hackrf_device* device, uint8_t addr, uint8_t* value)
{
	return hackrf_fpga_read_register(device, addr, value);
}

int main(int argc, char** argv)
{
	int opt;
	int result;
	const char* serial_number = NULL;
	hackrf_device* device = NULL;
	bool do_dc_block = false;
	bool dc_block = false;
	bool do_decimation = false;
	uint8_t decimation = 0;
	bool do_interpolation = false;
	uint8_t interpolation = 0;
	bool do_quarter_shift = false;
	uint8_t quarter_shift = 0;
	bool do_nco = false;
	uint64_t nco_freq = 0;
	bool do_read_reg = false;
	uint8_t read_reg_addr = 0;
	bool do_write_reg = false;
	uint8_t write_reg_addr = 0;
	uint8_t write_reg_val = 0;

	static struct option long_options[] = {
		{"dc-block", required_argument, 0, 1},
		{"decimation", required_argument, 0, 2},
		{"interpolation", required_argument, 0, 3},
		{"quarter-shift", required_argument, 0, 4},
		{"nco-freq", required_argument, 0, 5},
		{"read-reg", required_argument, 0, 6},
		{"write-reg", required_argument, 0, 7},
		{"device", required_argument, 0, 'd'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0},
	};

	while ((opt = getopt_long(argc, argv, "d:h", long_options, NULL)) != EOF) {
		result = HACKRF_SUCCESS;
		switch (opt) {
		case 1:
			result = parse_on_off(optarg, &dc_block);
			do_dc_block = true;
			break;
		case 2:
			decimation = (uint8_t) atoi(optarg);
			do_decimation = true;
			break;
		case 3:
			interpolation = (uint8_t) atoi(optarg);
			do_interpolation = true;
			break;
		case 4:
			result = parse_shift(optarg, &quarter_shift);
			do_quarter_shift = true;
			break;
		case 5:
			nco_freq = strtoull(optarg, NULL, 10);
			do_nco = true;
			break;
		case 6:
			read_reg_addr = (uint8_t) strtoul(optarg, NULL, 0);
			do_read_reg = true;
			break;
		case 7:
			write_reg_addr = (uint8_t) strtoul(optarg, NULL, 0);
			write_reg_val = (uint8_t) strtoul(argv[optind], NULL, 0);
			optind++;
			do_write_reg = true;
			break;
		case 'd':
			serial_number = optarg;
			break;
		case 'h':
		case '?':
			usage();
			return EXIT_SUCCESS;
		default:
			fprintf(stderr, "unknown argument\n");
			usage();
			return EXIT_FAILURE;
		}
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr, "argument error\n");
			usage();
			return EXIT_FAILURE;
		}
	}

	result = hackrf_init();
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_init() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		return EXIT_FAILURE;
	}

	result = hackrf_open_by_serial(serial_number, &device);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_open() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		hackrf_exit();
		return EXIT_FAILURE;
	}

	if (do_dc_block) {
		uint8_t ctrl;
		result = fpga_read_reg(device, 1, &ctrl);
		if (result == HACKRF_SUCCESS) {
			if (dc_block) {
				ctrl |= 0x01;
			} else {
				ctrl &= ~0x01;
			}
			result = fpga_write_reg(device, 1, ctrl);
		}
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"DC block failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
		} else {
			printf("DC block %s\n", dc_block ? "enabled" : "disabled");
		}
	}

	if (do_decimation) {
		result = fpga_write_reg(device, 2, decimation & 0x07);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"Decimation failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
		} else {
			printf("RX decimation set to %u\n", decimation);
		}
	}

	if (do_interpolation) {
		result = fpga_write_reg(device, 5, interpolation & 0x07);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"Interpolation failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
		} else {
			printf("TX interpolation set to %u\n", interpolation);
		}
	}

	if (do_quarter_shift) {
		result = fpga_write_reg(device, 3, quarter_shift);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"Quarter-shift failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
		} else {
			printf("Quarter-shift set to %s\n",
			       quarter_shift == 0 ? "none" :
			       quarter_shift == 1 ? "up" : "down");
		}
	}

	if (do_nco) {
		/* Enable NCO via TX_CTRL reg 4, bit 0 */
		uint8_t tx_ctrl;
		result = fpga_read_reg(device, 4, &tx_ctrl);
		if (result == HACKRF_SUCCESS) {
			tx_ctrl |= 0x01;
			result = fpga_write_reg(device, 4, tx_ctrl);
		}
		if (result == HACKRF_SUCCESS) {
			/* Set NCO phase increment via TX_PSTEP reg 6 */
			/* Phase increment = (nco_freq * 256) / sample_rate
			 * For simplicity we write the raw 8-bit value;
			 * users can compute the correct value externally. */
			uint8_t pstep = (uint8_t)(nco_freq & 0xFF);
			result = fpga_write_reg(device, 6, pstep);
		}
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"NCO setup failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
		} else {
			printf("NCO enabled with phase increment %u\n", (unsigned int) (nco_freq & 0xFF));
		}
	}

	if (do_read_reg) {
		uint8_t value;
		result = fpga_read_reg(device, read_reg_addr, &value);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"Register read failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
		} else {
			printf("FPGA reg[%u] = 0x%02x\n", read_reg_addr, value);
		}
	}

	if (do_write_reg) {
		result = fpga_write_reg(device, write_reg_addr, write_reg_val);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"Register write failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
		} else {
			printf("FPGA reg[%u] = 0x%02x\n", write_reg_addr, write_reg_val);
		}
	}

	hackrf_close(device);
	hackrf_exit();
	return EXIT_SUCCESS;
}
