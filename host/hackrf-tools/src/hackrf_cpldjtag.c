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
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <stdint.h>

#ifdef _MSC_VER
#ifdef _WIN64
typedef int64_t ssize_t;
#else
typedef int32_t ssize_t;
#endif
#endif
/* input file shouldn't be any longer than this */
#define MAX_XSVF_LENGTH 0x10000
#define PACKET_LEN	4096

uint8_t data[MAX_XSVF_LENGTH];

static struct option long_options[] = {
	{ "xsvf", required_argument, 0, 'x' },
	{ 0, 0, 0, 0 },
};

int parse_int(char* s, uint32_t* const value)
{
	uint_fast8_t base = 10;
	char* s_end;
	long long_value;

	if (strlen(s) > 2) {
		if (s[0] == '0')  {
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
		*value = long_value;
		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_INVALID_PARAM;
	}
}

static void usage()
{
	printf("Usage:\n");
	printf("\t-x <filename>: XSVF file to be written to CPLD.\n");
}

int main(int argc, char** argv)
{
	int opt;
	uint32_t length = 0;
	uint32_t total_length = 0;
	const char* path = NULL;
	hackrf_device* device = NULL;
	int result = HACKRF_SUCCESS;
	int option_index = 0;
	FILE* fd = NULL;
	ssize_t bytes_read;
	uint8_t* pdata = &data[0];

	while ((opt = getopt_long(argc, argv, "x:", long_options,
			&option_index)) != EOF) {
		switch (opt) {
		case 'x':
			path = optarg;
			break;

		default:
			usage();
			return EXIT_FAILURE;
		}

		if (result != HACKRF_SUCCESS) {
			fprintf(stderr, "argument error: %s (%d)\n",
					hackrf_error_name(result), result);
			usage();
			return EXIT_FAILURE;
		}
	}

	if (path == NULL) {
		fprintf(stderr, "Specify a path to a file.\n");
		usage();
		return EXIT_FAILURE;
	}

	fd = fopen(path, "rb");
	if (fd == NULL)
	{
		fprintf(stderr, "Failed to open file: %s\n", path);
		return EXIT_FAILURE;
	}
	/* Get size of the file  */
	fseek(fd, 0, SEEK_END); /* Not really portable but work on major OS Linux/Win32 */
	length = ftell(fd);
	/* Move to start */
	rewind(fd);
	printf("File size %d bytes.\n", length);

	if (length > MAX_XSVF_LENGTH) {
		fprintf(stderr, "XSVF file too large.\n");
		usage();
		return EXIT_FAILURE;
	}

	total_length = length;
	bytes_read = fread(data, 1, total_length, fd);
	if (bytes_read != total_length)
	{
		fprintf(stderr, "Failed to read all bytes (read %d bytes instead of %d bytes).\n",
				(int)bytes_read, total_length);
		fclose(fd);
		fd = NULL;
		return EXIT_FAILURE;
	}

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

	printf("LED1/2/3 blinking means CPLD program success.\nLED3/RED steady means error.\n");
	printf("Wait message 'Write finished' or in case of LED3/RED steady, Power OFF/Disconnect the HackRF.\n");
	result = hackrf_cpld_write(device, pdata, total_length);
	if (result != HACKRF_SUCCESS)
	{
		fprintf(stderr, "hackrf_cpld_write() failed: %s (%d)\n",
				hackrf_error_name(result), result);
		fclose(fd);
		fd = NULL;
		return EXIT_FAILURE;
	}

	printf("Write finished.\n");
	printf("Please Power OFF/Disconnect the HackRF.\n");
	fflush(stdout);

	result = hackrf_close(device);
	if( result != HACKRF_SUCCESS )
	{
		fprintf(stderr, "hackrf_close() failed: %s (%d)\n",
				hackrf_error_name(result), result);
		fclose(fd);
		fd = NULL;
		return EXIT_FAILURE;
	}

	hackrf_exit();

	if (fd != NULL) {
		fclose(fd);
	}

	return EXIT_SUCCESS;
}
