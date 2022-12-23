/*
 * Copyright 2012-2022 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2012 Jared Boone <jared@sharebrained.com>
 * Copyright 2013-2014 Benjamin Vernoux <titanmkd@gmail.com>
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

#define _FILE_OFFSET_BITS 64

#include <hackrf.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>

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

int gettimeofday(struct timeval* tv, void* ignored)
{
	FILETIME ft;
	unsigned __int64 tmp = 0;
	if (NULL != tv) {
		GetSystemTimeAsFileTime(&ft);
		tmp |= ft.dwHighDateTime;
		tmp <<= 32;
		tmp |= ft.dwLowDateTime;
		tmp /= 10;
		tmp -= 11644473600000000Ui64;
		tv->tv_sec = (long) (tmp / 1000000UL);
		tv->tv_usec = (long) (tmp % 1000000UL);
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

#define FD_BUFFER_SIZE (8 * 1024)

#define FREQ_ONE_MHZ (1000000ll)

#define DEFAULT_FREQ_HZ (900000000ll)  /* 900MHz */
#define FREQ_ABS_MIN_HZ (0ull)         /* 0 Hz */
#define FREQ_MIN_HZ     (1000000ll)    /* 1MHz */
#define FREQ_MAX_HZ     (6000000000ll) /* 6000MHz */
#define FREQ_ABS_MAX_HZ (7250000000ll) /* 7250MHz */
#define IF_ABS_MIN_HZ   (2000000000ll)
#define IF_MIN_HZ       (2170000000ll)
#define IF_MAX_HZ       (2740000000ll)
#define IF_ABS_MAX_HZ   (3000000000ll)
#define LO_MIN_HZ       (84375000ll)
#define LO_MAX_HZ       (5400000000ll)
#define DEFAULT_LO_HZ   (1000000000ll)

#define SAMPLE_RATE_MIN_HZ     (2000000)  /* 2MHz min sample rate */
#define SAMPLE_RATE_MAX_HZ     (20000000) /* 20MHz max sample rate */
#define DEFAULT_SAMPLE_RATE_HZ (10000000) /* 10MHz default sample rate */

#define DEFAULT_BASEBAND_FILTER_BANDWIDTH (5000000) /* 5MHz default */

#define SAMPLES_TO_XFER_MAX (0x8000000000000000ull) /* Max value */

#define BASEBAND_FILTER_BW_MIN (1750000)  /* 1.75 MHz min value */
#define BASEBAND_FILTER_BW_MAX (28000000) /* 28 MHz max value */

typedef enum {
	TRANSCEIVER_MODE_OFF = 0,
	TRANSCEIVER_MODE_RX = 1,
	TRANSCEIVER_MODE_TX = 2,
	TRANSCEIVER_MODE_SS = 3,
} transceiver_mode_t;

typedef enum {
	HW_SYNC_MODE_OFF = 0,
	HW_SYNC_MODE_ON = 1,
} hw_sync_mode_t;

typedef struct {
	uint64_t m0_total;
	uint64_t m4_total;
} stats_t;

/* WAVE or RIFF WAVE file format containing IQ 2x8bits data for HackRF compatible with SDR# Wav IQ file */
typedef struct {
	char groupID[4];  /* 'RIFF' */
	uint32_t size;    /* File size + 8bytes */
	char riffType[4]; /* 'WAVE'*/
} t_WAVRIFF_hdr;

#define FormatID \
	"fmt " /* chunkID for Format Chunk. NOTE: There is a space at the end of this ID. */

typedef struct {
	char chunkID[4];           /* 'fmt ' */
	uint32_t chunkSize;        /* 16 fixed */
	uint16_t wFormatTag;       /* 1 fixed */
	uint16_t wChannels;        /* 2 fixed */
	uint32_t dwSamplesPerSec;  /* Freq Hz sampling */
	uint32_t dwAvgBytesPerSec; /* Freq Hz sampling x 2 */
	uint16_t wBlockAlign;      /* 2 fixed */
	uint16_t wBitsPerSample;   /* 8 fixed */
} t_FormatChunk;

typedef struct {
	char chunkID[4];    /* 'data' */
	uint32_t chunkSize; /* Size of data in bytes */

	/* Samples I(8bits) then Q(8bits), I, Q ... */
} t_DataChunk;

typedef struct {
	t_WAVRIFF_hdr hdr;
	t_FormatChunk fmt_chunk;
	t_DataChunk data_chunk;
} t_wav_file_hdr;

t_wav_file_hdr wave_file_hdr = {
	/* t_WAVRIFF_hdr */
	{/* groupID */
	 {'R', 'I', 'F', 'F'},
	 0, /* size to update later */
	 {'W', 'A', 'V', 'E'}},
	/* t_FormatChunk */
	{
		/* char chunkID[4]; */
		{'f', 'm', 't', ' '},
		16, /* uint32_t chunkSize; */
		1,  /* uint16_t wFormatTag; 1 fixed */
		2,  /* uint16_t wChannels; 2 fixed */
		0,  /* uint32_t dwSamplesPerSec; Freq Hz sampling to update later */
		0,  /* uint32_t dwAvgBytesPerSec; Freq Hz sampling x 2 to update later */
		2,  /* uint16_t wBlockAlign; 2 fixed */
		8,  /* uint16_t wBitsPerSample; 8 fixed */
	},
	/* t_DataChunk */
	{
		/* char chunkID[4]; */
		{'d', 'a', 't', 'a'},
		0, /* uint32_t chunkSize; to update later */
	}};

static transceiver_mode_t transceiver_mode = TRANSCEIVER_MODE_RX;

#define U64TOA_MAX_DIGIT (31)

typedef struct {
	char data[U64TOA_MAX_DIGIT + 1];
} t_u64toa;

t_u64toa ascii_u64_data[4];

static float TimevalDiff(const struct timeval* a, const struct timeval* b)
{
	return (a->tv_sec - b->tv_sec) + 1e-6f * (a->tv_usec - b->tv_usec);
}

