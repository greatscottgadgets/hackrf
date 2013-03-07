/*
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
#include <stdbool.h>
#include <string.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h> 
#else
#include <unistd.h>
#endif

#include <sys/time.h>
#include <signal.h>

#define FREQ_MIN_HZ	(30000000ull) /* 30MHz */
#define FREQ_MAX_HZ	(6000000000ull) /* 6000MHz */

#if defined _WIN32
	#define sleep(a) Sleep( (a*1000) )
#endif

typedef enum {
	TRANSCEIVER_MODE_OFF = 0,
	TRANSCEIVER_MODE_RX = 1,
	TRANSCEIVER_MODE_TX = 2
} transceiver_mode_t;
static transceiver_mode_t transceiver_mode = TRANSCEIVER_MODE_RX;

static float
TimevalDiff(const struct timeval *a, const struct timeval *b)
{
   return (a->tv_sec - b->tv_sec) + 1e-6f * (a->tv_usec - b->tv_usec);
}

int parse_u64(char* s, uint64_t* const value) {
	uint_fast8_t base = 10;
	if( strlen(s) > 2 ) {
		if( s[0] == '0' ) {
			if( (s[1] == 'x') || (s[1] == 'X') ) {
				base = 16;
				s += 2;
			} else if( (s[1] == 'b') || (s[1] == 'B') ) {
				base = 2;
				s += 2;
			}
		}
	}

	char* s_end = s;
	const unsigned long long u64_value = strtoull(s, &s_end, base);
	if( (s != s_end) && (*s_end == 0) ) {
		*value = u64_value;
		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_INVALID_PARAM;
	}
}

FILE* fd = NULL;
volatile uint32_t byte_count = 0;

bool receive = false;
bool transmit = false;
struct timeval time_start;
struct timeval t_start;
	
bool freq = false;
uint64_t freq_hz;
	
int rx_callback(hackrf_transfer* transfer) {
	if( fd != NULL ) 
	{
		byte_count += transfer->valid_length;
		const ssize_t bytes_written = fwrite(transfer->buffer, 1, transfer->valid_length, fd);
		if( bytes_written == transfer->valid_length ) {
       		return 0;
		} else {
			fclose(fd);
			fd = NULL;
			return -1;
		}
	} else {
		return -1;
	}
}

