/*
 * Copyright 2012 Jared Boone <jared@sharebrained.com>
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

static void usage() {
	printf("\nUsage:\n");
	printf("\t-c, --config: print textual configuration information\n");
	printf("\t-n, --register <n>: set register number for subsequent read/write operations\n");
	printf("\t-r, --read: read register specified by last -n argument, or all registers\n");
	printf("\t-w, --write <v>: write register specified by last -n argument with value <v>\n");
	printf("\nExamples:\n");
	printf("\t<command> -n 12 -r    # reads from register 12\n");
	printf("\t<command> -r          # reads all registers\n");
	printf("\t<command> -n 10 -w 22 # writes register 10 with 22 decimal\n");
}

static struct option long_options[] = {
	{ "config", no_argument, 0, 'c' },
	{ "register", required_argument, 0, 'n' },
	{ "write", required_argument, 0, 'w' },
	{ "read", no_argument, 0, 'r' },
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

int dump_register(hackrf_device* device, const uint16_t register_number) {
	uint16_t register_value;
	int result = hackrf_si5351c_read(device, register_number, &register_value);
	
	if( result == HACKRF_SUCCESS ) {
		printf("[%3d] -> 0x%02x\n", register_number, register_value);
	} else {
		printf("hackrf_max2837_read() failed: %s (%d)\n", hackrf_error_name(result), result);
	}
	
	return result;
}

int dump_registers(hackrf_device* device) {
	uint16_t register_number;
	int result = HACKRF_SUCCESS;
	
	for(register_number=0; register_number<256; register_number++) {
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
	result = hackrf_si5351c_write(device, register_number, register_value);
	
	if( result == HACKRF_SUCCESS ) {
		printf("0x%2x -> [%3d]\n", register_value, register_number);
	} else {
		printf("hackrf_max2837_write() failed: %s (%d)\n", hackrf_error_name(result), result);
	}
	
	return result;
}

#define REGISTER_INVALID 32767

int dump_multisynth_config(hackrf_device* device, const uint_fast8_t ms_number) {
	uint_fast8_t i;
	uint_fast8_t reg_base;
	uint16_t parameters[8];
	uint32_t p1,p2,p3,r_div;
	uint_fast8_t div_lut[] = {1,2,4,8,16,32,64,128};

	printf("MS%d:", ms_number);
	if(ms_number <6){
		reg_base = 42 + (ms_number * 8);
		for(i=0; i<8; i++) {
			uint_fast8_t reg_number = reg_base + i;
			int result = hackrf_si5351c_read(device, reg_number, &parameters[i]);
			if( result != HACKRF_SUCCESS ) {
				return result;
			}
		}

		p1 =
			  ((parameters[2] & 0x03) << 16)
			| (parameters[3] << 8)
			| parameters[4]
			;
		p2 =
			  ((parameters[5] & 0x0F) << 16)
			| (parameters[6] << 8)
			| parameters[7]
			;
		p3 =
			  ((parameters[5] & 0xF0) << 12)
			| (parameters[0] << 8)
			|  parameters[1]
			;
		r_div =
			(parameters[2] >> 4) & 0x7
			;

		printf("\tp1 = %u\n", p1);
		printf("\tp2 = %u\n", p2);
		printf("\tp3 = %u\n", p3);
		if(p3)
			printf("\tOutput (800Mhz PLL): %#.10f Mhz\n", ((double)800 / (double)(((double)p1*p3 + p2 + 512*p3)/(double)(128*p3))) / div_lut[r_div] );
	} else {
		// MS6 and 7 are integer only
		unsigned int parms;
		reg_base = 90;

		for(i=0; i<3; i++) {
			uint_fast8_t reg_number = reg_base + i;
			int result = hackrf_si5351c_read(device, reg_number, &parameters[i]);
			if( result != HACKRF_SUCCESS ) {
				return result;
			}
		}

		r_div = (ms_number == 6) ? parameters[2] & 0x7 : (parameters[2] & 0x70) >> 4 ;
		parms = (ms_number == 6) ? parameters[0] : parameters[1];
		printf("\tp1_int = %u\n", parms);
		if(parms)
			printf("\tOutput (800Mhz PLL): %#.10f Mhz\n", (800.0f / parms) / div_lut[r_div] );
	}
	printf("\toutput divider = %u\n", div_lut[r_div]);
	
	return HACKRF_SUCCESS;
}

int dump_configuration(hackrf_device* device) {
	uint_fast8_t ms_number;
	int result;
			
	for(ms_number=0; ms_number<8; ms_number++) {
		result = dump_multisynth_config(device, ms_number);
		if( result != HACKRF_SUCCESS ) {
			return result;
		}
	}
	
	return HACKRF_SUCCESS;
}

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

	while( (opt = getopt_long(argc, argv, "cn:rw:", long_options, &option_index)) != EOF ) {
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
		
		case 'c':
			dump_configuration(device);
			break;

		default:
			usage();
		}
		
		if( result != HACKRF_SUCCESS ) {
			printf("argument error: %s (%d)\n", hackrf_error_name(result), result);
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
