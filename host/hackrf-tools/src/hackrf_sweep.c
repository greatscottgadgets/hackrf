/*
 * Copyright 2016 Dominic Spill <dominicgs@gmail.com>
 * Copyright 2016 Mike Walters <mike@flomp.net>
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
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <fftw3.h>
#include <math.h>

#ifndef bool
typedef int bool;
#define true 1
#define false 0
#endif

#ifdef _WIN32
#include <windows.h>

#ifdef _MSC_VER

#ifdef _WIN64
typedef int64_t ssize_t;
#else
typedef int32_t ssize_t;
#endif

#define strtoull _strtoui64
#define snprintf _snprintf

int gettimeofday(struct timeval *tv, void* ignored) {
	FILETIME ft;
	unsigned __int64 tmp = 0;
	if (NULL != tv) {
		GetSystemTimeAsFileTime(&ft);
		tmp |= ft.dwHighDateTime;
		tmp <<= 32;
		tmp |= ft.dwLowDateTime;
		tmp /= 10;
		tmp -= 11644473600000000Ui64;
		tv->tv_sec = (long)(tmp / 1000000UL);
		tv->tv_usec = (long)(tmp % 1000000UL);
	}
	return 0;
}

#endif
#endif

#if defined(__GNUC__)
#include <unistd.h>
#include <sys/time.h>
#endif

#include <signal.h>

#define FD_BUFFER_SIZE (8*1024)

#define FREQ_ONE_MHZ (1000000ull)

#define FREQ_MIN_HZ	(0ull) /* 0 Hz */
#define FREQ_MAX_HZ	(7250000000ull) /* 7250MHz */

#define DEFAULT_SAMPLE_RATE_HZ (20000000) /* 20MHz default sample rate */
#define DEFAULT_BASEBAND_FILTER_BANDWIDTH (15000000) /* 5MHz default */

#if defined _WIN32
	#define sleep(a) Sleep( (a*1000) )
#endif

static float TimevalDiff(const struct timeval *a, const struct timeval *b) {
   return (a->tv_sec - b->tv_sec) + 1e-6f * (a->tv_usec - b->tv_usec);
}

int parse_u32(char* s, uint32_t* const value) {
	uint_fast8_t base = 10;
	char* s_end;
	uint64_t ulong_value;

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

	s_end = s;
	ulong_value = strtoul(s, &s_end, base);
	if( (s != s_end) && (*s_end == 0) ) {
		*value = (uint32_t)ulong_value;
		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_INVALID_PARAM;
	}
}

int parse_u32_range(char* s, uint32_t* const value_min, uint32_t* const value_max) {
	int result;

	char *sep = strchr(s, ':');
	if (!sep)
		return HACKRF_ERROR_INVALID_PARAM;

	*sep = 0;

	result = parse_u32(s, value_min);
	if (result != HACKRF_SUCCESS)
		return result;
	result = parse_u32(sep + 1, value_max);
	if (result != HACKRF_SUCCESS);
		return result;

	return HACKRF_SUCCESS;
}

volatile bool do_exit = false;

FILE* fd = NULL;
volatile uint32_t byte_count = 0;

struct timeval time_start;
struct timeval t_start;

bool amp = false;
uint32_t amp_enable;

bool antenna = false;
uint32_t antenna_enable;

bool freq_range = false;
uint32_t freq_min;
uint32_t freq_max;

int fftSize;
fftwf_complex *fftwIn = NULL;
fftwf_complex *fftwOut = NULL;
fftwf_plan fftwPlan = NULL;
float* pwr;
float* window;

float logPower(fftwf_complex in, float scale)
{
	float re = in[0] * scale;
	float im = in[1] * scale;
	float magsq = re * re + im * im;
	return log2f(magsq) * 10.0f / log2(10.0f);
}

