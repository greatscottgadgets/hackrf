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
	{ "compatibility", no_argument, 0, 'c' },
	{ "device", required_argument, 0, 'd' },
	{ "reset", no_argument, 0, 'R' },
	{ "status", no_argument, 0, 's' },
	{ "clear", no_argument, 0, 'c' },
	{ "verbose", no_argument, 0, 'v' },
	{ "help", no_argument, 0, 'h' },
	{ 0, 0, 0, 0 },
};

/* Check for USB product string descriptor text in firmware file
 * It should match the appropriate one for the BOARD_ID
 * If you're already running firmware that reports the wrong ID
 * I can't help you, but you can use the -i optionto ignore (or DFU)
 */
int compatibility_check(uint8_t* data, int length, hackrf_device* device)
{
	int str_len, i,j;
	bool match = false;
	uint8_t board_id;
	char* dev_str;
	hackrf_board_id_read(device, &board_id);
	switch(board_id)
	{
	case BOARD_ID_JAWBREAKER:
		dev_str = "HackRF Jawbreaker";
		str_len = 17;
		break;
	case BOARD_ID_HACKRF_ONE:
		dev_str =  "HackRF One";
		str_len = 10;
		break;
	case BOARD_ID_RAD1O:
		dev_str =  "rad1o";
		str_len = 5;
		break;
	default:
		printf("Unknown Board ID");
		return 1;
	}
	// Search for dev_str in uint8_t array of bytes that we're flashing
	for(i=0; i<length-str_len; i++){
		if(data[i] == dev_str[0]) {
			match = true;
			for(j=1; j<str_len; j++) {
				if((data[i+j*2] != dev_str[j]) ||
				   (data[1+i+j*2] != 0x00)) {
					match = false;
					break;
				}
			}
			if(match)
				return 0;
		}
	}
	return 1;
}

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
	printf("\t-h, --help: this help\n");
	printf("\t-a, --address <n>: starting address (default: 0)\n");
	printf("\t-l, --length <n>: number of bytes to read (default: %d)\n", MAX_LENGTH);
	printf("\t-r, --read <filename>: Read data into file.\n");
	printf("\t-w, --write <filename>: Write data from file.\n");
	printf("\t-i, --no-check: Skip check for firmware compatibility with target device.\n");
	printf("\t-d, --device <serialnumber>: Serial number of device, if multiple devices\n");
	printf("\t-s, --status: Read SPI flash status registers before other operations.\n");
	printf("\t-c, --clear: Clear SPI flash status registers before other operations.\n");
	printf("\t-R, --reset: Reset HackRF after other operations.\n");
	printf("\t-v, --verbose: Verbose output.\n");
}

