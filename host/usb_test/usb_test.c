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
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>

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

int main(int argc, char** argv) {
    if( argc != 2 ) {
        printf("Usage: usb_test <file to capture to>\n");
        return -1;
    }
	
    const uint32_t buffer_size = 16384;

	fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
	if( fd == -1 ) {
        printf("Failed to open file for write\n");
		return -2;
	}

    libusb_context* context = NULL;
    int result = libusb_init(NULL);
    if( result != 0 ) {
        printf("libusb_init() failed: %d\n", result);
        return -4;
    }
    
    libusb_device_handle* device = libusb_open_device_with_vid_pid(context, 0x1d50, 0x604b);
    if( device == NULL ) {
        printf("libusb_open_device_with_vid_pid() failed\n");
        return -5;
    }
    
    //int speed = libusb_get_device_speed(device);
    //printf("device speed: %d\n", speed);

    result = libusb_set_configuration(device, 1);
    if( result != 0 ) {
        printf("libusb_set_configuration() failed: %d\n", result);
        return -6;
    }

    result = libusb_claim_interface(device, 0);
    if( result != 0 ) {
        printf("libusb_claim_interface() failed: %d\n", result);
        return -7;
    }
    
    unsigned char endpoint_address = 0x81;
    //unsigned char endpoint_address = 0x02;

    const uint32_t transfer_count = 1024;
    struct libusb_transfer* transfers[transfer_count];
    for(uint32_t transfer_index=0; transfer_index<transfer_count; transfer_index++) {
        transfers[transfer_index] = libusb_alloc_transfer(0);
        if( transfers[transfer_index] == 0 ) {
            printf("libusb_alloc_transfer() failed\n");
            return -6;
        }
        
        libusb_fill_bulk_transfer(
            transfers[transfer_index],
            device,
            endpoint_address,
            (unsigned char*)malloc(buffer_size),
            buffer_size,
            &write_callback,
            NULL,
            0
            );
        
        if( transfers[transfer_index]->buffer == 0 ) {
            printf("malloc() failed\n");
            return -7;
        }
        
        int error = libusb_submit_transfer(transfers[transfer_index]);
        if( error != 0 ) {
            printf("libusb_submit_transfer() failed: %d\n", error);
            return -8;
        }
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

    result = libusb_release_interface(device, 0);
    if( result != 0 ) {
        printf("libusb_release_interface() failed: %d\n", result);
        return -2000;
    }
    
    libusb_close(device);
    
    libusb_exit(context);
    
    return 0;
}