int parse_u64(char* s, uint64_t* const value)
{
	uint_fast8_t base = 10;
	char* s_end;
	uint64_t u64_value;

	if (strlen(s) > 2) {
		if (s[0] == '0') {
			if ((s[1] == 'x') || (s[1] == 'X')) {
				base = 16;
				s += 2;
			} else if ((s[1] == 'b') || (s[1] == 'B')) {
				base = 2;
				s += 2;
			}
		}
	}

	s_end = s;
	u64_value = strtoull(s, &s_end, base);
	if ((s != s_end) && (*s_end == 0)) {
		*value = u64_value;
		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_INVALID_PARAM;
	}
}

int parse_u32(char* s, uint32_t* const value)
{
	uint_fast8_t base = 10;
	char* s_end;
	uint64_t ulong_value;

	if (strlen(s) > 2) {
		if (s[0] == '0') {
			if ((s[1] == 'x') || (s[1] == 'X')) {
				base = 16;
				s += 2;
			} else if ((s[1] == 'b') || (s[1] == 'B')) {
				base = 2;
				s += 2;
			}
		}
	}

	s_end = s;
	ulong_value = strtoul(s, &s_end, base);
	if ((s != s_end) && (*s_end == 0)) {
		*value = (uint32_t) ulong_value;
		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_INVALID_PARAM;
	}
}

/* Parse frequencies as doubles to take advantage of notation parsing */
int parse_frequency_i64(char* optarg, char* endptr, int64_t* value)
{
	*value = (int64_t) strtod(optarg, &endptr);
	if (optarg == endptr) {
		return HACKRF_ERROR_INVALID_PARAM;
	}
	return HACKRF_SUCCESS;
}

int parse_frequency_u32(char* optarg, char* endptr, uint32_t* value)
{
	*value = (uint32_t) strtod(optarg, &endptr);
	if (optarg == endptr) {
		return HACKRF_ERROR_INVALID_PARAM;
	}
	return HACKRF_SUCCESS;
}

static char* stringrev(char* str)
{
	char *p1, *p2;

	if (!str || !*str)
		return str;

	for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2) {
		*p1 ^= *p2;
		*p2 ^= *p1;
		*p1 ^= *p2;
	}
	return str;
}

char* u64toa(uint64_t val, t_u64toa* str)
{
#define BASE (10ull) /* Base10 by default */
	uint64_t sum;
	int pos;
	int digit;
	int max_len;
	char* res;

	sum = val;
	max_len = U64TOA_MAX_DIGIT;
	pos = 0;

	do {
		digit = (sum % BASE);
		str->data[pos] = digit + '0';
		pos++;

		sum /= BASE;
	} while ((sum > 0) && (pos < max_len));

	if ((pos == max_len) && (sum > 0))
		return NULL;

	str->data[pos] = '\0';
	res = stringrev(str->data);

	return res;
}

static volatile bool do_exit = false;
static volatile bool interrupted = false;
static volatile bool tx_complete = false;
static volatile bool flush_complete = false;
#ifdef _WIN32
static HANDLE interrupt_handle;
#endif

FILE* file = NULL;
volatile uint32_t byte_count = 0;

bool signalsource = false;
uint32_t amplitude = 0;

bool hw_sync = false;

bool receive = false;
bool receive_wav = false;
uint64_t stream_size = 0;
uint32_t stream_head = 0;
uint32_t stream_tail = 0;
uint32_t stream_drop = 0;
uint8_t* stream_buf = NULL;

/* sum of power of all samples, reset on the periodic report */
volatile uint64_t stream_power = 0;

bool transmit = false;
struct timeval time_start;
struct timeval t_start;

bool automatic_tuning = false;
int64_t freq_hz;

bool if_freq = false;
int64_t if_freq_hz;

bool lo_freq = false;
int64_t lo_freq_hz = DEFAULT_LO_HZ;

bool image_reject = false;
uint32_t image_reject_selection;

bool amp = false;
uint32_t amp_enable;

bool antenna = false;
uint32_t antenna_enable;

bool sample_rate = false;
uint32_t sample_rate_hz;

bool force_ranges = false;

bool limit_num_samples = false;
uint64_t samples_to_xfer = 0;
size_t bytes_to_xfer = 0;

bool display_stats = false;

bool baseband_filter_bw = false;
uint32_t baseband_filter_bw_hz = 0;

bool repeat = false;

bool crystal_correct = false;
uint32_t crystal_correct_ppm;

int requested_mode_count = 0;

void stop_main_loop(void)
{
	do_exit = true;
#ifdef _WIN32
	SetEvent(interrupt_handle);
#else
	kill(getpid(), SIGALRM);
#endif
}

int rx_callback(hackrf_transfer* transfer)
{
	size_t bytes_to_write;
	size_t bytes_written;
	unsigned int i;

	if (file == NULL) {
		stop_main_loop();
		return -1;
	}

	/* Accumulate power (magnitude squared). */
	bytes_to_write = transfer->valid_length;
	uint64_t sum = 0;
	for (i = 0; i < bytes_to_write; i++) {
		int8_t value = transfer->buffer[i];
		sum += value * value;
	}

	/* Update both running totals at approximately the same time. */
	byte_count += transfer->valid_length;
	stream_power += sum;

	if (limit_num_samples) {
		if (bytes_to_write >= bytes_to_xfer) {
			bytes_to_write = bytes_to_xfer;
		}
		bytes_to_xfer -= bytes_to_write;
	}

	if (receive_wav) {
		/* convert .wav contents from signed to unsigned */
		for (i = 0; i < bytes_to_write; i++) {
			transfer->buffer[i] ^= (uint8_t) 0x80;
		}
	}

	if (stream_size == 0) {
		bytes_written = fwrite(transfer->buffer, 1, bytes_to_write, file);
		if ((bytes_written != bytes_to_write) ||
		    (limit_num_samples && (bytes_to_xfer == 0))) {
			stop_main_loop();
			return -1;
		} else {
			return 0;
		}
	}

#ifndef _WIN32
	if ((stream_size - 1 + stream_head - stream_tail) % stream_size <
	    bytes_to_write) {
		stream_drop++;
	} else {
		if (stream_tail + bytes_to_write <= stream_size) {
			memcpy(stream_buf + stream_tail,
			       transfer->buffer,
			       bytes_to_write);
		} else {
			memcpy(stream_buf + stream_tail,
			       transfer->buffer,
			       (stream_size - stream_tail));
			memcpy(stream_buf,
			       transfer->buffer + (stream_size - stream_tail),
			       bytes_to_write - (stream_size - stream_tail));
		};
		__atomic_store_n(
			&stream_tail,
			(stream_tail + bytes_to_write) % stream_size,
			__ATOMIC_RELEASE);
	}
#endif
	return 0;
}

