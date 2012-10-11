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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>

const uint16_t hackrf_usb_pid = 0x1d50;
const uint16_t hackrf_usb_vid = 0x604b;

typedef enum {
	TRANSCEIVER_MODE_RX,
	TRANSCEIVER_MODE_TX,
} transceiver_mode_t;
static transceiver_mode_t transceiver_mode = TRANSCEIVER_MODE_RX;

static float
TimevalDiff(const struct timeval *a, const struct timeval *b)
{
   return (a->tv_sec - b->tv_sec) + 1e-6f * (a->tv_usec - b->tv_usec);
}

int fd = -1;
struct timeval time_start;
volatile uint32_t byte_count = 0;

void write_callback(struct libusb_transfer* transfer) {
    if( transfer->status == LIBUSB_TRANSFER_COMPLETED ) {
        byte_count += transfer->actual_length;
		write(fd, transfer->buffer, transfer->actual_length);
        libusb_submit_transfer(transfer);
    } else {
        printf("transfer status was not 'completed'\n");
    }
}

void read_callback(struct libusb_transfer* transfer) {
	if( transfer->status == LIBUSB_TRANSFER_COMPLETED ) {
		byte_count += transfer->actual_length;
		read(fd, transfer->buffer, transfer->actual_length);
		libusb_submit_transfer(transfer);
	} else {
		printf("transfer status was not 'completed'\n");
	}
}

libusb_device_handle* open_device(libusb_context* const context) {
    int result = libusb_init(NULL);
    if( result != 0 ) {
        printf("libusb_init() failed: %d\n", result);
        return NULL;
    }
    
    libusb_device_handle* device = libusb_open_device_with_vid_pid(context, hackrf_usb_pid, hackrf_usb_vid);
    if( device == NULL ) {
        printf("libusb_open_device_with_vid_pid() failed\n");
        return NULL;
    }
    
    //int speed = libusb_get_device_speed(device);
    //printf("device speed: %d\n", speed);

    result = libusb_set_configuration(device, 1);
    if( result != 0 ) {
    	libusb_close(device);
        printf("libusb_set_configuration() failed: %d\n", result);
        return NULL;
    }

    result = libusb_claim_interface(device, 0);
    if( result != 0 ) {
    	libusb_close(device);
        printf("libusb_claim_interface() failed: %d\n", result);
        return NULL;
    }

	return device;
}

void free_transfers(struct libusb_transfer** const transfers, const uint32_t transfer_count) {
	for(uint32_t transfer_index=0; transfer_index<transfer_count; transfer_index++) {
		libusb_free_transfer(transfers[transfer_index]);
	}
	free(transfers);
}

struct libusb_transfer** prepare_transfers(
	libusb_device_handle* const device,
	const uint_fast8_t endpoint_address,
	const uint32_t transfer_count,
	const uint32_t buffer_size,
	libusb_transfer_cb_fn callback
) {
    struct libusb_transfer** const transfers = calloc(transfer_count, sizeof(struct libusb_transfer));
	if( transfers == NULL ) {
		return NULL;
	}
	
    for(uint32_t transfer_index=0; transfer_index<transfer_count; transfer_index++) {
        transfers[transfer_index] = libusb_alloc_transfer(0);
        if( transfers[transfer_index] == NULL ) {
			free_transfers(transfers, transfer_count);
            printf("libusb_alloc_transfer() failed\n");
            return NULL;
        }
        
        libusb_fill_bulk_transfer(
            transfers[transfer_index],
            device,
            endpoint_address,
            (unsigned char*)malloc(buffer_size),
            buffer_size,
            callback,
            NULL,
            0
            );
        
        if( transfers[transfer_index]->buffer == NULL ) {
			free_transfers(transfers, transfer_count);
            printf("malloc() failed\n");
            return NULL;
        }
        
        int error = libusb_submit_transfer(transfers[transfer_index]);
        if( error != 0 ) {
			free_transfers(transfers, transfer_count);
            printf("libusb_submit_transfer() failed: %d\n", error);
            return NULL;
        }
    }

	return transfers;
}

static void usage() {
	printf("Usage:\n");
	printf("\tGo fish.\n");
}

int main(int argc, char** argv) {
	int opt;
	bool receive = false;
	bool transmit = false;
	const char* path = NULL;
	
	while( (opt = getopt(argc, argv, "r:t:")) != EOF ) {
		switch( opt ) {
		case 'r':
			receive = true;
			path = optarg;
			break;
		
		case 't':
			transmit = true;
			path = optarg;
			break;
		
		default:
			usage();
			return 1;
		}
	}
	
	if( transmit == receive ) {
		if( transmit == true ) {
			fprintf(stderr, "receive and transmit options are mutually exclusive\n");
		} else {
			fprintf(stderr, "specify either transmit or receive option\n");
		}
		return 1;
	}

	if( receive ) {
		transceiver_mode = TRANSCEIVER_MODE_RX;
	}
	
	if( transmit ) {
		transceiver_mode = TRANSCEIVER_MODE_TX;
	}
	
	if( path == NULL ) {
		fprintf(stderr, "specify a path to a file to transmit/receive\n");
		return 1;
	}
	
	fd = -1;
	uint_fast8_t endpoint_address = 0;
	libusb_transfer_cb_fn callback = NULL;
	
	if( transceiver_mode == TRANSCEIVER_MODE_RX ) {
		fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
		endpoint_address = 0x81;
		callback = &write_callback;
	} else {
		fd = open(path, O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
		endpoint_address = 0x02;
		callback = &read_callback;
	}
	
	if( fd == -1 ) {
        printf("Failed to open file: errno %d\n", errno);
		return fd;
	}

    libusb_context* const context = NULL;
	libusb_device_handle* const device = open_device(context);
    if( device == NULL ) {
		return -3;
	}

	const uint32_t transfer_count = 1024;
	const uint32_t buffer_size = 16384;
	struct libusb_transfer** const transfers = prepare_transfers(
		device, endpoint_address, transfer_count, buffer_size, callback
	);
	if( transfers == NULL ) {
		return -4;
	}
	
    //////////////////////////////////////////////////////////////
    
    struct timeval timeout = { 0, 500000 };
    struct timeval time_now;

    const double progress_interval = 1.0;
    gettimeofday(&time_start, NULL);

    uint32_t call_count = 0;
    do {
        int error = libusb_handle_events_timeout(context, &timeout);
        if( error != 0 ) {
            printf("libusb_handle_events_timeout() failed: %d\n", error);
            return -10;
        }
        
        if( (call_count & 0xFF) == 0 ) {
            gettimeofday(&time_now, NULL);
            const float time_difference = TimevalDiff(&time_now, &time_start);
            if( time_difference >= progress_interval ) {
                const float rate = (float)byte_count / time_difference;
                printf("%.1f/%.3f = %.1f MiB/second\n",
                    byte_count / 1e6f,
                    time_difference,
                    rate / 1e6f
                );
                time_start = time_now;
                byte_count = 0;
            }
        }
        call_count += 1;
    } while(1);

	free_transfers(transfers, transfer_count);
	
    int result = libusb_release_interface(device, 0);
    if( result != 0 ) {
        printf("libusb_release_interface() failed: %d\n", result);
        return -2000;
    }
    
    libusb_close(device);
    
    libusb_exit(context);
    
    return 0;
}