int main(int argc, char** argv)
{
	int opt;
	uint8_t status[2];
	uint32_t address = 0;
	uint32_t length = MAX_LENGTH;
	uint32_t tmp_length;
	uint16_t xfer_len = 0;
	const char* path = NULL;
	const char* serial_number = NULL;
	hackrf_device* device = NULL;
	int result = HACKRF_SUCCESS;
	int option_index = 0;
	static uint8_t data[MAX_LENGTH];
	uint8_t* pdata = &data[0];
	FILE* fd = NULL;
	bool read = false;
	bool write = false;
	bool ignore_compat_check = false;
	bool verbose = false;
	bool reset = false;
	bool read_status = false;
	bool clear_status = false;
	uint16_t usb_api;

	while ((opt = getopt_long(argc, argv, "a:l:r:w:id:scvRh?", long_options,
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

		case 'i':
			ignore_compat_check = true;
			break;
		
		case 'd':
			serial_number = optarg;
			break;

		case 's':
			read_status = true;
			break;

		case 'c':
			clear_status = true;
			break;

		case 'v':
			verbose = true;
			break;

		case 'R':
			reset = true;
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

		if (result != HACKRF_SUCCESS) {
			fprintf(stderr, "argument error: %s (%d)\n",
					hackrf_error_name(result), result);
			usage();
			return EXIT_FAILURE;
		}
	}

	if(write && read) {
		fprintf(stderr, "Read and write options are mutually exclusive.\n");
		usage();
		return EXIT_FAILURE;
	}

	if(!(write || read || reset || read_status || clear_status)) {
		fprintf(stderr, "Specify either read, write, or reset option.\n");
		usage();
		return EXIT_FAILURE;
	}
	
	if( write )
	{
		fd = fopen(path, "rb");
		if(fd == NULL)
		{
			printf("Error opening file %s\n", path);
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

	result = hackrf_init();
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_init() failed: %s (%d)\n",
				hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	result = hackrf_open_by_serial(serial_number, &device);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_open() failed: %s (%d)\n",
				hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	if(read_status) {
		result = hackrf_spiflash_status(device, status);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr, "hackrf_spiflash_status() failed: %s (%d)\n",
					hackrf_error_name(result), result);
			return EXIT_FAILURE;
		}
		if(!verbose) {
			printf("Status: 0x%02x %02x\n", status[0], status[1]);
		} else {
			printf("SRP0\t%x\nSEC\t%x\nTB\t%x\nBP\t%x\nWEL\t%x\nBusy\t%x\n", 
			       (status[0] & 0x80) >> 7,
			       (status[0] & 0x40) >> 6,
			       (status[0] & 0x20) >> 5,
			       (status[0] & 0x1C) >> 2,
			       (status[0] & 0x02) >> 1,
			       status[0] & 0x01);
			printf("SUS\t%x\nCMP\t%x\nLB\t%x\nRes\t%x\nQE\t%x\nSRP1\t%x\n", 
			       (status[1] & 0x80) >> 7,
			       (status[1] & 0x40) >> 6,
			       (status[1] & 0x38) >> 3,
			       (status[1] & 0x04) >> 2,
			       (status[1] & 0x02) >> 1,
			       status[1] & 0x01);
		}
	}

	if(clear_status) {
		result = hackrf_spiflash_clear_status(device);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr, "hackrf_spiflash_clear_status() failed: %s (%d)\n",
					hackrf_error_name(result), result);
			return EXIT_FAILURE;
		}
	}

	if(read) {
		ssize_t bytes_written;
		tmp_length = length;
		while (tmp_length) 
		{
			xfer_len = (tmp_length > 256) ? 256 : tmp_length;
			if( verbose ) printf("Reading %d bytes from 0x%06x.\n", xfer_len, address);
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
	}

	if(write) {
		ssize_t bytes_read = fread(data, 1, length, fd);
		if (bytes_read != length) {
			fprintf(stderr, "Failed read file (read %d bytes).\n",
					(int)bytes_read);
			fclose(fd);
			fd = NULL;
			return EXIT_FAILURE;
		}
		if(!ignore_compat_check) {
			printf("Checking target device compatibility\n");
			result = compatibility_check(data, length, device);
			if(result) {
				printf("Compatibility test failed.\n");
				fclose(fd);
				fd = NULL;
				return EXIT_FAILURE;
			}
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
		if( !verbose ) printf("Writing %d bytes at 0x%06x.\n", length, address);
		while (length) {
			xfer_len = (length > 256) ? 256 : length;
			if( verbose ) printf("Writing %d bytes at 0x%06x.\n", xfer_len, address);
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

	if (fd != NULL) {
		fclose(fd);
		fd = NULL;
	}

	if(reset) {
		result = hackrf_reset(device);
		if (result != HACKRF_SUCCESS) {
			if (result == HACKRF_ERROR_USB_API_VERSION) {
				hackrf_usb_api_version_read(device, &usb_api);
				fprintf(stderr, "Reset is not supported by firmware API %x.%02x\n",
						(usb_api>>8)&0xFF, usb_api&0xFF);
			} else {
				fprintf(stderr, "hackrf_reset() failed: %s (%d)\n",
						hackrf_error_name(result), result);
			}
			return EXIT_FAILURE;
		}
	}

	result = hackrf_close(device);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_close() failed: %s (%d)\n",
				hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	hackrf_exit();
	return EXIT_SUCCESS;
}
