/*
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
#define MAX_FREQ_RANGES 8

static void usage() {
	printf("\nUsage:\n");
	printf("\t-h, --help: this help\n");
	printf("\t-d, --device <n>: specify a particular device by serial number\n");
	printf("\t-o, --address <n>: specify a particular operacake by address [default: 0x00]\n");
	printf("\t-a <n>: set port A connection\n");
	printf("\t-b <n>: set port B connection\n");
	printf("\t-f <min:max:port>: automatically assign <port> for range <min:max> in MHz\n");
	printf("\t-l, --list: list available operacake boards\n");
}

static struct option long_options[] = {
	{ "device", no_argument, 0, 'd' },
	{ "address", no_argument, 0, 'o' },
	{ "list", no_argument, 0, 'v' },
	{ "help", no_argument, 0, 'h' },
	{ 0, 0, 0, 0 },
};

typedef struct {
	uint16_t freq_min;
	uint16_t freq_max;
	uint8_t port;
} hackrf_oc_range;

int parse_uint16(char* const s, uint16_t* const value) {
	char* s_end = s;
	const long long_value = strtol(s, &s_end, 10);
	if( (s != s_end) && (*s_end == 0) ) {
		*value = (uint16_t)long_value;
		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_INVALID_PARAM;
	}
}

int parse_u16_range(char* s, hackrf_oc_range* range) {
	int result = 0;
	uint16_t tmp_port, port = 0;

	char *sep = strchr(s, ':');
	if (!sep)
		return HACKRF_ERROR_INVALID_PARAM;
	*sep = 0;

	char *sep2 = strchr(sep+1, ':');
	if (!sep2)
		return HACKRF_ERROR_INVALID_PARAM;
	*sep2 = 0;

	result = parse_uint16(s, &range->freq_min);
	if (result != HACKRF_SUCCESS)
		return result;
	result = parse_uint16(sep + 1, &range->freq_max);
	if (result != HACKRF_SUCCESS)
		return result;

	sep2++; // Skip past the ':'
	if(sep2[0] == 'A' || sep2[0] == 'B') {
		// The port was specified as a side and number eg. A1 or B3
		if(sep2[1] > 0x30 && sep2[1] < 0x35) {
			tmp_port = sep2[1] - 0x30;
			if(tmp_port >= 5 || tmp_port <= 0)
				return HACKRF_ERROR_INVALID_PARAM;

			// Value was a valid port between 0-4
			if(sep2[0] == 'A') {
				// A1=0, A2=1, A3=2, A4=3
				port = (uint16_t) tmp_port-1;
			} else {
				// If B was specfied just add 4-1 ports
				// B1=4, B2=5, B3=6, B4=7
				port = (uint16_t) tmp_port+3;
			}
			//printf("Setting port %c%c to port %d\n", sep2[0], sep2[1], (uint16_t)port);
		}
	} else {
		result = parse_uint16(sep2, &port);
		if (result != HACKRF_SUCCESS)
			return result;
	}
	range->port = port & 0xFF;

	return HACKRF_SUCCESS;
}

int main(int argc, char** argv) {
	int opt;
	const char* serial_number = NULL;
	int operacake_address = 0;
	int port_a = 0;
	int port_b = 0;
	bool set_ports = false;
	bool list = false;
	uint8_t operacakes[8];
	uint8_t operacake_count = 0;
	int i = 0;
	hackrf_device* device = NULL;
	int option_index = 0;
	hackrf_oc_range ranges[MAX_FREQ_RANGES];
	uint8_t range_idx = 0;

	int result = hackrf_init();
	if( result ) {
		printf("hackrf_init() failed: %s (%d)\n", hackrf_error_name(result), result);
		return -1;
	}

	while( (opt = getopt_long(argc, argv, "d:o:a:b:lf:h?", long_options, &option_index)) != EOF ) {
		switch( opt ) {
		case 'd':
			serial_number = optarg;
			break;

		case 'o':
			operacake_address = atoi(optarg);
			set_ports = true;
			break;

		case 'f':
			result = parse_u16_range(optarg, &ranges[range_idx]);
			if(ranges[range_idx].freq_min >= ranges[range_idx].freq_max) {
				fprintf(stderr,
						"argument error: freq_max must be greater than freq_min.\n");
				usage();
				return EXIT_FAILURE;
			}
			if(FREQ_MAX_MHZ < ranges[range_idx].freq_max) {
				fprintf(stderr,
						"argument error: freq_max may not be higher than %u.\n",
						FREQ_MAX_MHZ);
				usage();
				return EXIT_FAILURE;
			}
			if(MAX_FREQ_RANGES <= range_idx++) {
				fprintf(stderr,
						"argument error: specify a maximum of %u frequency ranges.\n",
						MAX_FREQ_RANGES);
				usage();
				return EXIT_FAILURE;
			}
			break;

		case 'a':
			port_a = atoi(optarg);
			break;

		case 'b':
			port_b = atoi(optarg);
			break;

		case 'l':
			list = true;
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

	if(!(list || set_ports || range_idx)) {
		fprintf(stderr, "Specify either list or address option.\n");
		usage();
		return EXIT_FAILURE;
	}

	result = hackrf_open_by_serial(serial_number, &device);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_open() failed: %s (%d)\n",
				hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	if(list) {
		result = hackrf_get_operacake_boards(device, operacakes);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr, "hackrf_get_operacake_boards() failed: %s (%d)\n",
					hackrf_error_name(result), result);
			return EXIT_FAILURE;
		}
		printf("Operacakes found: ");
		for(i=0; i<8; i++) {
			if(operacakes[i] !=0) {
				printf("\n%d", operacakes[i]);
				operacake_count++;
			}
		}
		if(!operacake_count)
			printf("None");
		printf("\n");
	}

	if(set_ports) {
		result = hackrf_set_operacake_ports(device, operacake_address, port_a, port_b);
		if( result ) {
			printf("hackrf_set_operacake_ports() failed: %s (%d)\n", hackrf_error_name(result), result);
			return -1;
		}
	}

	if(range_idx) {
		uint8_t range_bytes[MAX_FREQ_RANGES * 5];
		uint8_t ptr;
		for(i=0; i<range_idx; i++) {
			ptr = 5*i;
			range_bytes[ptr] = ranges[i].freq_min >> 8;
			range_bytes[ptr+1] = ranges[i].freq_min & 0xFF;
			range_bytes[ptr+2] = ranges[i].freq_max >> 8;
			range_bytes[ptr+3] = ranges[i].freq_max & 0xFF;
			range_bytes[ptr+4] = ranges[i].port;
		}

		result = hackrf_set_operacake_ranges(device, range_bytes, range_idx*5);
		if( result ) {
			printf("hackrf_set_operacake_ranges() failed: %s (%d)\n", hackrf_error_name(result), result);
			return -1;
		}
	}

	result = hackrf_close(device);
	if( result ) {
		printf("hackrf_close() failed: %s (%d)\n", hackrf_error_name(result), result);
		return -1;
	}
	hackrf_exit();
	return 0;
}