int tx_callback(hackrf_transfer* transfer)
{
	size_t bytes_to_read;
	size_t bytes_read;
	unsigned int i;

	/* Check we have a valid source of samples. */
	if (file == NULL && transceiver_mode != TRANSCEIVER_MODE_SS) {
		stop_main_loop();
		return -1;
	}

	/* If the last data was already buffered, stop. */
	if (tx_complete) {
		return -1;
	}

	/* Determine how many bytes we need to put in the buffer. */
	bytes_to_read = transfer->buffer_length;
	if (limit_num_samples) {
		if (bytes_to_read >= bytes_to_xfer) {
			bytes_to_read = bytes_to_xfer;
		}
		bytes_to_xfer -= bytes_to_read;
	}

	/* Fill the buffer. */
	if (file == NULL) {
		/* Transmit continuous wave with specific amplitude */
		for (i = 0; i < bytes_to_read; i += 2) {
			transfer->buffer[i] = amplitude;
			transfer->buffer[i + 1] = 0;
		}
		bytes_read = bytes_to_read;
	} else {
		/* Read samples from file. */
		bytes_read = fread(transfer->buffer, 1, bytes_to_read, file);

		/* If no more bytes, error or file empty, terminate. */
		if (bytes_read == 0) {
			/* Report any error. */
			if (ferror(file)) {
				fprintf(stderr, "Could not read input file.\n");
				stop_main_loop();
				return -1;
			}
			if (ftell(file) < 1) {
				stop_main_loop();
				return -1;
			}
		}
	}

	/* Now set the valid length to the bytes we put in the buffer. */
	transfer->valid_length = bytes_read;

	/* If the sample limit has been reached, this is the last data. */
	if (limit_num_samples && (bytes_to_xfer == 0)) {
		tx_complete = true;
		return 0;
	}

	/* If we filled the number of bytes needed, return normally. */
	if (bytes_read == bytes_to_read) {
		return 0;
	}

	/* Otherwise, the file ran short. If not repeating, this is the last data. */
	if ((!repeat) || (ftell(file) < 1)) {
		tx_complete = true;
		return 0;
	}

	/* If we get to here, we need to repeat the file until we fill the buffer. */
	while (bytes_read < bytes_to_read) {
		size_t extra_bytes_read;

		/* Rewind and read more samples. */
		rewind(file);
		extra_bytes_read =
			fread(transfer->buffer + bytes_read,
			      1,
			      bytes_to_read - bytes_read,
			      file);

		/* If no more bytes, error or file empty, use what we have. */
		if (extra_bytes_read == 0) {
			/* Report any error. */
			if (ferror(file)) {
				fprintf(stderr, "Could not read input file.\n");
				tx_complete = true;
				return 0;
			}
			if (ftell(file) < 1) {
				tx_complete = true;
				return 0;
			}
		}

		bytes_read += extra_bytes_read;
		transfer->valid_length += extra_bytes_read;
	}

	/* Then return normally. */
	return 0;
}

static void tx_complete_callback(hackrf_transfer* transfer, int success)
{
	// If a transfer failed to complete, stop the main loop.
	if (!success) {
		stop_main_loop();
		return;
	}

	/* Accumulate power (magnitude squared). */
	uint32_t i;
	uint64_t sum = 0;
	for (i = 0; i < transfer->valid_length; i++) {
		int8_t value = transfer->buffer[i];
		sum += value * value;
	}

	/* Update both running totals at approximately the same time. */
	byte_count += transfer->valid_length;
	stream_power += sum;
}

static void flush_callback(void* flush_ctx, int success)
{
	if (success) {
		flush_complete = true;
	}
	stop_main_loop();
}

static int update_stats(hackrf_device* device, hackrf_m0_state* state, stats_t* stats)
{
	int result = hackrf_get_m0_state(device, state);

	if (result == HACKRF_SUCCESS) {
		/*
		 * Update 64-bit running totals, to handle wrapping of the 32-bit fields
		 * for M0 and M4 byte counts.
		 *
		 * The logic for handling wrapping works as follows:
		 *
		 * If a 32-bit count read from the HackRF is less than the lower 32 bits of
		 * the previous 64-bit running total, this indicates the 32-bit counter has
		 * wrapped since it was last read. Add 2^32 to the 64-bit total to account
		 * for this.
		 *
		 * Then, having accounted for the possible wrap, mask off the bottom 32
		 * bits of the 64-bit total, and replace them with the new 32-bit count.
		 *
		 * This should result in correct results as long as the 32-bit counter
		 * cannot wrap more than once between reads.
		 *
		 * We read the M0 state every second, and the counters will wrap every 107
		 * seconds at 20Msps, so this should be a safe assumption.
		 */
		if (state->m0_count < (stats->m0_total & 0xFFFFFFFF))
			stats->m0_total += 0x100000000;
		if (state->m4_count < (stats->m4_total & 0xFFFFFFFF))
			stats->m4_total += 0x100000000;
		stats->m0_total =
			(stats->m0_total & 0xFFFFFFFF00000000) | state->m0_count;
		stats->m4_total =
			(stats->m4_total & 0xFFFFFFFF00000000) | state->m4_count;
	}

	return result;
}

