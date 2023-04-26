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
#include <getopt.h>

#if defined(__GNUC__)
	#include <unistd.h>
	#include <sys/time.h>
#endif

void usage() {
		fprintf(stderr,"hackrf_biast - enable/disable antenna power on the HackRF for compatibility\n");
		fprintf(stderr,"               with software that does not support this function (i.e. SDR#)\n\n");
		fprintf(stderr,"Usage: hackrf_biast [-b 0|1] [-d serial]\n");
		fprintf(stderr,"\t-h\tDisplay this help.\n");
		fprintf(stderr,"\t-b\t1=Enable antenna power, 0=disable (default=0)\n");
		fprintf(stderr,"\t-d\tSpecify serial number of HackRF device\n\n");		
		fprintf(stderr,"Note that antenna power will be automatically disabled by the firmware\n");
		fprintf(stderr,"when RX is stopped.\n");
}

int main(int argc, char** argv)
{
	int opt;
	int result = HACKRF_SUCCESS;
	hackrf_device* device;

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

/*
	result = hackrf_set_antenna_enable(device, (uint8_t) biast_enable);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_set_antenna_enable() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		return EXIT_FAILURE;
	}
*/	

	result = hackrf_set_user_bias_t_opts(device, (uint16_t) 0x1FE);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_set_user_bias_t_opts() failed: %s (%d)\n",
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
