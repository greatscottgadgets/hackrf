/*
 * Copyright 2012 Jared Boone <jared@sharebrained.com>
 * Copyright 2013 Benjamin Vernoux <titanmkd@gmail.com>
 * Copyright 2013 Michael Ossmann <mike@ossmann.com>
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

int main(int argc, char** argv)
{
	hackrf_device* device = NULL;
	int result = HACKRF_SUCCESS;
	uint8_t board_id = BOARD_ID_INVALID;
	char version[255 + 1];
	read_partid_serialno_t read_partid_serialno;

	result = hackrf_init();
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_init() failed: %s (%d)\n",
				hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	result = hackrf_open(&device);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_open() failed: %s (%d)\n",
				hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	printf("Found HackRF board.\n");

	result = hackrf_board_id_read(device, &board_id);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_board_id_read() failed: %s (%d)\n",
				hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}
	printf("Board ID Number: %d (%s)\n", board_id,
			hackrf_board_id_name(board_id));

	result = hackrf_version_string_read(device, &version[0], 255);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_version_string_read() failed: %s (%d)\n",
				hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}
	printf("Firmware Version: %s\n", version);

	result = hackrf_board_partid_serialno_read(device, &read_partid_serialno);	
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_board_partid_serialno_read() failed: %s (%d)\n",
				hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}
	printf("Part ID Number: 0x%08x 0x%08x\n", 
				read_partid_serialno.part_id[0],
				read_partid_serialno.part_id[1]);
	printf("Serial Number: 0x%08x 0x%08x 0x%08x 0x%08x\n", 
				read_partid_serialno.serial_no[0],
				read_partid_serialno.serial_no[1],
				read_partid_serialno.serial_no[2],
				read_partid_serialno.serial_no[3]);
	
	result = hackrf_close(device);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_close() failed: %s (%d)\n",
				hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	hackrf_exit();

	return EXIT_SUCCESS;
}