static void usage()
{
	printf("Usage:\n");
	printf("\t-h # this help\n");
	printf("\t[-d serial_number] # Serial number of desired HackRF.\n");
	printf("\t-r <filename> # Receive data into file (use '-' for stdout).\n");
	printf("\t-t <filename> # Transmit data from file (use '-' for stdin).\n");
	printf("\t-w # Receive data into file with WAV header and automatic name.\n");
	printf("\t   # This is for SDR# compatibility and may not work with other software.\n");
	printf("\t[-f freq_hz] # Frequency in Hz [%sMHz to %sMHz supported, %sMHz to %sMHz forceable].\n",
	       u64toa((FREQ_MIN_HZ / FREQ_ONE_MHZ), &ascii_u64_data[0]),
	       u64toa((FREQ_MAX_HZ / FREQ_ONE_MHZ), &ascii_u64_data[1]),
	       u64toa((FREQ_ABS_MIN_HZ / FREQ_ONE_MHZ), &ascii_u64_data[2]),
	       u64toa((FREQ_ABS_MAX_HZ / FREQ_ONE_MHZ), &ascii_u64_data[3]));
	printf("\t[-i if_freq_hz] # Intermediate Frequency (IF) in Hz [%sMHz to %sMHz supported, %sMHz to %sMHz forceable].\n",
	       u64toa((IF_MIN_HZ / FREQ_ONE_MHZ), &ascii_u64_data[0]),
	       u64toa((IF_MAX_HZ / FREQ_ONE_MHZ), &ascii_u64_data[1]),
	       u64toa((IF_ABS_MIN_HZ / FREQ_ONE_MHZ), &ascii_u64_data[2]),
	       u64toa((IF_ABS_MAX_HZ / FREQ_ONE_MHZ), &ascii_u64_data[3]));
	printf("\t[-o lo_freq_hz] # Front-end Local Oscillator (LO) frequency in Hz [%sMHz to %sMHz].\n",
	       u64toa((LO_MIN_HZ / FREQ_ONE_MHZ), &ascii_u64_data[0]),
	       u64toa((LO_MAX_HZ / FREQ_ONE_MHZ), &ascii_u64_data[1]));
	printf("\t[-m image_reject] # Image rejection filter selection, 0=bypass, 1=low pass, 2=high pass.\n");
	printf("\t[-a amp_enable] # RX/TX RF amplifier 1=Enable, 0=Disable.\n");
	printf("\t[-p antenna_enable] # Antenna port power, 1=Enable, 0=Disable.\n");
	printf("\t[-l gain_db] # RX LNA (IF) gain, 0-40dB, 8dB steps\n");
	printf("\t[-g gain_db] # RX VGA (baseband) gain, 0-62dB, 2dB steps\n");
	printf("\t[-x gain_db] # TX VGA (IF) gain, 0-47dB, 1dB steps\n");
	printf("\t[-s sample_rate_hz] # Sample rate in Hz (%s-%sMHz supported, default %sMHz).\n",
	       u64toa((SAMPLE_RATE_MIN_HZ / FREQ_ONE_MHZ), &ascii_u64_data[0]),
	       u64toa((SAMPLE_RATE_MAX_HZ / FREQ_ONE_MHZ), &ascii_u64_data[1]),
	       u64toa((DEFAULT_SAMPLE_RATE_HZ / FREQ_ONE_MHZ), &ascii_u64_data[2]));
	printf("\t[-F force] # Force use of parameters outside supported ranges.\n");
	printf("\t[-n num_samples] # Number of samples to transfer (default is unlimited).\n");
#ifndef _WIN32
	/* The required atomic load/store functions aren't available when using C with MSVC */
	printf("\t[-S buf_size] # Enable receive streaming with buffer size buf_size.\n");
#endif
	printf("\t[-B] # Print buffer statistics during transfer\n");
	printf("\t[-c amplitude] # CW signal source mode, amplitude 0-127 (DC value to DAC).\n");
	printf("\t[-R] # Repeat TX mode (default is off) \n");
	printf("\t[-b baseband_filter_bw_hz] # Set baseband filter bandwidth in Hz.\n");
	printf("\tPossible values: 1.75/2.5/3.5/5/5.5/6/7/8/9/10/12/14/15/20/24/28MHz, default <= 0.75 * sample_rate_hz.\n");
	printf("\t[-C ppm] # Set Internal crystal clock error in ppm.\n");
	printf("\t[-H] # Synchronize RX/TX to external trigger input.\n");
}

static hackrf_device* device = NULL;

#ifdef _WIN32
BOOL WINAPI sighandler(int signum)
{
	if (CTRL_C_EVENT == signum) {
		interrupted = true;
		fprintf(stderr, "Caught signal %d\n", signum);
		stop_main_loop();
		return TRUE;
	}
	return FALSE;
}
#else
void sigint_callback_handler(int signum)
{
	interrupted = true;
	fprintf(stderr, "Caught signal %d\n", signum);
	do_exit = true;
}
#endif

#ifndef _WIN32
void sigalrm_callback_handler(int signum)
{
}
#endif

#define PATH_FILE_MAX_LEN (FILENAME_MAX)
#define DATE_TIME_MAX_LEN (32)

