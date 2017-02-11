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

#ifndef bool
typedef int bool;
#define true 1
#define false 0
#endif

static void usage() {
	printf("\nUsage:\n");
	printf("\t-h, --help: this help\n");
	printf("\t-d, --device <n>: specify a particular device by serial number\n");
	printf("\t-o, --address <n>: specify a particular operacake by address [default: 0x00]\n");
	printf("\t-a <n>: set port A connection\n");
	printf("\t-b <n>: set port B connection\n");
	printf("\t-l, --list: list available operacake boards\n");
}

static struct option long_options[] = {
	{ "device", no_argument, 0, 'd' },
	{ "address", no_argument, 0, 'o' },
	{ "list", no_argument, 0, 'v' },
	{ "help", no_argument, 0, 'h' },
	{ 0, 0, 0, 0 },
};

int parse_int(char* const s, uint16_t* const value) {
	char* s_end = s;
	const long long_value = strtol(s, &s_end, 10);
	if( (s != s_end) && (*s_end == 0) ) {
		*value = (uint16_t)long_value;
		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_INVALID_PARAM;
	}
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

	int result = hackrf_init();
	if( result ) {
		printf("hackrf_init() failed: %s (%d)\n", hackrf_error_name(result), result);
		return -1;
	}

	while( (opt = getopt_long(argc, argv, "d:o:a:b:lh?", long_options, &option_index)) != EOF ) {
		switch( opt ) {
		case 'd':
			serial_number = optarg;
			break;

		case 'o':
			operacake_address = atoi(optarg);
			set_ports = true;
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

	if(!(list || set_ports)) {
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

	result = hackrf_close(device);
	if( result ) {
		printf("hackrf_close() failed: %s (%d)\n", hackrf_error_name(result), result);
		return -1;
	}
	hackrf_exit();
	return 0;
}
