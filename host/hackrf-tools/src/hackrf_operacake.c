/*
 * Copyright 2016-2022 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2016 Dominic Spill <dominicgs@gmail.com>
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
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#ifndef bool
typedef int bool;
	#define true 1
	#define false 0
#endif

#define FREQ_MIN_MHZ (0)    /*    0 MHz */
#define FREQ_MAX_MHZ (7250) /* 7250 MHz */

#define INVALID_ADDRESS 0xFF
#define INVALID_MODE    0xFF
#define INVALID_PORT    0xFF

#define GPIO_TEST_DISABLED 0xFFFF

static void usage()
{
	printf("\nUsage:\n");
	printf("\t-h, --help: this help\n");
	printf("\t-d, --device <n>: specify a particular device by serial number\n");
	printf("\t-o, --address <n>: specify a particular Opera Cake by address [default: 0]\n");
	printf("\t-m, --mode <mode>: specify switching mode [options: manual, frequency, time]\n");
	printf("\t-a <port>: set port connected to port A0\n");
	printf("\t-b <port>: set port connected to port B0\n");
	printf("\t-f <port:min:max>: automatically assign <port> for range <min:max> in MHz. This argument can be repeated to specify a list of ports.\n");
	printf("\t-t <port:dwell>: in time mode, dwell on <port> for <dwell> samples. Specify only <port> to use the default dwell time (with -w). This argument can be repeated to specify a list of ports.\n");
	printf("\t-w <n>: set default dwell time in samples for time mode\n");
	printf("\t-l, --list: list available Opera Cake boards\n");
	printf("\t-g, --gpio_test: test GPIO functionality of an Opera Cake\n");
}

static struct option long_options[] = {
	{"device", required_argument, 0, 'd'},
	{"address", required_argument, 0, 'o'},
	{"mode", required_argument, 0, 'm'},
	{"list", no_argument, 0, 'l'},
	{"gpio_test", no_argument, 0, 'g'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0},
};

int parse_uint16(char* const s, uint16_t* const value)
{
	char* s_end = s;
	const long long_value = strtol(s, &s_end, 10);
	if ((s != s_end) && (*s_end == 0)) {
		*value = (uint16_t) long_value;
		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_INVALID_PARAM;
	}
}

int parse_uint32(char* const s, uint32_t* const value)
{
	char* s_end = s;
	const long long_value = strtol(s, &s_end, 10);
	if ((s != s_end) && (*s_end == 0)) {
		*value = (uint32_t) long_value;
		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_INVALID_PARAM;
	}
}

int parse_port(char* str, uint8_t* port)
{
	uint16_t tmp_port;
	int result;

	if (str[0] == 'A' || str[0] == 'B') {
		// The port was specified as a side and number eg. A1 or B3
		result = parse_uint16(str + 1, &tmp_port);
		if (result != HACKRF_SUCCESS) {
			return result;
		}

		if (tmp_port >= 5 || tmp_port <= 0) {
			fprintf(stderr, "invalid port: %s\n", str);
			return HACKRF_ERROR_INVALID_PARAM;
		}

		// Value was a valid port between 0-4
		if (str[0] == 'A') {
			// A1=0, A2=1, A3=2, A4=3
			tmp_port -= 1;
		} else {
			// If B was specfied just add 4-1 ports
			// B1=4, B2=5, B3=6, B4=7
			tmp_port += 3;
		}
	} else {
		result = parse_uint16(str, &tmp_port);
		if (result != HACKRF_SUCCESS) {
			return result;
		}
	}
	*port = tmp_port & 0xFF;
	// printf("Port: %d\n", *port);
	return HACKRF_SUCCESS;
}

int parse_range(char* s, hackrf_operacake_freq_range* range)
{
	char port[16];
	float min;
	float max;
	int result;

	// Read frequency as a float here to support scientific notation (e.g: 1e6)
	if (sscanf(s, "%15[^:]:%f:%f", port, &min, &max) == 3) {
		result = parse_port(port, &(range->port));
		if (result != HACKRF_SUCCESS) {
			return result;
		}

		range->freq_min = min;
		range->freq_max = max;
		return HACKRF_SUCCESS;
	}
	return HACKRF_ERROR_INVALID_PARAM;
}

int parse_dwell(char* s, hackrf_operacake_dwell_time* dwell_time)
{
	int result;
	char port[16];
	float dwell;

	// Read dwell as a float here to support scientific notation (e.g: 1e6)
	if (sscanf(s, "%15[^:]:%f", port, &dwell) == 2) {
		result = parse_port(port, &dwell_time->port);
		if (result != HACKRF_SUCCESS) {
			return result;
		}

		if (dwell == 0) {
			fprintf(stderr, "dwell time cannot be 0\n");
			return HACKRF_ERROR_INVALID_PARAM;
		}
		dwell_time->dwell = (uint32_t) dwell;
		return HACKRF_SUCCESS;
	} else if (sscanf(s, "%15[^:]", port) == 1) {
		result = parse_port(port, &dwell_time->port);
		if (result != HACKRF_SUCCESS) {
			return result;
		}

		// This will be replaced with the default dwell time later.
		dwell_time->dwell = 0;
		return HACKRF_SUCCESS;
	}
	return HACKRF_ERROR_INVALID_PARAM;
}