int tx_callback(hackrf_transfer* transfer) {
	if( fd != NULL )
	{
		byte_count += transfer->valid_length;
		const ssize_t bytes_read = fread(transfer->buffer, 1, transfer->valid_length, fd);
		if( bytes_read == transfer->valid_length ) {
			return 0;
		} else {
			fclose(fd);
			fd = NULL;
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
	printf("\t-f <set_freq_MHz> # Set Frequency in MHz (between [%lld, %lld[).\n", FREQ_MIN_HZ, FREQ_MAX_HZ);
}

static hackrf_device* device = NULL;

void sigint_callback_handler(int signum) 
{
	int result;
	printf("Caught signal %d\n", signum);

	struct timeval t_end;
	gettimeofday(&t_end, NULL);
	const float time_diff = TimevalDiff(&t_end, &t_start);
	printf("Total time: %5.5f s\n", time_diff);

	if(device != NULL)
	{
		if( receive ) 
		{
			printf("hackrf_stop_rx \n");
			result = hackrf_stop_rx(device);
			if( result != HACKRF_SUCCESS ) {
				printf("hackrf_stop_rx() failed: %s (%d)\n", hackrf_error_name(result), result);
			}else {
				printf("hackrf_stop_rx() done\n");
			}
		}
	
		if( transmit ) 
		{
			result = hackrf_stop_tx(device);
			if( result != HACKRF_SUCCESS ) {
				printf("hackrf_stop_tx() failed: %s (%d)\n", hackrf_error_name(result), result);
			}else {
				printf("hackrf_stop_tx() done\n");
			}
		}
		
		result = hackrf_close(device);
		if( result != HACKRF_SUCCESS ) 
		{
			printf("hackrf_close() failed: %s (%d)\n", hackrf_error_name(result), result);
		}
		
		printf("hackrf_close() done\n");
		
		hackrf_exit();
	}
		
	if(fd != NULL)
	{
		fclose(fd);
		fd = NULL;
		printf("fclose() file handle done\n");
	}
	
	printf("Exit\n");	
	/* Terminate program */
	exit(signum);
}

int main(int argc, char** argv) {
	int opt;
	const char* path = NULL;
	int result;
	
	while( (opt = getopt(argc, argv, "r:t:f:")) != EOF ) {
		result = HACKRF_SUCCESS;
		switch( opt ) {
		case 'r':
			receive = true;
			path = optarg;
			break;
		
		case 't':
			transmit = true;
			path = optarg;
			break;
		
		case 'f':
			freq = true;
			result = parse_u64(optarg, &freq_hz);
			break;
			
		default:
			usage();
			return EXIT_FAILURE;
		}
		
		if( result != HACKRF_SUCCESS ) {
			printf("argument error: %s (%d)\n", hackrf_error_name(result), result);
			usage();
			break;
		}		
	}

	if( freq ) {
		if( (freq_hz >= FREQ_MAX_HZ) || (freq_hz < FREQ_MIN_HZ) )
			printf("argument error: frequency shall be between [%lld, %lld[.\n", FREQ_MIN_HZ, FREQ_MAX_HZ);
	}
	
	if( transmit == receive ) 
	{
		if( transmit == true ) 
		{
			fprintf(stderr, "receive and transmit options are mutually exclusive\n");
		} else {
			fprintf(stderr, "specify either transmit or receive option\n");
		}
		usage();
		return EXIT_FAILURE;
	}

	if( receive ) {
		transceiver_mode = TRANSCEIVER_MODE_RX;
	}
	
	if( transmit ) {
		transceiver_mode = TRANSCEIVER_MODE_TX;
	}
	
	if( path == NULL ) {
		fprintf(stderr, "specify a path to a file to transmit/receive\n");
		usage();
		return EXIT_FAILURE;
	}
	
	result = hackrf_init();
	if( result != HACKRF_SUCCESS ) {
		printf("hackrf_init() failed: %s (%d)\n", hackrf_error_name(result), result);
		usage();
		return EXIT_FAILURE;
	}
	
	result = hackrf_open(&device);
	if( result != HACKRF_SUCCESS ) {
		printf("hackrf_open() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}
	
	if( transceiver_mode == TRANSCEIVER_MODE_RX ) 
	{
		fd = fopen(path, "wb");
	} else {
		fd = fopen(path, "rb");
	}
	
	if( fd == NULL ) {
		printf("Failed to open file: %s\n", path);
		return EXIT_FAILURE;
	}

	signal(SIGINT, sigint_callback_handler);
	
	result = hackrf_sample_rate_set(device, 10000000);
	if( result != HACKRF_SUCCESS ) {
		printf("hackrf_sample_rate_set() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}
	
	result = hackrf_baseband_filter_bandwidth_set(device, 5000000);
	if( result != HACKRF_SUCCESS ) {
		printf("hackrf_baseband_filter_bandwidth_set() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}
	
	if( transceiver_mode == TRANSCEIVER_MODE_RX ) {
		result = hackrf_start_rx(device, rx_callback);
	} else {
		result = hackrf_start_tx(device, tx_callback);
	}
	if( result != HACKRF_SUCCESS ) {
		printf("hackrf_start_?x() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	if( freq ) {
		printf("call hackrf_set_freq(%lld Hz)\n", freq_hz);
		result = hackrf_set_freq(device, freq_hz);
		if( result != HACKRF_SUCCESS ) {
			printf("hackrf_set_freq() failed: %s (%d)\n", hackrf_error_name(result), result);
			return EXIT_FAILURE;
		}
	}
	
	gettimeofday(&t_start, NULL);
	gettimeofday(&time_start, NULL);

	while( hackrf_is_streaming(device) ) 
	{
		sleep(1);
		
		struct timeval time_now;
		gettimeofday(&time_now, NULL);
		
		uint32_t byte_count_now = byte_count;
		byte_count = 0;
		
		const float time_difference = TimevalDiff(&time_now, &time_start);
		const float rate = (float)byte_count_now / time_difference;
		printf("%4.1f MiB / %5.3f sec = %4.1f MiB/second\n",
				(byte_count_now / 1e6f), time_difference, (rate / 1e6f) );

		time_start = time_now;
	}

	result = hackrf_close(device);
	if( result != HACKRF_SUCCESS ) {
		printf("hackrf_close() failed: %s (%d)\n", hackrf_error_name(result), result);
		return -1;
	}
	
	hackrf_exit();
	
	if(fd != NULL)
	{
		fclose(fd);
	}

	return EXIT_SUCCESS;
}