int main(int argc, char** argv)
{
	int opt;
	char path_file[PATH_FILE_MAX_LEN];
	char date_time[DATE_TIME_MAX_LEN];
	const char* path = NULL;
	const char* serial_number = NULL;
	char* endptr = NULL;
	int result;
	time_t rawtime;
	struct tm* timeinfo;
	long int file_pos;
	int exit_code = EXIT_SUCCESS;
	struct timeval t_end;
	float time_diff;
	unsigned int lna_gain = 8, vga_gain = 20, txvga_gain = 0;
	hackrf_m0_state state;
	stats_t stats = {0, 0};

	while ((opt = getopt(argc, argv, "Hwr:t:f:i:o:m:a:p:s:Fn:b:l:g:x:c:d:C:RS:Bh?")) !=
	       EOF) {
		result = HACKRF_SUCCESS;
		switch (opt) {
		case 'H':
			hw_sync = true;
			break;
		case 'w':
			receive_wav = true;
			requested_mode_count++;
			break;

		case 'r':
			receive = true;
			requested_mode_count++;
			path = optarg;
			break;

		case 't':
			transmit = true;
			requested_mode_count++;
			path = optarg;
			break;

		case 'd':
			serial_number = optarg;
			break;

		case 'S':
			result = parse_u64(optarg, &stream_size);
			stream_buf = calloc(1, stream_size);
			break;

		case 'f':
			result = parse_frequency_i64(optarg, endptr, &freq_hz);
			automatic_tuning = true;
			break;

		case 'i':
			result = parse_frequency_i64(optarg, endptr, &if_freq_hz);
			if_freq = true;
			break;

		case 'o':
			result = parse_frequency_i64(optarg, endptr, &lo_freq_hz);
			lo_freq = true;
			break;

		case 'm':
			image_reject = true;
			result = parse_u32(optarg, &image_reject_selection);
			break;

		case 'a':
			amp = true;
			result = parse_u32(optarg, &amp_enable);
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

		case 's':
			result = parse_frequency_u32(optarg, endptr, &sample_rate_hz);
			sample_rate = true;
			break;

		case 'F':
			force_ranges = true;
			break;

		case 'n':
			limit_num_samples = true;
			result = parse_u64(optarg, &samples_to_xfer);
			bytes_to_xfer = samples_to_xfer * 2ull;
			break;

		case 'B':
			display_stats = true;
			break;

		case 'b':
			result = parse_frequency_u32(
				optarg,
				endptr,
				&baseband_filter_bw_hz);
			baseband_filter_bw = true;
			break;

		case 'c':
			signalsource = true;
			requested_mode_count++;
			result = parse_u32(optarg, &amplitude);
			break;

		case 'R':
			repeat = true;
			break;

		case 'C':
			crystal_correct = true;
			result = parse_u32(optarg, &crystal_correct_ppm);
			break;

		case 'h':
		case '?':
			usage();
			return EXIT_SUCCESS;

		default:
			fprintf(stderr, "unknown argument '-%c %s'\n", opt, optarg);
			usage();
			return EXIT_FAILURE;
		}

		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"argument error: '-%c %s' %s (%d)\n",
				opt,
				optarg,
				hackrf_error_name(result),
				result);
			usage();
			return EXIT_FAILURE;
		}
	}

	if (lna_gain % 8)
		fprintf(stderr, "warning: lna_gain (-l) must be a multiple of 8\n");

	if (vga_gain % 2)
		fprintf(stderr, "warning: vga_gain (-g) must be a multiple of 2\n");

	if (samples_to_xfer >= SAMPLES_TO_XFER_MAX) {
		fprintf(stderr,
			"argument error: num_samples must be less than %s/%sMio\n",
			u64toa(SAMPLES_TO_XFER_MAX, &ascii_u64_data[0]),
			u64toa((SAMPLES_TO_XFER_MAX / FREQ_ONE_MHZ), &ascii_u64_data[1]));
		usage();
		return EXIT_FAILURE;
	}

	if (if_freq || lo_freq || image_reject) {
		/* explicit tuning selected */
		if (!if_freq) {
			fprintf(stderr,
				"argument error: if_freq_hz must be specified for explicit tuning.\n");
			usage();
			return EXIT_FAILURE;
		}
		if (!image_reject) {
			fprintf(stderr,
				"argument error: image_reject must be specified for explicit tuning.\n");
			usage();
			return EXIT_FAILURE;
		}
		if (!lo_freq && (image_reject_selection != RF_PATH_FILTER_BYPASS)) {
			fprintf(stderr,
				"argument error: lo_freq_hz must be specified for explicit tuning unless image_reject is set to bypass.\n");
			usage();
			return EXIT_FAILURE;
		}
		if (((if_freq_hz > IF_MAX_HZ) || (if_freq_hz < IF_MIN_HZ)) &&
		    !force_ranges) {
			fprintf(stderr,
				"argument error: if_freq_hz should be between %s and %s.\n",
				u64toa(IF_MIN_HZ, &ascii_u64_data[0]),
				u64toa(IF_MAX_HZ, &ascii_u64_data[1]));
			usage();
			return EXIT_FAILURE;
		}
		if ((if_freq_hz > IF_ABS_MAX_HZ) || (if_freq_hz < IF_ABS_MIN_HZ)) {
			fprintf(stderr,
				"argument error: if_freq_hz must be between %s and %s.\n",
				u64toa(IF_ABS_MIN_HZ, &ascii_u64_data[0]),
				u64toa(IF_ABS_MAX_HZ, &ascii_u64_data[1]));
			usage();
			return EXIT_FAILURE;
		}
		if ((lo_freq_hz > LO_MAX_HZ) || (lo_freq_hz < LO_MIN_HZ)) {
			fprintf(stderr,
				"argument error: lo_freq_hz shall be between %s and %s.\n",
				u64toa(LO_MIN_HZ, &ascii_u64_data[0]),
				u64toa(LO_MAX_HZ, &ascii_u64_data[1]));
			usage();
			return EXIT_FAILURE;
		}
		if (image_reject_selection > 2) {
			fprintf(stderr,
				"argument error: image_reject must be 0, 1, or 2 .\n");
			usage();
			return EXIT_FAILURE;
		}
		if (automatic_tuning) {
			fprintf(stderr,
				"warning: freq_hz ignored by explicit tuning selection.\n");
			automatic_tuning = false;
		}
		switch (image_reject_selection) {
		case RF_PATH_FILTER_BYPASS:
			freq_hz = if_freq_hz;
			break;
		case RF_PATH_FILTER_LOW_PASS:
			freq_hz = (int64_t) labs((long int) (if_freq_hz - lo_freq_hz));
			break;
		case RF_PATH_FILTER_HIGH_PASS:
			freq_hz = if_freq_hz + lo_freq_hz;
			break;
		default:
			freq_hz = DEFAULT_FREQ_HZ;
			break;
		}
		fprintf(stderr,
			"explicit tuning specified for %s Hz.\n",
			u64toa(freq_hz, &ascii_u64_data[0]));

	} else if (automatic_tuning) {
		if (((freq_hz > FREQ_MAX_HZ) || (freq_hz < FREQ_MIN_HZ)) &&
		    !force_ranges) {
			fprintf(stderr,
				"argument error: freq_hz should be between %s and %s.\n",
				u64toa(FREQ_MIN_HZ, &ascii_u64_data[0]),
				u64toa(FREQ_MAX_HZ, &ascii_u64_data[1]));
			usage();
			return EXIT_FAILURE;
		}
		if (freq_hz > FREQ_ABS_MAX_HZ) {
			fprintf(stderr,
				"argument error: freq_hz must be between %s and %s.\n",
				u64toa(FREQ_ABS_MIN_HZ, &ascii_u64_data[0]),
				u64toa(FREQ_ABS_MAX_HZ, &ascii_u64_data[1]));
			usage();
			return EXIT_FAILURE;
		}
	} else {
		/* Use default freq */
		freq_hz = DEFAULT_FREQ_HZ;
		automatic_tuning = true;
	}

	if (amp) {
		if (amp_enable > 1) {
			fprintf(stderr, "argument error: amp_enable shall be 0 or 1.\n");
			usage();
			return EXIT_FAILURE;
		}
	}

	if (antenna) {
		if (antenna_enable > 1) {
			fprintf(stderr,
				"argument error: antenna_enable shall be 0 or 1.\n");
			usage();
			return EXIT_FAILURE;
		}
	}

	if (sample_rate) {
		if (sample_rate_hz > SAMPLE_RATE_MAX_HZ && !force_ranges) {
			fprintf(stderr,
				"argument error: sample_rate_hz should be less than or equal to %u Hz/%.03f MHz\n",
				SAMPLE_RATE_MAX_HZ,
				(float) (SAMPLE_RATE_MAX_HZ / FREQ_ONE_MHZ));
			usage();
			return EXIT_FAILURE;
		}
		if (sample_rate_hz < SAMPLE_RATE_MIN_HZ && !force_ranges) {
			fprintf(stderr,
				"argument error: sample_rate_hz should be greater than or equal to %u Hz/%.03f MHz\n",
				SAMPLE_RATE_MIN_HZ,
				(float) (SAMPLE_RATE_MIN_HZ / FREQ_ONE_MHZ));
			usage();
			return EXIT_FAILURE;
		}
	} else {
		sample_rate_hz = DEFAULT_SAMPLE_RATE_HZ;
	}

	if (baseband_filter_bw) {
		if (baseband_filter_bw_hz > BASEBAND_FILTER_BW_MAX) {
			fprintf(stderr,
				"argument error: baseband_filter_bw_hz must be less or equal to %u Hz/%.03f MHz\n",
				BASEBAND_FILTER_BW_MAX,
				(float) (BASEBAND_FILTER_BW_MAX / FREQ_ONE_MHZ));
			usage();
			return EXIT_FAILURE;
		}

		if (baseband_filter_bw_hz < BASEBAND_FILTER_BW_MIN) {
			fprintf(stderr,
				"argument error: baseband_filter_bw_hz must be greater or equal to %u Hz/%.03f MHz\n",
				BASEBAND_FILTER_BW_MIN,
				(float) (BASEBAND_FILTER_BW_MIN / FREQ_ONE_MHZ));
			usage();
			return EXIT_FAILURE;
		}

		/* Compute nearest freq for bw filter */
		baseband_filter_bw_hz =
			hackrf_compute_baseband_filter_bw(baseband_filter_bw_hz);
	}

	if (requested_mode_count > 1) {
		fprintf(stderr, "specify only one of: -t, -c, -r, -w\n");
		usage();
		return EXIT_FAILURE;
	}

	if (requested_mode_count < 1) {
		fprintf(stderr, "specify one of: -t, -c, -r, -w\n");
		usage();
		return EXIT_FAILURE;
	}

	if (receive) {
		transceiver_mode = TRANSCEIVER_MODE_RX;
	}

	if (transmit) {
		transceiver_mode = TRANSCEIVER_MODE_TX;
	}

	if (signalsource) {
		transceiver_mode = TRANSCEIVER_MODE_SS;
		if (amplitude > 127) {
			fprintf(stderr,
				"argument error: amplitude must be between 0 and 127.\n");
			usage();
			return EXIT_FAILURE;
		}
	}

	if (receive_wav) {
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		transceiver_mode = TRANSCEIVER_MODE_RX;
		/* File format HackRF Year(2013), Month(11), Day(28), Hour Min Sec+Z, Freq kHz, IQ.wav */
		strftime(date_time, DATE_TIME_MAX_LEN, "%Y%m%d_%H%M%S", timeinfo);
		snprintf(
			path_file,
			PATH_FILE_MAX_LEN,
			"HackRF_%sZ_%ukHz_IQ.wav",
			date_time,
			(uint32_t) (freq_hz / (1000ull)));
		path = path_file;
		fprintf(stderr, "Receive wav file: %s\n", path);
	}

	// In signal source mode, the PATH argument is neglected.
	if (transceiver_mode != TRANSCEIVER_MODE_SS) {
		if (path == NULL) {
			fprintf(stderr, "specify a path to a file to transmit/receive\n");
			usage();
			return EXIT_FAILURE;
		}
	}

	// Change the freq and sample rate to correct the crystal clock error.
	if (crystal_correct) {
		sample_rate_hz =
			(uint32_t) ((double) sample_rate_hz * (1000000 - crystal_correct_ppm) / 1000000 + 0.5);
		freq_hz = freq_hz * (1000000 - crystal_correct_ppm) / 1000000;
	}

	result = hackrf_init();
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_init() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		usage();
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

	if (transceiver_mode != TRANSCEIVER_MODE_SS) {
		if (transceiver_mode == TRANSCEIVER_MODE_RX) {
			if (strcmp(path, "-") == 0) {
				file = stdout;
			} else {
				file = fopen(path, "wb");
			}
		} else {
			if (strcmp(path, "-") == 0) {
				file = stdin;
			} else {
				file = fopen(path, "rb");
			}
		}

		if (file == NULL) {
			fprintf(stderr, "Failed to open file: %s\n", path);
			return EXIT_FAILURE;
		}
		/* Change file buffer to have bigger one to store or read data on/to HDD */
		result = setvbuf(file, NULL, _IOFBF, FD_BUFFER_SIZE);
		if (result != 0) {
			fprintf(stderr, "setvbuf() failed: %d\n", result);
			usage();
			return EXIT_FAILURE;
		}
	}

	/* Write Wav header */
	if (receive_wav) {
		fwrite(&wave_file_hdr, 1, sizeof(t_wav_file_hdr), file);
	}

