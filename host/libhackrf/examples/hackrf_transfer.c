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

#define FREQ_ONE_MHZ (1000000)

#define FREQ_MIN_HZ	(30000000ull) /* 30MHz */
#define FREQ_MAX_HZ	(6000000000ull) /* 6000MHz */

#define DEFAULT_SAMPLE_RATE_HZ (10000000) /* 10MHz default sample rate */

#define DEFAULT_BASEBAND_FILTER_BANDWIDTH (5000000) /* 5MHz default */

#define SAMPLES_TO_XFER_MAX (0x8000000000000000ull) /* Max value */

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

int parse_u32(char* s, uint32_t* const value) {
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
	const unsigned long ulong_value = strtoul(s, &s_end, base);
	if( (s != s_end) && (*s_end == 0) ) {
		*value = ulong_value;
		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_INVALID_PARAM;
	}
}

volatile bool do_exit = false;

FILE* fd = NULL;
volatile uint32_t byte_count = 0;

bool receive = false;
bool transmit = false;
struct timeval time_start;
struct timeval t_start;
	
bool freq = false;
uint64_t freq_hz;

bool amp = false;
uint32_t amp_enable;


bool sample_rate = false;
uint32_t sample_rate_hz;

bool limit_num_samples = false;
uint64_t samples_to_xfer = 0;
uint64_t bytes_to_xfer = 0;

int rx_callback(hackrf_transfer* transfer) {
	int bytes_to_write;

	if( fd != NULL ) 
	{
		byte_count += transfer->valid_length;
		bytes_to_write = transfer->valid_length;
		if (limit_num_samples) {
			if (bytes_to_write >= bytes_to_xfer) {
				bytes_to_write = bytes_to_xfer;
			}
			bytes_to_xfer -= bytes_to_write;
		}
		const ssize_t bytes_written = fwrite(transfer->buffer, 1, bytes_to_write, fd);
		if ((bytes_written != bytes_to_write)
				|| (limit_num_samples && (bytes_to_xfer == 0))) {
			fclose(fd);
			fd = NULL;
			return -1;
		} else {
			return 0;
		}
	} else {
		return -1;
	}
}

int tx_callback(hackrf_transfer* transfer) {
	int bytes_to_read;

	if( fd != NULL )
	{
		byte_count += transfer->valid_length;
		bytes_to_read = transfer->valid_length;
		if (limit_num_samples) {
			if (bytes_to_read >= bytes_to_xfer) {
				/*
				 * In this condition, we probably tx some of the previous
				 * buffer contents at the end.  :-(
				 */
				bytes_to_read = bytes_to_xfer;
			}
			bytes_to_xfer -= bytes_to_read;
		}
		const ssize_t bytes_read = fread(transfer->buffer, 1, bytes_to_read, fd);
		if ((bytes_read != bytes_to_read)
				|| (limit_num_samples && (bytes_to_xfer == 0))) {
			fclose(fd);
			fd = NULL;
			return -1;
		} else {
			return 0;
		}
	} else {
		return -1;
	}
}

static void usage() {
	printf("Usage:\n");
	printf("\t-r <filename> # Receive data into file.\n");
	printf("\t-t <filename> # Transmit data from file.\n");
	printf("\t[-f set_freq_hz] # Set Freq in Hz between [%lldMHz, %lldMHz[.\n", FREQ_MIN_HZ/FREQ_ONE_MHZ, FREQ_MAX_HZ/FREQ_ONE_MHZ);
	printf("\t[-a set_amp] # Set Amp 1=Enable, 0=Disable.\n");
	printf("\t[-s sample_rate_hz] # Set sample rate in Hz (5/10/12.5/16/20MHz, default %dMHz).\n", DEFAULT_SAMPLE_RATE_HZ/FREQ_ONE_MHZ);
	printf("\t[-n num_samples] # Number of samples to transfer (default is unlimited).\n");
}

static hackrf_device* device = NULL;

void sigint_callback_handler(int signum) 
{
	fprintf(stdout, "Caught signal %d\n", signum);
	do_exit = true;
}