int rx_callback(hackrf_transfer* transfer) {
	/* This is where we need to do interesting things with the samples
	 * FFT
	 * Throw away unused bins
	 * write output to pipe
	 */
	ssize_t bytes_to_write;
	ssize_t bytes_written;
	int8_t* buf;
	float frequency;
	int i, j;

	if( fd != NULL ) 
	{
		byte_count += transfer->valid_length;
		bytes_to_write = transfer->valid_length;
		buf = (int8_t*) transfer->buffer;
		for(j=0; j<16; j++) {
			if(buf[0] == 0x7F && buf[1] == 0x7F) {
				frequency = *(uint16_t*)&buf[2];
			}
			/* copy to fftwIn as floats */
			buf += 16384 - (fftSize * 2);
			for(i=0; i < fftSize; i++) {
				fftwIn[i][0] = buf[i*2] * window[i] * 1.0f / 128.0f;
				fftwIn[i][1] = buf[i*2+1] * window[i] * 1.0f / 128.0f;
			}
			buf += fftSize * 2;
			fftwf_execute(fftwPlan);
			for (i=0; i < fftSize; i++) {
				// Start from the middle of the FFTW array and wrap
				// to rearrange the data
				int k = i ^ (fftSize >> 1);
				pwr[i] = logPower(fftwOut[k], 1.0f / fftSize);
			}
			fwrite(&frequency, sizeof(float), 1, stdout);
			fwrite(pwr, sizeof(float), fftSize, stdout);
		}

		bytes_written = fwrite(transfer->buffer, 1, bytes_to_write, fd);
		if (bytes_written != bytes_to_write) {
			return -1;
		} else {
			return 0;
		}
	} else {
		return -1;
	}
}

static void usage() {
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\t[-d serial_number] # Serial number of desired HackRF.\n");
	fprintf(stderr, "\t[-a amp_enable] # RX/TX RF amplifier 1=Enable, 0=Disable.\n");
	fprintf(stderr, "\t[-f freq_min:freq_max # Specify minimum & maximum sweep frequencies (MHz).\n");
	fprintf(stderr, "\t[-p antenna_enable] # Antenna port power, 1=Enable, 0=Disable.\n");
	fprintf(stderr, "\t[-l gain_db] # RX LNA (IF) gain, 0-40dB, 8dB steps\n");
	fprintf(stderr, "\t[-g gain_db] # RX VGA (baseband) gain, 0-62dB, 2dB steps\n");
	fprintf(stderr, "\t[-x gain_db] # TX VGA (IF) gain, 0-47dB, 1dB steps\n");
}

static hackrf_device* device = NULL;

#ifdef _MSC_VER
BOOL WINAPI
sighandler(int signum) {
	if (CTRL_C_EVENT == signum) {
		fprintf(stderr, "Caught signal %d\n", signum);
		do_exit = true;
		return TRUE;
	}
	return FALSE;
}
#else
void sigint_callback_handler(int signum)  {
	fprintf(stderr, "Caught signal %d\n", signum);
	do_exit = true;
}
#endif