int main(int argc, char** argv)
{
	int opt;
	const char* serial_number = NULL;
	uint8_t operacake_address = 0;
	bool set_mode = false;
	uint8_t mode;
	uint8_t port_a = INVALID_PORT;
	uint8_t port_b = INVALID_PORT;
	bool set_ports = false;
	bool list = false;
	bool gpio_test = false;
	uint8_t operacakes[8];
	uint8_t operacake_count = 0;
	int i = 0;
	hackrf_device* device = NULL;
	int option_index = 0;
	hackrf_operacake_freq_range ranges[HACKRF_OPERACAKE_MAX_FREQ_RANGES];
	hackrf_operacake_dwell_time dwell_times[HACKRF_OPERACAKE_MAX_DWELL_TIMES];
	uint8_t range_idx = 0;
	uint8_t dwell_idx = 0;
	uint32_t default_dwell = 0;

	int result = hackrf_init();
	if (result) {
		printf("hackrf_init() failed: %s (%d)\n",
		       hackrf_error_name(result),
		       result);
		return -1;
	}

	while ((opt = getopt_long(
			argc,
			argv,
			"d:o:a:m:b:lf:t:w:hg?",
			long_options,
			&option_index)) != EOF) {
		switch (opt) {
		case 'd':
			serial_number = optarg;
			break;

		case 'o':
			operacake_address = atoi(optarg);
			break;

		case 'm':
			if (strcmp(optarg, "manual") == 0) {
				mode = OPERACAKE_MODE_MANUAL;
				set_mode = true;
			} else if (strcmp(optarg, "frequency") == 0) {
				mode = OPERACAKE_MODE_FREQUENCY;
				set_mode = true;
			} else if (strcmp(optarg, "time") == 0) {
				mode = OPERACAKE_MODE_TIME;
				set_mode = true;
			} else {
				fprintf(stderr,
					"argument error: mode must be one of [manual, frequency, time].\n");
				usage();
				return EXIT_FAILURE;
			}
			break;

		case 'f':
			if (HACKRF_OPERACAKE_MAX_FREQ_RANGES == range_idx) {
				fprintf(stderr,
					"argument error: specify a maximum of %u frequency ranges.\n",
					HACKRF_OPERACAKE_MAX_FREQ_RANGES);
				usage();
				return EXIT_FAILURE;
			}
			result = parse_range(optarg, &ranges[range_idx]);
			if (result != HACKRF_SUCCESS) {
				fprintf(stderr, "failed to parse range\n");
				return EXIT_FAILURE;
			}
			if (ranges[range_idx].freq_min >= ranges[range_idx].freq_max) {
				fprintf(stderr,
					"argument error: freq_max must be greater than freq_min.\n");
				usage();
				return EXIT_FAILURE;
			}
			if (FREQ_MAX_MHZ < ranges[range_idx].freq_max) {
				fprintf(stderr,
					"argument error: freq_max may not be higher than %u.\n",
					FREQ_MAX_MHZ);
				usage();
				return EXIT_FAILURE;
			}
			range_idx++;
			break;

		case 't':
			if (HACKRF_OPERACAKE_MAX_DWELL_TIMES == dwell_idx) {
				fprintf(stderr,
					"argument error: specify a maximum of %u dwell times.\n",
					HACKRF_OPERACAKE_MAX_DWELL_TIMES);
				usage();
				return EXIT_FAILURE;
			}
			result = parse_dwell(optarg, &dwell_times[dwell_idx]);
			if (result != HACKRF_SUCCESS) {
				fprintf(stderr, "failed to parse dwell time\n");
				return EXIT_FAILURE;
			}
			dwell_idx++;
			break;

		case 'w':
			default_dwell = atof(optarg);
			break;

		case 'a':
			result = parse_port(optarg, &port_a);
			if (result != HACKRF_SUCCESS) {
				fprintf(stderr, "failed to parse port\n");
				return EXIT_FAILURE;
			}
			set_ports = true;
			break;

		case 'b':
			result = parse_port(optarg, &port_b);
			if (result != HACKRF_SUCCESS) {
				fprintf(stderr, "failed to parse port\n");
				return EXIT_FAILURE;
			}
			set_ports = true;
			break;

		case 'l':
			list = true;
			break;

		case 'g':
			gpio_test = true;
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
	}

	// Any operations that set a parameter on an Opera Cake board.
	bool set_params = set_mode || set_ports || range_idx || dwell_idx;

	// Error out unless exactly one option is selected.
	if (list + set_params + gpio_test != 1) {
		fprintf(stderr, "Specify either list, mode, or GPIO test option.\n");
		usage();
		return EXIT_FAILURE;
	}

	result = hackrf_open_by_serial(serial_number, &device);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_open() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		return EXIT_FAILURE;
	}

	if (set_mode) {
		result = hackrf_set_operacake_mode(device, operacake_address, mode);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_set_operacake_mode() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			return EXIT_FAILURE;
		}
	}

	if (list) {
		result = hackrf_get_operacake_boards(device, operacakes);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_get_operacake_boards() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			return EXIT_FAILURE;
		}
		printf("Opera Cakes found: ");
		for (i = 0; i < 8; i++) {
			if (operacakes[i] != HACKRF_OPERACAKE_ADDRESS_INVALID) {
				printf("\n\tAddress: %d", operacakes[i]);
				enum operacake_switching_mode mode;
				hackrf_get_operacake_mode(device, i, &mode);
				printf("\tSwitching mode: ");
				if (mode == OPERACAKE_MODE_MANUAL) {
					printf("manual\n");
				} else if (mode == OPERACAKE_MODE_FREQUENCY) {
					printf("frequency\n");
				} else if (mode == OPERACAKE_MODE_TIME) {
					printf("time\n");
				} else {
					printf("unknown\n");
				}
				operacake_count++;
			}
		}
		if (!operacake_count) {
			printf("None");
		}
		printf("\n");
	}

	if (gpio_test) {
		uint16_t test_result;
		uint8_t reg, mask = 0x7;
		result = hackrf_operacake_gpio_test(
			device,
			operacake_address,
			&test_result);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_operacake_gpio_test() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			return EXIT_FAILURE;
		}

		if (test_result == GPIO_TEST_DISABLED) {
			fprintf(stderr, "GPIO mode disabled.\n");
			fprintf(stderr, "Remove additional add-on boards and retry.\n");
		} else if (test_result) {
			fprintf(stderr, "GPIO test failed\n");
			fprintf(stderr, "Pin\tHigh\tShorts\tLow\n");
			reg = test_result & mask;
			fprintf(stderr,
				"u2ctrl1\t%d\t%d\t%d\n",
				(reg >> 2) & 1,
				(reg >> 1) & 1,
				reg & 1);
			test_result >>= 3;
			reg = test_result & mask;
			fprintf(stderr,
				"u2ctrl0\t%d\t%d\t%d\n",
				(reg >> 2) & 1,
				(reg >> 1) & 1,
				reg & 1);
			test_result >>= 3;
			reg = test_result & mask;
			fprintf(stderr,
				"u3ctrl1\t%d\t%d\t%d\n",
				(reg >> 2) & 1,
				(reg >> 1) & 1,
				reg & 1);
			test_result >>= 3;
			reg = test_result & mask;
			fprintf(stderr,
				"u3ctrl0\t%d\t%d\t%d\n",
				(reg >> 2) & 1,
				(reg >> 1) & 1,
				reg & 1);
			test_result >>= 3;
			reg = test_result & mask;
			fprintf(stderr,
				"u1ctrl \t%d\t%d\t%d\n",
				(reg >> 2) & 1,
				(reg >> 1) & 1,
				reg & 1);
		} else {
			fprintf(stderr, "GPIO test passed\n");
		}
	}

	if (set_ports) {
		// Set other port to "don't care" if not set
		if (port_a == INVALID_PORT) {
			if (port_b >= 4) {
				port_a = 0;
			} else {
				port_a = 4;
			}
		}
		if (port_b == INVALID_PORT) {
			if (port_a >= 4) {
				port_b = 0;
			} else {
				port_b = 4;
			}
		}
		if (((port_a <= 3) && (port_b <= 3)) ||
		    ((port_a >= 4) && (port_b >= 4))) {
			fprintf(stderr,
				"Port A and B cannot be connected to the same side\n");
			return EXIT_FAILURE;
		}
		result = hackrf_set_operacake_ports(
			device,
			operacake_address,
			port_a,
			port_b);
		if (result) {
			printf("hackrf_set_operacake_ports() failed: %s (%d)\n",
			       hackrf_error_name(result),
			       result);
			return EXIT_FAILURE;
		}
	}

	if (range_idx) {
		result = hackrf_set_operacake_freq_ranges(device, ranges, range_idx);
		if (result) {
			printf("hackrf_set_operacake_freq_ranges() failed: %s (%d)\n",
			       hackrf_error_name(result),
			       result);
			return -1;
		}
	}

	if (dwell_idx) {
		for (i = 0; i < dwell_idx; i++) {
			if (dwell_times[i].dwell == 0) {
				if (default_dwell == 0) {
					fprintf(stderr,
						"port '%u' set to use default dwell time, but default dwell time is not set. Use -w argument to set default dwell time.\n",
						dwell_times[i].port);
					return EXIT_FAILURE;
				}
				dwell_times[i].dwell = default_dwell;
			}
		}
		result = hackrf_set_operacake_dwell_times(device, dwell_times, dwell_idx);
		if (result) {
			printf("hackrf_set_operacake_dwell_times() failed: %s (%d)\n",
			       hackrf_error_name(result),
			       result);
			return -1;
		}
	}

	result = hackrf_close(device);
	if (result) {
		printf("hackrf_close() failed: %s (%d)\n",
		       hackrf_error_name(result),
		       result);
		return -1;
	}
	hackrf_exit();
	return 0;
}
