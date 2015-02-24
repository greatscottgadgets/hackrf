/*
 * Copyright 2012 Jared Boone <jared@sharebrained.com>
 * Copyright 2013 Benjamin Vernoux <titanmkd@gmail.com>
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

static void usage() {
	printf("\nUsage:\n");
	printf("\t-n, --register <n>: set register number for subsequent read/write operations\n");
	printf("\t-r, --read: read register specified by last -n argument, or all registers\n");
	printf("\t-w, --write <v>: write register specified by last -n argument with value <v>\n");
	printf("\nExamples:\n");
	printf("\t<command> -n 12 -r    # reads from register 12\n");
	printf("\t<command> -r          # reads all registers\n");
	printf("\t<command> -n 10 -w 514 # writes register 10 with 514 decimal\n");
}

static struct option long_options[] = {
	{ "register", required_argument, 0, 'n' },
	{ "write", required_argument, 0, 'w' },
	{ "read", no_argument, 0, 'r' },
	{ 0, 0, 0, 0 },
};

int parse_int(char* s, uint16_t* const value) {
	uint_fast8_t base = 10;
	char* s_end;
	long long_value;

	if( strlen(s) > 2 ) {
		if( s[0] == '0' ) {
			if( (s[1] == 'x') || (s[1] == 'X') ) {
				base = 16;
				s += 2;
			} else if( (s[1] == 'b') || (s[1] == 'B') ) {
				base = 2;
				s += 2;
			}
		}
	}

	s_end = s;
	long_value = strtol(s, &s_end, base);
	if( (s != s_end) && (*s_end == 0) ) {
		*value = (uint16_t)long_value;
		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_INVALID_PARAM;
	}
}

int dump_register(hackrf_device* device, const uint16_t register_number) {
	uint16_t register_value;
	int result = hackrf_rffc5071_read(device, (uint8_t)register_number, &register_value);
	
	if( result == HACKRF_SUCCESS ) {
		printf("[%2d] -> 0x%03x\n", register_number, register_value);
	} else {
		printf("hackrf_rffc5071_read() failed: %s (%d)\n", hackrf_error_name(result), result);
	}
	
	return result;
}

int dump_registers(hackrf_device* device) {
	uint16_t register_number;
	int result = HACKRF_SUCCESS;
	
	for(register_number=0; register_number<31; register_number++) {
		result = dump_register(device, register_number);
		if( result != HACKRF_SUCCESS ) {
			break;
		}
	}
	
	return result;
}

int write_register(
	hackrf_device* device,
	const uint16_t register_number,
	const uint16_t register_value
) {
	int result = HACKRF_SUCCESS;
	result = hackrf_rffc5071_write(device, (uint8_t)register_number, register_value);
	
	if( result == HACKRF_SUCCESS ) {
		printf("0x%03x -> [%2d]\n", register_value, register_number);
	} else {
		printf("hackrf_rffc5071_write() failed: %s (%d)\n", hackrf_error_name(result), result);
	}
	
	return result;
}

#define REGISTER_INVALID 32767

int main(int argc, char** argv) {
	int opt;
	uint16_t register_number = REGISTER_INVALID;
	uint16_t register_value;
	hackrf_device* device = NULL;
	int option_index = 0;

	int result = hackrf_init();
	if( result ) {
		printf("hackrf_init() failed: %s (%d)\n", hackrf_error_name(result), result);
		return -1;
	}
	
	result = hackrf_open(&device);
	if( result ) {
		printf("hackrf_open() failed: %s (%d)\n", hackrf_error_name(result), result);
		return -1;
	}

	while( (opt = getopt_long(argc, argv, "n:rw:", long_options, &option_index)) != EOF ) {
		switch( opt ) {
		case 'n':
			result = parse_int(optarg, &register_number);
			break;
		
		case 'w':
			result = parse_int(optarg, &register_value);
			if( result == HACKRF_SUCCESS ) {
				result = write_register(device, register_number, register_value);
			}
			break;
		
		case 'r':
			if( register_number == REGISTER_INVALID ) {
				result = dump_registers(device);
			} else {
				result = dump_register(device, register_number);
			}
			break;
		
		default:
			usage();
		}
		
		if( result != HACKRF_SUCCESS ) {
			printf("argument error: %s (%d)\n", hackrf_error_name(result), result);
			usage();
			break;
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
