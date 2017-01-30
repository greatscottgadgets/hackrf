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
	printf("\t-s, --serial <s>: specify a particular device by serial number\n");
	printf("\t-d, --device <n>: specify a particular device by number\n");
	printf("\t-o, --address <n>: specify a particular operacake by address [default: 0x00]\n");
	printf("\t-a <n>: set port A connection\n");
	printf("\t-b <n>: set port B connection\n");
	printf("\t-v: verbose, list available operacake boards\n");
}

static struct option long_options[] = {
	{ "device", no_argument, 0, 'd' },
	{ "serial", no_argument, 0, 's' },
	{ "address", no_argument, 0, 'o' },
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
	int device_index = 0;
	int operacake_address = 0;
	int port_a = 0;
	int port_b = 0;
	int verbose = 0;
	uint8_t operacakes[8];
	int i = 0;
	hackrf_device* device = NULL;
	int option_index = 0;

	int result = hackrf_init();
	if( result ) {
		printf("hackrf_init() failed: %s (%d)\n", hackrf_error_name(result), result);
		return -1;
	}

	while( (opt = getopt_long(argc, argv, "d:s:o:a:b:vh?", long_options, &option_index)) != EOF ) {
		switch( opt ) {
		case 'd':
			device_index = atoi(optarg);
			break;

		case 's':
			serial_number = optarg;
			break;

		case 'o':
			operacake_address = atoi(optarg);
			break;

		case 'a':
			port_a = atoi(optarg);
			break;

		case 'b':
			port_b = atoi(optarg);
			break;

		case 'v':
			verbose = 1;
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
		
		if( result != HACKRF_SUCCESS ) {
			printf("argument error: %s (%d)\n", hackrf_error_name(result), result);
			break;
		}
	}

	if(serial_number != NULL) {
		result = hackrf_open_by_serial(serial_number, &device);
	} else {
		hackrf_device_list_t* device_list = hackrf_device_list();
		if(device_list->devicecount <= 0) {
			result = HACKRF_ERROR_NOT_FOUND;
		} else {
			result = hackrf_device_list_open(device_list, device_index, &device);
		}	
	}

	if( result ) {
		printf("hackrf_open() failed: %s (%d)\n", hackrf_error_name(result), result);
		return -1;
	}

	if(verbose) {
		hackrf_get_operacake_boards(device, operacakes);
		printf("Operacakes found:\n");
		for(i=0; i<8; i++) {
			if(operacakes[i] !=0)
				printf("%d\n", operacakes[i]);
		}
		printf("\n");
	}

	result = hackrf_set_operacake_ports(device, operacake_address, port_a, port_b);
	if( result ) {
		printf("hackrf_set_operacake_ports() failed: %s (%d)\n", hackrf_error_name(result), result);
		return -1;
	}

	result = hackrf_close(device);
	if( result ) {
		printf("hackrf_close() failed: %s (%d)\n", hackrf_error_name(result), result);
		return -1;
	}
	
	hackrf_exit();
	
	return 0;
}