int main(int argc, char** argv) {
	int opt;
	const char* path = "/dev/null";
	const char* serial_number = NULL;
	int result;
	int exit_code = EXIT_SUCCESS;
	struct timeval t_end;
	float time_diff;
	unsigned int lna_gain=20, vga_gain=20, txvga_gain=0;
  
	while( (opt = getopt(argc, argv, "a:f:p:l:g:x:d:")) != EOF )
	{
		result = HACKRF_SUCCESS;
		switch( opt ) 
		{
		case 'd':
			serial_number = optarg;
			break;

		case 'a':
			amp = true;
			result = parse_u32(optarg, &amp_enable);
			break;

		case 'f':
			freq_range = true;
			result = parse_u32_range(optarg, &freq_min, &freq_max);
			fprintf(stderr, "Scanning %uMHz to %uMHz\n", freq_min, freq_max);
			break;

		case 'p':
			antenna = true;
			result = parse_u32(optarg, &antenna_enable);
			break;

		case 'l':
			result = parse_u32(optarg, &lna_gain);
			break;

		case 'g':
			result = parse_u32(optarg, &vga_gain);
			break;

		case 'x':
			result = parse_u32(optarg, &txvga_gain);
			break;

		default:
			fprintf(stderr, "unknown argument '-%c %s'\n", opt, optarg);
			usage();
			return EXIT_FAILURE;
		}
		
		if( result != HACKRF_SUCCESS ) {
			fprintf(stderr, "argument error: '-%c %s' %s (%d)\n", opt, optarg, hackrf_error_name(result), result);
			usage();
			return EXIT_FAILURE;
		}		
	}

	if (lna_gain % 8)
		fprintf(stderr, "warning: lna_gain (-l) must be a multiple of 8\n");

	if (vga_gain % 2)
		fprintf(stderr, "warning: vga_gain (-g) must be a multiple of 2\n");

	if( amp ) {
		if( amp_enable > 1 ) {
			fprintf(stderr, "argument error: amp_enable shall be 0 or 1.\n");
			usage();
			return EXIT_FAILURE;
		}
	}

	if (antenna) {
		if (antenna_enable > 1) {
			fprintf(stderr, "argument error: antenna_enable shall be 0 or 1.\n");
			usage();
			return EXIT_FAILURE;
		}
	}

	if (!freq_range) {
		fprintf(stderr, "argument error: must specify sweep frequency range (-f).\n");
		usage();
		return EXIT_FAILURE;
	}
	
	fftSize = 64;
    fftwIn = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * fftSize);
    fftwOut = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * fftSize);
    fftwPlan = fftwf_plan_dft_1d(fftSize, fftwIn, fftwOut, FFTW_FORWARD, FFTW_MEASURE);
	pwr = (float*)fftwf_malloc(sizeof(float) * fftSize);
	window = (float*)fftwf_malloc(sizeof(float) * fftSize);
	int i;
	for (i = 0; i < fftSize; i++) {
		window[i] = 0.5f * (1.0f - cos(2 * M_PI * i / (fftSize - 1)));
	}

	result = hackrf_init();
	if( result != HACKRF_SUCCESS ) {
		fprintf(stderr, "hackrf_init() failed: %s (%d)\n", hackrf_error_name(result), result);
		usage();
		return EXIT_FAILURE;
	}
	
	result = hackrf_open_by_serial(serial_number, &device);
	if( result != HACKRF_SUCCESS ) {
		fprintf(stderr, "hackrf_open() failed: %s (%d)\n", hackrf_error_name(result), result);
		usage();
		return EXIT_FAILURE;
	}
	
	fd = fopen(path, "wb");
	if( fd == NULL ) {
		fprintf(stderr, "Failed to open file: %s\n", path);
		return EXIT_FAILURE;
	}
	/* Change fd buffer to have bigger one to store or read data on/to HDD */
	result = setvbuf(fd , NULL , _IOFBF , FD_BUFFER_SIZE);
	if( result != 0 ) {
		fprintf(stderr, "setvbuf() failed: %d\n", result);
		usage();
		return EXIT_FAILURE;
	}
	
#ifdef _MSC_VER
	SetConsoleCtrlHandler( (PHANDLER_ROUTINE) sighandler, TRUE );
#else
	signal(SIGINT, &sigint_callback_handler);
	signal(SIGILL, &sigint_callback_handler);
	signal(SIGFPE, &sigint_callback_handler);
	signal(SIGSEGV, &sigint_callback_handler);
	signal(SIGTERM, &sigint_callback_handler);
	signal(SIGABRT, &sigint_callback_handler);