int main(int argc, char** argv) {
	int opt;
	const char* path = NULL;
	int result;
	
	while( (opt = getopt(argc, argv, "r:t:f:a:s:n:")) != EOF )
	{
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

		case 'a':
			amp = true;
			result = parse_u32(optarg, &amp_enable);
			break;

		case 's':
			sample_rate = true;
			result = parse_u32(optarg, &sample_rate_hz);
			break;

		case 'n':
			limit_num_samples = true;
			result = parse_u64(optarg, &samples_to_xfer);
			bytes_to_xfer = samples_to_xfer * 2ull;
			break;

		default:
			usage();
			return EXIT_FAILURE;
		}
		
		if( result != HACKRF_SUCCESS ) {
			printf("argument error: '-%c %s' %s (%d)\n", opt, optarg, hackrf_error_name(result), result);
			usage();
			return EXIT_FAILURE;
		}		
	}

	if (samples_to_xfer >= SAMPLES_TO_XFER_MAX) {
		printf("argument error: num_samples must be less than %llu/%lluMio\n", SAMPLES_TO_XFER_MAX, SAMPLES_TO_XFER_MAX/FREQ_ONE_MHZ);
		usage();
		return EXIT_FAILURE;
	}

	if( freq ) {
		if( (freq_hz >= FREQ_MAX_HZ) || (freq_hz < FREQ_MIN_HZ) )
		{
			printf("argument error: set_freq_hz shall be between [%llu, %llu[.\n", FREQ_MIN_HZ, FREQ_MAX_HZ);
			usage();
			return EXIT_FAILURE;
		}
	}
	
	if( amp ) {
		if( amp_enable > 1 )
		{
			printf("argument error: set_amp shall be 0 or 1.\n");
			usage();
			return EXIT_FAILURE;
		}
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

	signal(SIGINT, &sigint_callback_handler);
	signal(SIGILL, &sigint_callback_handler);
	signal(SIGFPE, &sigint_callback_handler);
	signal(SIGSEGV, &sigint_callback_handler);
	signal(SIGTERM, &sigint_callback_handler);
	signal(SIGABRT, &sigint_callback_handler);
	
	if( sample_rate ) 
	{
		printf("call hackrf_sample_rate_set(%u Hz/%u MHz)\n", sample_rate_hz, sample_rate_hz/FREQ_ONE_MHZ);
		result = hackrf_sample_rate_set(device, sample_rate_hz);
		if( result != HACKRF_SUCCESS ) {
			printf("hackrf_sample_rate_set() failed: %s (%d)\n", hackrf_error_name(result), result);
			return EXIT_FAILURE;
		}
	}else
	{
		printf("call hackrf_sample_rate_set(%u Hz/%u MHz)\n", DEFAULT_SAMPLE_RATE_HZ, DEFAULT_SAMPLE_RATE_HZ/FREQ_ONE_MHZ);
		result = hackrf_sample_rate_set(device, DEFAULT_SAMPLE_RATE_HZ);
		if( result != HACKRF_SUCCESS ) {
			printf("hackrf_sample_rate_set() failed: %s (%d)\n", hackrf_error_name(result), result);
			return EXIT_FAILURE;
		}
	}

	printf("call hackrf_baseband_filter_bandwidth_set(%d Hz/%d MHz)\n",
		DEFAULT_BASEBAND_FILTER_BANDWIDTH, DEFAULT_BASEBAND_FILTER_BANDWIDTH/FREQ_ONE_MHZ);
	result = hackrf_baseband_filter_bandwidth_set(device, DEFAULT_BASEBAND_FILTER_BANDWIDTH);
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
		printf("call hackrf_set_freq(%llu Hz/%llu MHz)\n", freq_hz, (freq_hz/FREQ_ONE_MHZ) );
		result = hackrf_set_freq(device, freq_hz);
		if( result != HACKRF_SUCCESS ) {
			printf("hackrf_set_freq() failed: %s (%d)\n", hackrf_error_name(result), result);
			return EXIT_FAILURE;
		}
	}

	if( amp ) {
		printf("call hackrf_set_amp_enable(%u)\n", amp_enable);
		result = hackrf_set_amp_enable(device, (uint8_t)amp_enable);
		if( result != HACKRF_SUCCESS ) {
			printf("hackrf_set_amp_enable() failed: %s (%d)\n", hackrf_error_name(result), result);
			return EXIT_FAILURE;
		}
	}
	
	if( limit_num_samples ) {
		printf("samples_to_xfer %llu/%lluMio\n", samples_to_xfer, (samples_to_xfer/FREQ_ONE_MHZ) );
	}
	
	gettimeofday(&t_start, NULL);
	gettimeofday(&time_start, NULL);

	printf("Stop with Ctrl-C\n");
	while( (hackrf_is_streaming(device)) &&
		   (do_exit == false) ) 
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

    if (do_exit)
        printf("\nUser cancel, exiting...\n");
    else
        printf("\nExiting...\n");
	
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
		}else {
			printf("hackrf_close() done\n");
		}
		
		hackrf_exit();
		printf("hackrf_exit() done\n");
	}
		
	if(fd != NULL)
	{
		fclose(fd);
		fd = NULL;
		printf("fclose(fd) done\n");
	}
	printf("exit\n");
	return EXIT_SUCCESS;
}