#ifdef _WIN32
	SetConsoleCtrlHandler((PHANDLER_ROUTINE) sighandler, TRUE);
#else
	signal(SIGINT, &sigint_callback_handler);
	signal(SIGILL, &sigint_callback_handler);
	signal(SIGFPE, &sigint_callback_handler);
	signal(SIGSEGV, &sigint_callback_handler);
	signal(SIGTERM, &sigint_callback_handler);
	signal(SIGABRT, &sigint_callback_handler);
#endif

#ifdef _WIN32
	interrupt_handle = CreateEvent(NULL, TRUE, FALSE, NULL);
#else
	signal(SIGALRM, &sigalrm_callback_handler);
#endif

	fprintf(stderr,
		"call hackrf_set_sample_rate(%u Hz/%.03f MHz)\n",
		sample_rate_hz,
		((float) sample_rate_hz / (float) FREQ_ONE_MHZ));
	result = hackrf_set_sample_rate(device, sample_rate_hz);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_set_sample_rate() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		usage();
		return EXIT_FAILURE;
	}

	if (baseband_filter_bw) {
		fprintf(stderr,
			"call hackrf_set_baseband_filter_bandwidth(%d Hz/%.03f MHz)\n",
			baseband_filter_bw_hz,
			((float) baseband_filter_bw_hz / (float) FREQ_ONE_MHZ));
		result = hackrf_set_baseband_filter_bandwidth(
			device,
			baseband_filter_bw_hz);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_set_baseband_filter_bandwidth() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			usage();
			return EXIT_FAILURE;
		}
	}

	fprintf(stderr, "call hackrf_set_hw_sync_mode(%d)\n", hw_sync ? 1 : 0);
	result = hackrf_set_hw_sync_mode(
		device,
		hw_sync ? HW_SYNC_MODE_ON : HW_SYNC_MODE_OFF);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_set_hw_sync_mode() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		return EXIT_FAILURE;
	}

	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_start_?x() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		usage();
		return EXIT_FAILURE;
	}

	if (automatic_tuning) {
		fprintf(stderr,
			"call hackrf_set_freq(%s Hz/%.03f MHz)\n",
			u64toa(freq_hz, &ascii_u64_data[0]),
			((double) freq_hz / (double) FREQ_ONE_MHZ));
		result = hackrf_set_freq(device, freq_hz);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_set_freq() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			usage();
			return EXIT_FAILURE;
		}
	} else {
		fprintf(stderr,
			"call hackrf_set_freq_explicit() with %s Hz IF, %s Hz LO, %s\n",
			u64toa(if_freq_hz, &ascii_u64_data[0]),
			u64toa(lo_freq_hz, &ascii_u64_data[1]),
			hackrf_filter_path_name(image_reject_selection));
		result = hackrf_set_freq_explicit(
			device,
			if_freq_hz,
			lo_freq_hz,
			image_reject_selection);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_set_freq_explicit() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			usage();
			return EXIT_FAILURE;
		}
	}

	if (amp) {
		fprintf(stderr, "call hackrf_set_amp_enable(%u)\n", amp_enable);
		result = hackrf_set_amp_enable(device, (uint8_t) amp_enable);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_set_amp_enable() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			usage();
			return EXIT_FAILURE;
		}
	}

	if (antenna) {
		fprintf(stderr, "call hackrf_set_antenna_enable(%u)\n", antenna_enable);
		result = hackrf_set_antenna_enable(device, (uint8_t) antenna_enable);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_set_antenna_enable() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			usage();
			return EXIT_FAILURE;
		}
	}

	if (transceiver_mode == TRANSCEIVER_MODE_RX) {
		result = hackrf_set_vga_gain(device, vga_gain);
		result |= hackrf_set_lna_gain(device, lna_gain);
		result |= hackrf_start_rx(device, rx_callback, NULL);
	} else {
		result = hackrf_set_txvga_gain(device, txvga_gain);
		result |= hackrf_enable_tx_flush(device, flush_callback, NULL);
		result |= hackrf_set_tx_block_complete_callback(
			device,
			tx_complete_callback);
		result |= hackrf_start_tx(device, tx_callback, NULL);
	}

	if (limit_num_samples) {
		fprintf(stderr,
			"samples_to_xfer %s/%sMio\n",
			u64toa(samples_to_xfer, &ascii_u64_data[0]),
			u64toa((samples_to_xfer / FREQ_ONE_MHZ), &ascii_u64_data[1]));
	}

	gettimeofday(&t_start, NULL);
	gettimeofday(&time_start, NULL);

	fprintf(stderr, "Stop with Ctrl-C\n");

	// Set up an interval timer which will fire once per second.