#endif
	fprintf(stderr, "call hackrf_sample_rate_set(%.03f MHz)\n",
		   ((float)DEFAULT_SAMPLE_RATE_HZ/(float)FREQ_ONE_MHZ));
	result = hackrf_set_sample_rate_manual(device, DEFAULT_SAMPLE_RATE_HZ, 1);
	if( result != HACKRF_SUCCESS ) {
		fprintf(stderr, "hackrf_sample_rate_set() failed: %s (%d)\n",
			   hackrf_error_name(result), result);
		usage();
		return EXIT_FAILURE;
	}

	fprintf(stderr, "call hackrf_baseband_filter_bandwidth_set(%.03f MHz)\n",
			((float)DEFAULT_BASEBAND_FILTER_BANDWIDTH/(float)FREQ_ONE_MHZ));
	result = hackrf_set_baseband_filter_bandwidth(device, DEFAULT_BASEBAND_FILTER_BANDWIDTH);
	if( result != HACKRF_SUCCESS ) {
		fprintf(stderr, "hackrf_baseband_filter_bandwidth_set() failed: %s (%d)\n",
			   hackrf_error_name(result), result);
		usage();
		return EXIT_FAILURE;
	}

	result = hackrf_set_vga_gain(device, vga_gain);
	result |= hackrf_set_lna_gain(device, lna_gain);
	result |= hackrf_start_rx(device, rx_callback, NULL);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_start_?x() failed: %s (%d)\n", hackrf_error_name(result), result);
		usage();
		return EXIT_FAILURE;
	}

	/* DGS FIXME: allow upper and lower frequencies to be set */
	result = hackrf_init_sweep(device, freq_min, freq_max, 20);
	if( result != HACKRF_SUCCESS ) {
		fprintf(stderr, "hackrf_init_scan() failed: %s (%d)\n",
			   hackrf_error_name(result), result);
		usage();
		return EXIT_FAILURE;
	}

	if (amp) {
		fprintf(stderr, "call hackrf_set_amp_enable(%u)\n", amp_enable);
		result = hackrf_set_amp_enable(device, (uint8_t)amp_enable);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr, "hackrf_set_amp_enable() failed: %s (%d)\n",
				   hackrf_error_name(result), result);
			usage();
			return EXIT_FAILURE;
		}
	}

	if (antenna) {
		fprintf(stderr, "call hackrf_set_antenna_enable(%u)\n", antenna_enable);
		result = hackrf_set_antenna_enable(device, (uint8_t)antenna_enable);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr, "hackrf_set_antenna_enable() failed: %s (%d)\n",
				   hackrf_error_name(result), result);
			usage();
			return EXIT_FAILURE;
		}
	}

	gettimeofday(&t_start, NULL);
	gettimeofday(&time_start, NULL);

	fprintf(stderr, "Stop with Ctrl-C\n");
	while((hackrf_is_streaming(device) == HACKRF_TRUE) && (do_exit == false)) {
		uint32_t byte_count_now;
		struct timeval time_now;
		float time_difference, rate;
		sleep(1);
		
		gettimeofday(&time_now, NULL);
		
		byte_count_now = byte_count;
		byte_count = 0;
		
		time_difference = TimevalDiff(&time_now, &time_start);
		rate = (float)byte_count_now / time_difference;
		fprintf(stderr, "%4.1f MiB / %5.3f sec = %4.1f MiB/second\n",
				(byte_count_now / 1e6f), time_difference, (rate / 1e6f) );

		time_start = time_now;

		if (byte_count_now == 0) {
			exit_code = EXIT_FAILURE;
			fprintf(stderr, "\nCouldn't transfer any bytes for one second.\n");
			break;
		}
	}

	result = hackrf_is_streaming(device);	
	if (do_exit) {
		fprintf(stderr, "\nUser cancel, exiting...\n");
	} else {
		fprintf(stderr, "\nExiting... hackrf_is_streaming() result: %s (%d)\n",
			   hackrf_error_name(result), result);
	}

	gettimeofday(&t_end, NULL);
	time_diff = TimevalDiff(&t_end, &t_start);
	fprintf(stderr, "Total time: %5.5f s\n", time_diff);

	if(device != NULL) {
		result = hackrf_stop_rx(device);
		if(result != HACKRF_SUCCESS) {
			fprintf(stderr, "hackrf_stop_rx() failed: %s (%d)\n",
				   hackrf_error_name(result), result);
		} else {
			fprintf(stderr, "hackrf_stop_rx() done\n");
		}

		result = hackrf_close(device);
		if(result != HACKRF_SUCCESS) {
			fprintf(stderr, "hackrf_close() failed: %s (%d)\n",
				   hackrf_error_name(result), result);
		} else {
			fprintf(stderr, "hackrf_close() done\n");
		}

		hackrf_exit();
		fprintf(stderr, "hackrf_exit() done\n");
	}

	if(fd != NULL) {
		fclose(fd);
		fd = NULL;
		fprintf(stderr, "fclose(fd) done\n");
	}
	fprintf(stderr, "exit\n");
	return exit_code;
}
