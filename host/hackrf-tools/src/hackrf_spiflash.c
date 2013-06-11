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

#ifndef bool
typedef int bool;
#define true 1
#define false 0
#endif

#ifdef _MSC_VER
#ifdef _WIN64
typedef int64_t ssize_t;
#else
typedef int32_t ssize_t;
#endif
#endif

/* 8 Mbit flash */
#define MAX_LENGTH 0x100000

static struct option long_options[] = {
	{ "address", required_argument, 0, 'a' },
	{ "length", required_argument, 0, 'l' },
	{ "read", required_argument, 0, 'r' },
	{ "write", required_argument, 0, 'w' },
	{ 0, 0, 0, 0 },
};

int parse_u32(char* s, uint32_t* const value)
{
	char* s_end;
	uint_fast8_t base = 10;
	uint32_t u32_value;

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
	u32_value = strtoul(s, &s_end, base);
	if ((s != s_end) && (*s_end == 0)) {
		*value = u32_value;
		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_INVALID_PARAM;
	}
}

static void usage()
{
	printf("Usage:\n");
	printf("\t-a, --address <n>: starting address (default: 0)\n");
	printf("\t-l, --length <n>: number of bytes to read (default: 0)\n");
	printf("\t-r <filename>: Read data into file.\n");
	printf("\t-w <filename>: Write data from file.\n");
}

int main(int argc, char** argv)
{
	int opt;
	uint32_t address = 0;
	uint32_t length = 0;
	uint32_t tmp_length;
	uint16_t xfer_len = 0;
	const char* path = NULL;
	hackrf_device* device = NULL;
	int result = HACKRF_SUCCESS;
	int option_index = 0;
	static uint8_t data[MAX_LENGTH];
	uint8_t* pdata = &data[0];
	FILE* fd = NULL;
	bool read = false;
	bool write = false;

	while ((opt = getopt_long(argc, argv, "a:l:r:w:", long_options,
			&option_index)) != EOF) {
		switch (opt) {
		case 'a':
			result = parse_u32(optarg, &address);
			break;

		case 'l':
			result = parse_u32(optarg, &length);
			break;

		case 'r':
			read = true;
			path = optarg;
			break;

		case 'w':
			write = true;
			path = optarg;
			break;

		default:
			fprintf(stderr, "opt error: %d\n", opt);
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

	if (write == read) {
		if (write == true) {
			fprintf(stderr, "Read and write options are mutually exclusive.\n");
		} else {
			fprintf(stderr, "Specify either read or write option.\n");
		}
		usage();
		return EXIT_FAILURE;
	}
	
	if (path == NULL) {
		fprintf(stderr, "Specify a path to a file.\n");
		usage();
		return EXIT_FAILURE;
	}	
	
	if( write )
	{
		fd = fopen(path, "rb");
		if(fd == NULL)
		{
			printf("Error to open file %s\n", path);
			return EXIT_FAILURE;
		}
		/* Get size of the file  */
		fseek(fd, 0, SEEK_END); /* Not really portable but work on major OS Linux/Win32 */
		length = ftell(fd);
		/* Move to start */
		rewind(fd);
		printf("File size %d bytes.\n", length);
	}

	if (length == 0) {
		fprintf(stderr, "Requested transfer of zero bytes.\n");
		if(fd != NULL)
			fclose(fd);
		usage();
		return EXIT_FAILURE;
	}

	if ((length > MAX_LENGTH) || (address > MAX_LENGTH)
			|| ((address + length) > MAX_LENGTH)) {
		fprintf(stderr, "Request exceeds size of flash memory.\n");
		if(fd != NULL)
			fclose(fd);
		usage();
		return EXIT_FAILURE;
	}

	if (read) {
		fd = fopen(path, "wb");
		if(fd == NULL)
		{
			printf("Error to open file %s\n", path);
			return EXIT_FAILURE;
		}
	}

	if (fd == NULL) {
		fprintf(stderr, "Failed to open file: %s\n", path);
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

	if (read) 
	{
		ssize_t bytes_written;
		tmp_length = length;
		while (tmp_length) 
		{
			xfer_len = (tmp_length > 256) ? 256 : tmp_length;
			printf("Reading %d bytes from 0x%06x.\n", xfer_len, address);
			result = hackrf_spiflash_read(device, address, xfer_len, pdata);
			if (result != HACKRF_SUCCESS) {
				fprintf(stderr, "hackrf_spiflash_read() failed: %s (%d)\n",
						hackrf_error_name(result), result);
				fclose(fd);
				fd = NULL;
				return EXIT_FAILURE;
			}			
			address += xfer_len;
			pdata += xfer_len;
			tmp_length -= xfer_len;
		}
		bytes_written = fwrite(data, 1, length, fd);
		if (bytes_written != length) {
			fprintf(stderr, "Failed write to file (wrote %d bytes).\n",
					(int)bytes_written);
			fclose(fd);
			fd = NULL;
			return EXIT_FAILURE;
		}
	} else {
		ssize_t bytes_read = fread(data, 1, length, fd);
		if (bytes_read != length) {
			fprintf(stderr, "Failed read file (read %d bytes).\n",
					(int)bytes_read);
			fclose(fd);
			fd = NULL;
			return EXIT_FAILURE;
		}
		printf("Erasing SPI flash.\n");
		result = hackrf_spiflash_erase(device);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr, "hackrf_spiflash_erase() failed: %s (%d)\n",
					hackrf_error_name(result), result);
			fclose(fd);
			fd = NULL;
			return EXIT_FAILURE;
		}
		while (length) {
			xfer_len = (length > 256) ? 256 : length;
			printf("Writing %d bytes at 0x%06x.\n", xfer_len, address);
			result = hackrf_spiflash_write(device, address, xfer_len, pdata);
			if (result != HACKRF_SUCCESS) {
				fprintf(stderr, "hackrf_spiflash_write() failed: %s (%d)\n",
						hackrf_error_name(result), result);
				fclose(fd);
				fd = NULL;
				return EXIT_FAILURE;
			}
			address += xfer_len;
			pdata += xfer_len;
			length -= xfer_len;
		}
	}

	result = hackrf_close(device);
	if (result != HACKRF_SUCCESS) {
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