#ifdef _WIN32
	HANDLE timer_handle = CreateWaitableTimer(NULL, FALSE, NULL);
	LARGE_INTEGER due_time;
	due_time.QuadPart = -10000000LL;
	LONG period = 1000;
	SetWaitableTimer(timer_handle, &due_time, period, NULL, NULL, 0);
#else
	struct itimerval interval_timer = {
		.it_interval = {.tv_sec = 1, .tv_usec = 0},
		.it_value = {.tv_sec = 1, .tv_usec = 0}};
	setitimer(ITIMER_REAL, &interval_timer, NULL);
#endif
	while (!do_exit) {
		struct timeval time_now;
		float time_difference, rate;
		if (stream_size > 0) {
#ifndef _WIN32
			if (stream_head == stream_tail) {
				usleep(10000); // queue empty
			} else {
				ssize_t len;
				ssize_t bytes_written;
				uint32_t _st =
					__atomic_load_n(&stream_tail, __ATOMIC_ACQUIRE);
				if (stream_head < _st)
					len = _st - stream_head;
				else
					len = stream_size - stream_head;
				bytes_written =
					fwrite(stream_buf + stream_head, 1, len, file);
				if (len != bytes_written) {
					fprintf(stderr, "write failed");
					do_exit = true;
				};
				stream_head = (stream_head + len) % stream_size;
			}
			if (stream_drop > 0) {
				uint32_t drops = __atomic_exchange_n(
					&stream_drop,
					0,
					__ATOMIC_SEQ_CST);
				fprintf(stderr, "dropped frames: [%d]\n", drops);
			}
#endif
		} else {
			uint64_t byte_count_now;
			uint64_t stream_power_now;
#ifdef _WIN32
			// Wait for interval timer event, or interrupt event.
			HANDLE handles[] = {timer_handle, interrupt_handle};
			WaitForMultipleObjects(2, handles, FALSE, INFINITE);
#else
			// Wait for SIGALRM from interval timer, or another signal.
			pause();
#endif
			gettimeofday(&time_now, NULL);

			/* Read and reset both totals at approximately the same time. */
			byte_count_now = byte_count;
			stream_power_now = stream_power;
			byte_count = 0;
			stream_power = 0;

			time_difference = TimevalDiff(&time_now, &time_start);
			rate = (float) byte_count_now / time_difference;
			if ((byte_count_now == 0) && (hw_sync)) {
				fprintf(stderr, "Waiting for trigger...\n");
			} else if (!((byte_count_now == 0) && (flush_complete))) {
				double full_scale_ratio = (double) stream_power_now /
					(byte_count_now * 127 * 127);
				double dB_full_scale = 10 * log10(full_scale_ratio) + 3.0;
				fprintf(stderr,
					"%4.1f MiB / %5.3f sec = %4.1f MiB/second, average power %3.1f dBfs",
					(byte_count_now / 1e6f),
					time_difference,
					(rate / 1e6f),
					dB_full_scale);
				if (display_stats) {
					bool tx = transmit || signalsource;
					result = update_stats(device, &state, &stats);
					if (result != HACKRF_SUCCESS)
						fprintf(stderr,
							"\nhackrf_get_m0_state() failed: %s (%d)\n",
							hackrf_error_name(result),
							result);
					else
						fprintf(stderr,
							", %d bytes %s in buffer, %u %s, longest %u bytes\n",
							tx ? state.m4_count -
									state.m0_count :
							     state.m0_count -
									state.m4_count,
							tx ? "filled" : "free",
							state.num_shortfalls,
							tx ? "underruns" : "overruns",
							state.longest_shortfall);
				} else {
					fprintf(stderr, "\n");
				}
			}

			time_start = time_now;

			if ((byte_count_now == 0) && (!hw_sync) && (!flush_complete)) {
				exit_code = EXIT_FAILURE;
				fprintf(stderr,
					"\nCouldn't transfer any bytes for one second.\n");
				break;
			}
		}
	}

	// Stop interval timer.
