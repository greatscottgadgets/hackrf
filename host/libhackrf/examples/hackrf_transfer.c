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

#include <hackrf.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

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
volatile uint32_t byte_count = 0;

int rx_callback(hackrf_transfer* transfer) {
	if( fd != -1 ) {
		byte_count += transfer->valid_length;
		const ssize_t bytes_written = write(fd, transfer->buffer, transfer->valid_length);
		if( bytes_written == transfer->valid_length ) {
       		return 0;
		} else {
			close(fd);
			fd = -1;
			return -1;
		}
	} else {
		return -1;
	}
}

int tx_callback(hackrf_transfer* transfer) {
	if( fd != -1 ) {
		byte_count += transfer->valid_length;
		const ssize_t bytes_read = read(fd, transfer->buffer, transfer->valid_length);
		if( bytes_read == transfer->valid_length ) {
			return 0;
		} else {
			close(fd);
			fd = -1;
			return -1;
		}
	} else {
		return -1;
	}
}

static void usage() {
	printf("Usage:\n");
	printf("\t-r <filename> # Receive data into file.\n");
	printf("\t-t <filename> # Transmit data from file.\n");
}

static hackrf_device* device = NULL;

void sigint_callback_handler(int signum) {
	hackrf_stop_rx(device);
	hackrf_stop_tx(device);
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
	
	int result = hackrf_init();
	if( result != HACKRF_SUCCESS ) {
		printf("hackrf_init() failed: %s (%d)\n", hackrf_error_name(result), result);
		return -1;
	}
	
	result = hackrf_open(&device);
	if( result != HACKRF_SUCCESS ) {
		printf("hackrf_open() failed: %s (%d)\n", hackrf_error_name(result), result);
		return -1;
	}
	
	fd = -1;	
	if( transceiver_mode == TRANSCEIVER_MODE_RX ) {
		fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
	} else {
		fd = open(path, O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
	}
	
	if( fd == -1 ) {
        printf("Failed to open file: errno %d\n", errno);
		return fd;
	}

	signal(SIGINT, sigint_callback_handler);
	
	result = hackrf_sample_rate_set(device, 10000000);
	if( result != HACKRF_SUCCESS ) {
		printf("hackrf_sample_rate_set() failed: %s (%d)\n", hackrf_error_name(result), result);
		return -1;
	}
	
	result = hackrf_baseband_filter_bandwidth_set(device, 5000000);
	if( result != HACKRF_SUCCESS ) {
		printf("hackrf_baseband_filter_bandwidth_set() failed: %s (%d)\n", hackrf_error_name(result), result);
		return -1;
	}
	
	if( transceiver_mode == TRANSCEIVER_MODE_RX ) {
		result = hackrf_start_rx(device, rx_callback);
	} else {
		result = hackrf_start_tx(device, tx_callback);
	}
	if( result != HACKRF_SUCCESS ) {
		printf("hackrf_start_?x() failed: %s (%d)\n", hackrf_error_name(result), result);
		return -1;
	}
	
	struct timeval time_start;
    gettimeofday(&time_start, NULL);

	while( hackrf_is_streaming(device) ) {
		sleep(1);

    	struct timeval time_now;
		gettimeofday(&time_now, NULL);
		
		uint32_t byte_count_now = byte_count;
		byte_count = 0;
		
		const float time_difference = TimevalDiff(&time_now, &time_start);
	    const float rate = (float)byte_count_now / time_difference;
	    printf("%4.1f MiB / %5.3f sec = %4.1f MiB/second\n",
	        byte_count_now / 1e6f,
	        time_difference,
	        rate / 1e6f
	    );

	    time_start = time_now;
	}
	
	result = hackrf_close(device);
	if( result != HACKRF_SUCCESS ) {
		printf("hackrf_close() failed: %s (%d)\n", hackrf_error_name(result), result);
		return -1;
	}
	
	hackrf_exit();
    
    return 0;
}
