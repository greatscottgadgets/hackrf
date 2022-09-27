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

int main(void)
{
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

	result = hackrf_init();
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_init() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		return EXIT_FAILURE;
	}

	printf("hackrf_info version: %s\n", TOOL_RELEASE);
	printf("libhackrf version: %s (%s)\n",
	       hackrf_library_release(),
	       hackrf_library_version());

	list = hackrf_device_list();

	if (list->devicecount < 1) {
		printf("No HackRF boards found.\n");
		return EXIT_FAILURE;
	}

	for (i = 0; i < list->devicecount; i++) {
		if (i > 0) {
			printf("\n");
		}

		printf("Found HackRF\n");
		printf("Index: %d\n", i);

		if (list->serial_numbers[i]) {
			printf("Serial number: %s\n", list->serial_numbers[i]);
		}

		device = NULL;
		result = hackrf_device_list_open(list, i, &device);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_open() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			if (result == HACKRF_ERROR_LIBUSB) {
				continue;
			}
			return EXIT_FAILURE;
		}

		result = hackrf_board_id_read(device, &board_id);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_board_id_read() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			return EXIT_FAILURE;
		}
		printf("Board ID Number: %d (%s)\n",
		       board_id,
		       hackrf_board_id_name(board_id));

		result = hackrf_version_string_read(device, &version[0], 255);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_version_string_read() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			return EXIT_FAILURE;
		}

		result = hackrf_usb_api_version_read(device, &usb_version);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_usb_api_version_read() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			return EXIT_FAILURE;
		}
		printf("Firmware Version: %s (API:%x.%02x)\n",
		       version,
		       (usb_version >> 8) & 0xFF,
		       usb_version & 0xFF);

		result = hackrf_board_partid_serialno_read(device, &read_partid_serialno);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_board_partid_serialno_read() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			return EXIT_FAILURE;
		}
		printf("Part ID Number: 0x%08x 0x%08x\n",
		       read_partid_serialno.part_id[0],
		       read_partid_serialno.part_id[1]);

		if ((usb_version >= 0x0106) && ((board_id == 2) || (board_id == 4))) {
			result = hackrf_board_rev_read(device, &board_rev);
			if (result != HACKRF_SUCCESS) {
				fprintf(stderr,
					"hackrf_board_rev_read() failed: %s (%d)\n",
					hackrf_error_name(result),
					result);
				return EXIT_FAILURE;
			}
			print_board_rev(board_rev);
		}
		if (usb_version >= 0x0106) {
			result = hackrf_supported_platform_read(
				device,
				&supported_platform);
			if (result != HACKRF_SUCCESS) {
				fprintf(stderr,
					"hackrf_supported_platform_read() failed: %s (%d)\n",
					hackrf_error_name(result),
					result);
				return EXIT_FAILURE;
			}
			print_supported_platform(supported_platform, board_id);
		}

		result = hackrf_get_operacake_boards(device, &operacakes[0]);
		if ((result != HACKRF_SUCCESS) &&
		    (result != HACKRF_ERROR_USB_API_VERSION)) {
			fprintf(stderr,
				"hackrf_get_operacake_boards() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			return EXIT_FAILURE;
		}
		if (result == HACKRF_SUCCESS) {
			for (j = 0; j < 8; j++) {
				if (operacakes[j] == HACKRF_OPERACAKE_ADDRESS_INVALID) {
					break;
				}
				printf("Opera Cake found, address: %d\n", operacakes[j]);
			}
		}

#ifdef HACKRF_ISSUE_609_IS_FIXED
		uint32_t cpld_crc = 0;
		result = hackrf_cpld_checksum(device, &cpld_crc);
		if ((result != HACKRF_SUCCESS) &&
		    (result != HACKRF_ERROR_USB_API_VERSION)) {
			fprintf(stderr,
				"hackrf_cpld_checksum() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			return EXIT_FAILURE;
		}
		if (result == HACKRF_SUCCESS) {
			printf("CPLD checksum: 0x%08x\n", cpld_crc);
		}
#endif /* HACKRF_ISSUE_609_IS_FIXED */

		result = hackrf_close(device);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_close() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
		}
	}

	hackrf_device_list_free(list);
	hackrf_exit();

	return EXIT_SUCCESS;
}