#ifdef _WIN32
	CancelWaitableTimer(timer_handle);
	CloseHandle(timer_handle);
#else
	interval_timer.it_value.tv_sec = 0;
	setitimer(ITIMER_REAL, &interval_timer, NULL);
#endif
	result = hackrf_is_streaming(device);
	if (do_exit) {
		fprintf(stderr, "\nExiting...\n");
	} else {
		fprintf(stderr,
			"\nExiting... hackrf_is_streaming() result: %s (%d)\n",
			hackrf_error_name(result),
			result);
	}

	gettimeofday(&t_end, NULL);
	time_diff = TimevalDiff(&t_end, &t_start);
	fprintf(stderr, "Total time: %5.5f s\n", time_diff);

	if (device != NULL) {
		if (receive || receive_wav) {
			result = hackrf_stop_rx(device);
			if (result != HACKRF_SUCCESS) {
				fprintf(stderr,
					"hackrf_stop_rx() failed: %s (%d)\n",
					hackrf_error_name(result),
					result);
			} else {
				fprintf(stderr, "hackrf_stop_rx() done\n");
			}
		}

		if (transmit || signalsource) {
			result = hackrf_stop_tx(device);
			if (result != HACKRF_SUCCESS) {
				fprintf(stderr,
					"hackrf_stop_tx() failed: %s (%d)\n",
					hackrf_error_name(result),
					result);
			} else {
				fprintf(stderr, "hackrf_stop_tx() done\n");
			}
		}

		if (display_stats) {
			result = update_stats(device, &state, &stats);
			if (result != HACKRF_SUCCESS) {
				fprintf(stderr,
					"hackrf_get_m0_state() failed: %s (%d)\n",
					hackrf_error_name(result),
					result);
			} else {
				fprintf(stderr,
					"Transfer statistics:\n"
					"%" PRIu64 " bytes transferred by M0\n"
					"%" PRIu64 " bytes transferred by M4\n"
					"%u %s, longest %u bytes\n",
					stats.m0_total,
					stats.m4_total,
					state.num_shortfalls,
					(transmit || signalsource) ? "underruns" :
								     "overruns",
					state.longest_shortfall);
			}
		}

		result = hackrf_close(device);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_close() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
		} else {
			fprintf(stderr, "hackrf_close() done\n");
		}

		hackrf_exit();
		fprintf(stderr, "hackrf_exit() done\n");
	}

	if (file != NULL) {
		if (receive_wav) {
			/* Get size of file */
			file_pos = ftell(file);
			/* Update Wav Header */
			wave_file_hdr.hdr.size = file_pos - 8;
			wave_file_hdr.fmt_chunk.dwSamplesPerSec = sample_rate_hz;
			wave_file_hdr.fmt_chunk.dwAvgBytesPerSec =
				wave_file_hdr.fmt_chunk.dwSamplesPerSec * 2;
			wave_file_hdr.data_chunk.chunkSize =
				file_pos - sizeof(t_wav_file_hdr);
			/* Overwrite header with updated data */
			rewind(file);
			fwrite(&wave_file_hdr, 1, sizeof(t_wav_file_hdr), file);
		}
		if (file != stdin) {
			fflush(file);
		}
		if ((file != stdout) && (file != stdin)) {
			fclose(file);
			file = NULL;
			fprintf(stderr, "fclose() done\n");
		}
	}
	fprintf(stderr, "exit\n");
	return exit_code;
}
