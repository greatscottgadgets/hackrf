/*
 * Copyright 2017-2022 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2017 Dominic Spill <dominicgs@gmail.com>
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

#include <hackrf.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#ifndef bool
typedef int bool;
	#define true 1
	#define false 0
#endif

#define CLOCK_UNDEFINED  0xFF
#define REGISTER_INVALID 32767

int parse_int(char* s, uint8_t* const value)
{
	uint_fast8_t base = 10;
	char* s_end;
	long long_value;

	if (strlen(s) > 2) {
		if (s[0] == '0') {
			if ((s[1] == 'x') || (s[1] == 'X')) {
				base = 16;
				s += 2;
			} else if ((s[1] == 'b') || (s[1] == 'B')) {
				base = 2;
				s += 2;
			}
		}
	}

	s_end = s;
	long_value = strtol(s, &s_end, base);
	if ((s != s_end) && (*s_end == 0)) {
		*value = (uint8_t) long_value;
		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_INVALID_PARAM;
	}
}

int si5351c_read_register(hackrf_device* device, const uint16_t register_number)
{
	uint16_t register_value;
	int result = hackrf_si5351c_read(device, register_number, &register_value);

	if (result == HACKRF_SUCCESS) {
		printf("[%3d] -> 0x%02x\n", register_number, register_value);
	} else {
		printf("hackrf_si5351c_read() failed: %s (%d)\n",
		       hackrf_error_name(result),
		       result);
	}

	return result;
}

int si5351c_write_register(
	hackrf_device* device,
	const uint16_t register_number,
	const uint16_t register_value)
{
	int result = HACKRF_SUCCESS;
	result = hackrf_si5351c_write(device, register_number, register_value);

	if (result == HACKRF_SUCCESS) {
		printf("0x%2x -> [%3d]\n", register_value, register_number);
	} else {
		printf("hackrf_max2837_write() failed: %s (%d)\n",
		       hackrf_error_name(result),
		       result);
	}

	return result;
}

#define SI5351C_CLK_POWERDOWN           (1 << 7)
#define SI5351C_CLK_INT_MODE            (1 << 6)
#define SI5351C_CLK_PLL_SRC             (1 << 5)
#define SI5351C_CLK_INV                 (1 << 4)
#define SI5351C_CLK_SRC_XTAL            0
#define SI5351C_CLK_SRC_CLKIN           1
#define SI5351C_CLK_SRC_MULTISYNTH_0_4  2
#define SI5351C_CLK_SRC_MULTISYNTH_SELF 3

void print_clk_control(uint16_t clk_ctrl)
{
	uint8_t clk_src, clk_pwr;
	printf("\tclock control = ");
	if (clk_ctrl & SI5351C_CLK_POWERDOWN) {
		printf("Down, ");
	} else {
		printf("Up, ");
	}
	if (clk_ctrl & SI5351C_CLK_INT_MODE) {
		printf("Int Mode, ");
	} else {
		printf("Frac Mode, ");
	}
	if (clk_ctrl & SI5351C_CLK_PLL_SRC) {
		printf("PLL src B, ");
	} else {
		printf("PLL src A, ");
	}
	if (clk_ctrl & SI5351C_CLK_INV) {
		printf("Inverted, ");
	}
	clk_src = (clk_ctrl >> 2) & 0x3;
	switch (clk_src) {
	case 0:
		printf("XTAL, ");
		break;
	case 1:
		printf("CLKIN, ");
		break;
	case 2:
		printf("MULTISYNTH 0 4, ");
		break;
	case 3:
		printf("MULTISYNTH SELF, ");
		break;
	}
	clk_pwr = clk_ctrl & 0x3;
	switch (clk_pwr) {
	case 0:
		printf("2 mA\n");
		break;
	case 1:
		printf("4 mA\n");
		break;
	case 2:
		printf("6 mA\n");
		break;
	case 3:
		printf("8 mA\n");
		break;
	}
}

int si5351c_read_multisynth_config(hackrf_device* device, const uint_fast8_t ms_number)
{
	uint_fast8_t i, reg_base, reg_number;
	uint16_t parameters[8], clk_control;
	uint32_t p1, p2, p3, r_div;
	uint_fast8_t div_lut[] = {1, 2, 4, 8, 16, 32, 64, 128};
	int result;

	printf("MS%d:\n", ms_number);
	result = hackrf_si5351c_read(device, 16 + ms_number, &clk_control);
	if (result != HACKRF_SUCCESS) {
		return result;
	}
	print_clk_control(clk_control);
	if (ms_number < 6) {
		reg_base = 42 + (ms_number * 8);
		for (i = 0; i < 8; i++) {
			reg_number = reg_base + i;
			result = hackrf_si5351c_read(device, reg_number, &parameters[i]);
			if (result != HACKRF_SUCCESS) {
				return result;
			}
		}

		p1 = ((parameters[2] & 0x03) << 16) | (parameters[3] << 8) |
			parameters[4];
		p2 = ((parameters[5] & 0x0F) << 16) | (parameters[6] << 8) |
			parameters[7];
		p3 = ((parameters[5] & 0xF0) << 12) | (parameters[0] << 8) |
			parameters[1];
		r_div = (parameters[2] >> 4) & 0x7;

		printf("\tp1 = %u\n", p1);
		printf("\tp2 = %u\n", p2);
		printf("\tp3 = %u\n", p3);
		if (p3) {
			printf("\tOutput (800Mhz PLL): %#.10f Mhz\n",
			       ((double) 800 /
				(double) (((double) p1 * p3 + p2 + 512 * p3) / (double) (128 * p3))) /
				       div_lut[r_div]);
		}
	} else {
		// MS6 and 7 are integer only
		unsigned int parms;
		reg_base = 90;

		for (i = 0; i < 3; i++) {
			uint_fast8_t reg_number = reg_base + i;
			int result =
				hackrf_si5351c_read(device, reg_number, &parameters[i]);
			if (result != HACKRF_SUCCESS) {
				return result;
			}
		}
		r_div = (ms_number == 6) ? parameters[2] & 0x7 :
					   (parameters[2] & 0x70) >> 4;
		parms = (ms_number == 6) ? parameters[0] : parameters[1];
		printf("\tp1_int = %u\n", parms);
		if (parms) {
			printf("\tOutput (800Mhz PLL): %#.10f Mhz\n",
			       (800.0f / parms) / div_lut[r_div]);
		}
	}
	printf("\toutput divider = %u\n", div_lut[r_div]);
	return HACKRF_SUCCESS;
}

int si5351c_read_configuration(hackrf_device* device)
{
	uint_fast8_t ms_number;
	int result;

	for (ms_number = 0; ms_number < 8; ms_number++) {
		result = si5351c_read_multisynth_config(device, ms_number);
		if (result != HACKRF_SUCCESS) {
			return result;
		}
	}
	return HACKRF_SUCCESS;
}

static void usage()
{
	printf("hackrf_clock - HackRF clock configuration utility\n");
	printf("Usage:\n");
	printf("\t-h, --help: this help\n");
	printf("\t-r, --read <clock_num>: read settings for clock_num\n");
	printf("\t-a, --all: read settings for all clocks\n");
	printf("\t-i, --clkin: get CLKIN status\n");
	printf("\t-o, --clkout <clkout_enable>: enable/disable CLKOUT\n");
	printf("\t-d, --device <serial_number>: Serial number of desired HackRF.\n");
	printf("\nExamples:\n");
	printf("\thackrf_clock -r 3 : prints settings for CLKOUT\n");
}

static struct option long_options[] = {
	{"help", no_argument, 0, 'h'},
	{"read", required_argument, 0, 'r'},
	{"all", no_argument, 0, 'a'},
	{"clkin", required_argument, 0, 'i'},
	{"clkout", required_argument, 0, 'o'},
	{"device", required_argument, 0, 'd'},
	{0, 0, 0, 0},
};

int main(int argc, char** argv)
{
	hackrf_device* device = NULL;
	int opt, option_index = 0;
	bool read = false;
	uint8_t clock = CLOCK_UNDEFINED;
	bool clkout = false;
	bool clkin = false;
	uint8_t clkout_enable;
	uint8_t clkin_status;
	const char* serial_number = NULL;

	int result = hackrf_init();
	if (result) {
		printf("hackrf_init() failed: %s (%d)\n",
		       hackrf_error_name(result),
		       result);
		return EXIT_FAILURE;
	}

	while ((opt = getopt_long(argc, argv, "r:aio:d:h?", long_options, &option_index)) !=
	       EOF) {
		switch (opt) {
		case 'r':
			read = true;
			result = parse_int(optarg, &clock);
			break;

		case 'a':
			read = true;
			break;

		case 'i':
			clkin = true;
			break;

		case 'o':
			clkout = true;
			result = parse_int(optarg, &clkout_enable);
			break;

		case 'd':
			serial_number = optarg;
			break;

		case 'h':
		case '?':
			usage();
			return EXIT_SUCCESS;
		default:
			fprintf(stderr, "unknown argument '-%c %s'\n", opt, optarg);
			usage();
			return EXIT_FAILURE;
		}

		if (result != HACKRF_SUCCESS) {
			printf("argument error: %s (%d)\n",
			       hackrf_error_name(result),
			       result);
			usage();
			return EXIT_FAILURE;
		}
	}

	if (!clkin && !clkout && !read) {
		fprintf(stderr, "An operation must be specified.\n");
		usage();
		return EXIT_FAILURE;
	}

	result = hackrf_open_by_serial(serial_number, &device);
	if (result) {
		printf("hackrf_open() failed: %s (%d)\n",
		       hackrf_error_name(result),
		       result);
		return EXIT_FAILURE;
	}

	if (clkout) {
		result = hackrf_set_clkout_enable(device, clkout_enable);
		if (result) {
			printf("hackrf_set_clkout_enable() failed: %s (%d)\n",
			       hackrf_error_name(result),
			       result);
			return EXIT_FAILURE;
		}
	}

	if (clkin) {
		result = hackrf_get_clkin_status(device, &clkin_status);
		if (result) {
			printf("hackrf_get_clkin_status() failed: %s (%d)\n",
			       hackrf_error_name(result),
			       result);
			return EXIT_FAILURE;
		}
		printf("CLKIN status: %s\n",
		       clkin_status ? "clock signal detected" :
				      "no clock signal detected");
	}

	if (read) {
		if (clock == CLOCK_UNDEFINED) {
			si5351c_read_configuration(device);
		} else {
			printf("%d\n", clock);
			si5351c_read_multisynth_config(device, clock);
		}
	}

	result = hackrf_close(device);
	if (result) {
		printf("hackrf_close() failed: %s (%d)\n",
		       hackrf_error_name(result),
		       result);
		return EXIT_FAILURE;
	}

	hackrf_exit();
	return EXIT_SUCCESS;
}
