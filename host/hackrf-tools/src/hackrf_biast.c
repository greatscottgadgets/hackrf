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

#define USAGE 1
#define INVALID_BIAST_MODE 2

void usage() {
		fprintf(stderr,"\nhackrf_biast - enable/disable antenna power on the HackRF for compatibility\n");
		fprintf(stderr,"               with software that does not support this function (i.e. SDR#)\n\n");
		fprintf(stderr,"Usage: \n");
		fprintf(stderr,"  -h         Display this help\n");
		fprintf(stderr,"  -R         Reset all bias tee settings to device default.  When combined\n");
		fprintf(stderr,"             with -r/-t/-o, those settings will take precedence.\n\n");

		fprintf(stderr,"  [-b mode]  1=Enable bias tee immediately, 0=disable immediately\n");
		fprintf(stderr,"  [-r mode]  Set default bias tee power when device enters RX mode\n");
		fprintf(stderr,"  [-t mode]  Set default bias tee power when device enters TX mode\n");
		fprintf(stderr,"  [-o mode]  Set default bias tee power when device enters OFF mode\n");

		fprintf(stderr,"  [-d serial_number]  Specify serial number of HackRF device to configure\n\n\n");		
		fprintf(stderr,"The -r/-t/-o options support the following mode settings:\n\n");
		fprintf(stderr,"  0=use device default (RX: don't change, TX/OFF: disable bias tee)\n");
		fprintf(stderr,"  1=enable bias tee when entering mode\n");
		fprintf(stderr,"  2=disable bias tee when entering mode\n\n");
		exit(USAGE);
}

void update_user_mode(const char *direction_str,const char* strarg,uint16_t* user_biast_modeopts, uint8_t shift) {
	int mode = atoi(strarg);
	if (mode < 0 || mode > 2) {
		fprintf(stderr,"Invalid mode '%s' for %s\n",strarg,direction_str);
		exit(INVALID_BIAST_MODE);
	}
	if (mode) { mode += 1; }	// Skip over the RESERVED mode
	mode |= 0x4;
	*user_biast_modeopts |= (mode << shift);
}

int main(int argc, char** argv)
{
	int opt;
	int result = HACKRF_SUCCESS;
	hackrf_device* device;

	int biast_enable =-1;
	int do_user_opts_update = 0;
	uint16_t user_biast_modeopts = 0;

	const char* serial_number = NULL;

	while ((opt = getopt(argc, argv, "d:b:h?Rr:t:o:")) !=
	       EOF) {
		result = HACKRF_SUCCESS;

		switch (opt) {
		case 'b':
			biast_enable = atoi(optarg) ? 1 : 0;
			break;

		case 'd':
			serial_number = optarg;
			break;

		case 'r':
			do_user_opts_update=1;
			update_user_mode("RX",optarg,&user_biast_modeopts,3);
			break;

		case 't':
			do_user_opts_update=1;
			update_user_mode("TX",optarg,&user_biast_modeopts,6);
			break;

		case 'o':
			do_user_opts_update=1;
			update_user_mode("OFF",optarg,&user_biast_modeopts,0);
			break;

		case 'R':
			do_user_opts_update=1;
			user_biast_modeopts |= 0b100100100;	// Set all behaviors at once 
			break;

		case 'h':
		case '?':
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

	if (biast_enable >= 0) {
		result = hackrf_set_antenna_enable(device, (uint8_t) biast_enable);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_set_antenna_enable() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			return EXIT_FAILURE;
		}
	}
	
	if (do_user_opts_update) {
		printf("Send %X",user_biast_modeopts);
		result = hackrf_set_user_bias_t_opts(device, user_biast_modeopts);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_set_user_bias_t_opts() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			return EXIT_FAILURE;
		}
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
