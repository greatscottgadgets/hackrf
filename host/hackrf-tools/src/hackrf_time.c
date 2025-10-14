/*
 * Copyright 2012-2022 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2012 Jared Boone <jared@sharebrained.com>
 * Copyright 2013 Benjamin Vernoux <titanmkd@gmail.com>
 * Copyright 2017 Dominic Spill <dominicgs@gmail.com>
 * Copyright 2025 Fabrizio Pollastri <mxgbot@gmail.com>
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
	#define true  1
	#define false 0
#endif

int parse_int(char* s, uint32_t* const value)
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
		*value = (uint32_t) long_value;
		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_INVALID_PARAM;
	}
}

int parse_double(char* s, double* const double_value)
{
	char* s_end;

	s_end = s + strlen(s);
	*double_value = strtof(s, &s_end);
	if ((s != s_end) && (*s_end == 0)) {
		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_INVALID_PARAM;
	}
}

static void usage()
{
	printf("Manage HTime for HackRF: ticks, seconds, A/D trigger, clocks.\n");
	printf("\nUsage:\n");
	printf("\t-h, --help: this help\n");
	printf("\t-d, --divisor <v>: set divisor to <v> val next pps\n");
	printf("\t-D, --Divisor <v>: set divisor to <v> val for one pps\n");
	printf("\t-f, --set_clk_freq <v>: set sync clock freq to <v>\n");
	printf("\t-k, --ticks : read ticks counter now\n");
	printf("\t-K, --Ticks : set ticks counter now\n");
	printf("\t-r, --trig_delay <v>: set trig delay to <v> val next pps\n");
	printf("\t-s, --seconds <v>: set seconds counter now to <v> value\n");
	printf("\t-S, --Seconds <v>: as -s but in sync to next pps\n");
	printf("\t-t, --get_seconds: read seconds counter now\n");
	printf("\t-y, --mcu_clk_sync <0/1>: enable/disable mcu clock sync\n");
	printf("\nExamples:\n");
	printf("\thackrf_time -s 1234     # set seconds counter to 1234 now\n");
	printf("\thackrf_time -y 1        # enable MCU sync mode\n");

	printf("\nv0.1.0 20250215 F.P. <mxgbot@gmail.com>\n");
}

static struct option long_options[] = {
	{"help", no_argument, 0, 'h'},
	{"divisor", required_argument, 0, 'd'},
	{"Divisor", required_argument, 0, 'D'},
	{"set_clk_freq", required_argument, 0, 'f'},
	{"ticks", no_argument, 0, 'k'},
	{"Ticks", required_argument, 0, 'K'},
	{"trig_delay", required_argument, 0, 'r'},
	{"seconds", required_argument, 0, 's'},
	{"Seconds", required_argument, 0, 'S'},
	{"get_seconds", no_argument, 0, 't'},
	{"mcu_clk_sync", required_argument, 0, 'y'},
	{0, 0, 0, 0}};

int main(int argc, char** argv)
{
	int opt;
	hackrf_device* device = NULL;
	int option_index = 0;
	const char* serial_number = NULL;

	uint32_t divisor;
	uint32_t trig_delay;
	int64_t seconds;
	uint32_t ticks;
	uint32_t value;
	double frequency;

	if (argc > 1)
		opt = getopt_long(
			argc,
			argv,
			"hd:D:f:kK:r:s:S:ty:",
			long_options,
			&option_index);
	else {
		usage();
		return 0;
	}

	int result = hackrf_init();
	if (result) {
		printf("hackrf_init() failed: %s (%d)\n",
		       hackrf_error_name(result),
		       result);
		return EXIT_FAILURE;
	}

	result = hackrf_open_by_serial(serial_number, &device);
	if (result) {
		printf("hackrf_open() failed: %s (%d)\n",
		       hackrf_error_name(result),
		       result);
		return EXIT_FAILURE;
	}

	switch (opt) {
	case 'd':
		result = parse_int(optarg, &value);
		if (result) {
			printf("invalid divisor value. Int required'\n");
			return EXIT_FAILURE;
		}
		divisor = value;
		result = hackrf_time_set_divisor_next_pps(device, divisor);
		if (result) {
			printf("divisor next pps setting failed: %s (%d)\n",
			       hackrf_error_name(result),
			       result);
			return EXIT_FAILURE;
		}
		break;

	case 'D':
		result = parse_int(optarg, &value);
		if (result) {
			printf("invalid divisor value. Int required'\n");
			return EXIT_FAILURE;
		}
		divisor = value;
		result = hackrf_time_set_divisor_one_pps(device, divisor);
		if (result) {
			printf("divisor one pps setting failed: %s (%d)\n",
			       hackrf_error_name(result),
			       result);
			return EXIT_FAILURE;
		}
		break;

	case 'f':
		result = parse_double(optarg, &frequency);
		if (result) {
			printf("invalid frequency value. Double required'\n");
			return EXIT_FAILURE;
		}
		result = hackrf_time_set_clk_freq(device, frequency);
		if (result) {
			printf("clock frequency setting failed: %s (%d)\n",
			       hackrf_error_name(result),
			       result);
			return EXIT_FAILURE;
		}
		break;

	case 'k':
		result = hackrf_time_get_ticks_now(device, &ticks);
		if (result) {
			printf("ticks reading failed: %s (%d)\n",
			       hackrf_error_name(result),
			       result);
			return EXIT_FAILURE;
		}
		printf("tick counter: %d\n", ticks);
		break;

	case 'K':
		result = hackrf_time_set_ticks_now(device, ticks);
		if (result) {
			printf("tick setting failed: %s (%d)\n",
			       hackrf_error_name(result),
			       result);
			return EXIT_FAILURE;
		}
		break;

	case 'r':
		result = parse_int(optarg, &value);
		if (result) {
			printf("invalid delay value. Int required'\n");
			return EXIT_FAILURE;
		}
		trig_delay = value;
		result = hackrf_time_set_trig_delay_next_pps(device, trig_delay);
		if (result) {
			printf("trigger delay setting failed: %s (%d)\n",
			       hackrf_error_name(result),
			       result);
			return EXIT_FAILURE;
		}
		break;

	case 's':
		result = parse_int(optarg, &value);
		if (result) {
			printf("invalid second value. Int required'\n");
			return EXIT_FAILURE;
		}
		seconds = value;
		result = hackrf_time_set_seconds_now(device, seconds);
		if (result) {
			printf("second setting failed: %s (%d)\n",
			       hackrf_error_name(result),
			       result);
			return EXIT_FAILURE;
		}
		break;

	case 'S':
		result = parse_int(optarg, &value);
		if (result) {
			printf("invalid second value. Int required'\n");
			return EXIT_FAILURE;
		}
		seconds = value;
		result = hackrf_time_set_seconds_next_pps(device, seconds);
		if (result) {
			printf("second setting at next pps failed: %s (%d)\n",
			       hackrf_error_name(result),
			       result);
			return EXIT_FAILURE;
		}
		break;

	case 't':
		result = hackrf_time_get_seconds_now(device, &seconds);
		if (result) {
			printf("seconds reading failed: %s (%d)\n",
			       hackrf_error_name(result),
			       result);
			return EXIT_FAILURE;
		}
		printf("second counter: %ld\n", seconds);
		break;

	case 'y':
		result = parse_int(optarg, &value);
		if (result) {
			printf("invalid enable value. Int required'\n");
			return EXIT_FAILURE;
		}
		result = hackrf_time_set_mcu_clk_sync(device, value);
		if (result) {
			printf("seconds setting at next pps failed: %s (%d)\n",
			       hackrf_error_name(result),
			       result);
			return EXIT_FAILURE;
		}
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
		printf("argument error: %s (%d)\n", hackrf_error_name(result), result);
		usage();
		return EXIT_FAILURE;
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
