/*
 * Copyright 2023 Jonathan Suite (GitHub: @ai6aj)
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
#include <string.h>

#if defined(__GNUC__)
	#include <unistd.h>
	#include <sys/time.h>
#endif

#define USAGE              1
#define INVALID_BIAST_MODE 2

void usage()
{
	fprintf(stderr,
		"\nhackrf_biast - enable/disable antenna power on the HackRF for compatibility\n");
	fprintf(stderr,
		"               with software that does not support this function\n\n");
	fprintf(stderr, "Usage: \n");
	fprintf(stderr, "  -h         Display this help\n");
	fprintf(stderr,
		"  -R         Reset all bias tee settings to device default.  When combined\n");
	fprintf(stderr,
		"             with -r/-t/-o, those settings will take precedence.\n\n");

	fprintf(stderr,
		"  [-b mode]  1=Enable bias tee immediately, 0=disable immediately\n");
	fprintf(stderr,
		"  [-r mode]  Set default bias tee power when device enters RX mode\n");
	fprintf(stderr,
		"  [-t mode]  Set default bias tee power when device enters TX mode\n");
	fprintf(stderr,
		"  [-o mode]  Set default bias tee power when device enters OFF mode\n");

	fprintf(stderr,
		"  [-d serial_number]  Specify serial number of HackRF device to configure\n\n\n");
	fprintf(stderr, "The -r/-t/-o options support the following mode settings:\n\n");
	fprintf(stderr, "  leave		do nothing when entering mode\n");
	fprintf(stderr, "  on		enable bias tee when entering mode\n");
	fprintf(stderr, "  off		disable bias tee when entering mode\n\n");
	exit(USAGE);
}

void update_user_mode(
	const char* direction_str,
	const char* strarg,
	hackrf_bool_user_settting* setting)
{
	if (strcmp("off", strarg) == 0) {
		setting->do_update = true;
		setting->change_on_mode_entry = true;
		setting->enabled = false;
	} else if (strcmp("on", strarg) == 0) {
		setting->do_update = true;
		setting->change_on_mode_entry = true;
		setting->enabled = true;
	} else if (strcmp("leave", strarg) == 0) {
		setting->do_update = true;
		setting->change_on_mode_entry = false;
		setting->enabled = false;
	} else {
		fprintf(stderr, "Invalid mode '%s' for %s\n", strarg, direction_str);
		exit(INVALID_BIAST_MODE);
	}
}

int main(int argc, char** argv)
{
	int opt;
	int result = HACKRF_SUCCESS;
	hackrf_device* device;

	int biast_enable = -1;
	int do_user_opts_update = 0;
	hackrf_bias_t_user_settting_req req;

	req.off.do_update = false;
	req.off.change_on_mode_entry = false;
	req.off.enabled = false;
	req.rx.do_update = false;
	req.rx.change_on_mode_entry = false;
	req.rx.enabled = false;
	req.tx.do_update = false;
	req.tx.change_on_mode_entry = false;
	req.tx.enabled = false;

	const char* serial_number = NULL;

	while ((opt = getopt(argc, argv, "d:b:h?Rr:t:o:")) != EOF) {
		result = HACKRF_SUCCESS;

		switch (opt) {
		case 'b':
			biast_enable = atoi(optarg) ? 1 : 0;
			break;

		case 'd':
			serial_number = optarg;
			break;

		case 'r':
			do_user_opts_update = 1;
			update_user_mode("RX", optarg, &req.rx);
			break;

		case 't':
			do_user_opts_update = 1;
			update_user_mode("TX", optarg, &req.tx);
			break;

		case 'o':
			do_user_opts_update = 1;
			update_user_mode("OFF", optarg, &req.off);
			break;

		case 'R':
			do_user_opts_update = 1;
			req.rx.do_update = true;
			req.rx.change_on_mode_entry = false;
			req.rx.enabled = false;
			req.tx.do_update = true;
			req.tx.change_on_mode_entry = false;
			req.tx.enabled = false;
			req.off.do_update = true;
			req.off.change_on_mode_entry = true;
			req.off.enabled = false;
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

	if (!(biast_enable >= 0 || do_user_opts_update)) {
		usage();
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
		result = hackrf_set_user_bias_t_opts(device, &req);
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
