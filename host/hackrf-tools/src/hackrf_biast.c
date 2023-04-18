/*
 * Copyright 2012-2022 Great Scott Gadgets <info@greatscottgadgets.com>
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
#include <stdlib.h>

#if defined(__GNUC__)
	#include <unistd.h>
	#include <sys/time.h>
#endif

void print_board_rev(uint8_t board_rev)
{
	switch (board_rev) {
	case BOARD_REV_UNDETECTED:
		printf("Error: Hardware revision not yet detected by firmware.\n");
		return;
	case BOARD_REV_UNRECOGNIZED:
		printf("Warning: Hardware revision not recognized by firmware.\n");
		return;
	}
	printf("Hardware Revision: %s\n", hackrf_board_rev_name(board_rev));
	if (board_rev > BOARD_REV_HACKRF1_OLD) {
		if (board_rev & HACKRF_BOARD_REV_GSG) {
			printf("Hardware appears to have been manufactured by Great Scott Gadgets.\n");
		} else {
			printf("Hardware does not appear to have been manufactured by Great Scott Gadgets.\n");
		}
	}
}

void print_supported_platform(uint32_t platform, uint8_t board_id)
{
	printf("Hardware supported by installed firmware:\n");
	if (platform & HACKRF_PLATFORM_JAWBREAKER) {
		printf("    Jawbreaker\n");
	}
	if (platform & HACKRF_PLATFORM_RAD1O) {
		printf("    rad1o\n");
	}
	if ((platform & HACKRF_PLATFORM_HACKRF1_OG) ||
	    (platform & HACKRF_PLATFORM_HACKRF1_R9)) {
		printf("    HackRF One\n");
	}
	switch (board_id) {
	case BOARD_ID_HACKRF1_OG:
		if (!(platform & HACKRF_PLATFORM_HACKRF1_OG)) {
			printf("Error: Firmware does not support HackRF One revisions older than r9.\n");
		}
		break;
	case BOARD_ID_HACKRF1_R9:
		if (!(platform & HACKRF_PLATFORM_HACKRF1_R9)) {
			printf("Error: Firmware does not support HackRF One r9.\n");
		}
		break;
	case BOARD_ID_JAWBREAKER:
		if (platform & HACKRF_PLATFORM_JAWBREAKER) {
			break;
		}
	case BOARD_ID_RAD1O:
		if (platform & HACKRF_PLATFORM_RAD1O) {
			break;
		}
		printf("Error: Firmware does not support hardware platform.\n");
	}
}

void usage() {
		fprintf(stderr,"Usage: hackrf_biast [0|1]\n");
}

int main(int argc, char** argv)
{
	int opt;
	int result = HACKRF_SUCCESS;
	uint8_t board_id = BOARD_ID_UNDETECTED;
	uint8_t board_rev = BOARD_REV_UNDETECTED;
	uint32_t supported_platform = 0;
	char version[255 + 1];
	uint16_t usb_version;
	read_partid_serialno_t read_partid_serialno;
	uint8_t operacakes[8];
	hackrf_device_list_t* list;
	hackrf_device* device;
	int i, j;

	int biast_enable =0;
	const char* serial_number = NULL;

	while ((opt = getopt(argc, argv, "d:b:h?")) !=
	       EOF) {
		result = HACKRF_SUCCESS;

		switch (opt) {
		case 'b':
			biast_enable = atoi(optarg) ? 1 : 0;
			break;

		case 'd':
			serial_number = optarg;
			break;

		case 'h':
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
		usage();
		return EXIT_FAILURE;
	}

	fprintf(stderr, "call hackrf_set_antenna_enable(%u)\n", biast_enable);
	result = hackrf_set_antenna_enable(device, (uint8_t) biast_enable);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_set_antenna_enable() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		return EXIT_FAILURE;
	}
	

	result = hackrf_close(device);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_close() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
	}
	
	hackrf_exit();

	return EXIT_SUCCESS;
}
