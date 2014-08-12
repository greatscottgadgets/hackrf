/*
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

int gettimeofday(struct timeval *tv, void* ignored)
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

#define DEFAULT_FREQ_HZ (900000000ull) /* 900MHz */
#define FREQ_MIN_HZ	(0ull) /* 0 Hz */
#define FREQ_MAX_HZ	(7250000000ull) /* 7250MHz */
#define IF_MIN_HZ (2150000000ull)
#define IF_MAX_HZ (2750000000ull)
#define LO_MIN_HZ (84375000ull)
#define LO_MAX_HZ (5400000000ull)
#define DEFAULT_LO_HZ (1000000000ull)

#define DEFAULT_SAMPLE_RATE_HZ (10000000) /* 10MHz default sample rate */

#define DEFAULT_BASEBAND_FILTER_BANDWIDTH (5000000) /* 5MHz default */

#define SAMPLES_TO_XFER_MAX (0x8000000000000000ull) /* Max value */

#define BASEBAND_FILTER_BW_MIN (1750000)  /* 1.75 MHz min value */
#define BASEBAND_FILTER_BW_MAX (28000000) /* 28 MHz max value */

#if defined _WIN32
	#define sleep(a) Sleep( (a*1000) )
#endif

/* WAVE or RIFF WAVE file format containing IQ 2x8bits data for HackRF compatible with SDR# Wav IQ file */
typedef struct 
{
    char groupID[4]; /* 'RIFF' */
    uint32_t size; /* File size + 8bytes */
    char riffType[4]; /* 'WAVE'*/
} t_WAVRIFF_hdr;

#define FormatID "fmt "   /* chunkID for Format Chunk. NOTE: There is a space at the end of this ID. */

typedef struct {
  char		chunkID[4]; /* 'fmt ' */
  uint32_t	chunkSize; /* 16 fixed */

  uint16_t	wFormatTag; /* 1 fixed */
  uint16_t	wChannels;  /* 2 fixed */
  uint32_t	dwSamplesPerSec; /* Freq Hz sampling */
  uint32_t	dwAvgBytesPerSec; /* Freq Hz sampling x 2 */
  uint16_t	wBlockAlign; /* 2 fixed */
  uint16_t	wBitsPerSample; /* 8 fixed */
} t_FormatChunk;

typedef struct 
{
    char		chunkID[4]; /* 'data' */
    uint32_t	chunkSize; /* Size of data in bytes */
	/* Samples I(8bits) then Q(8bits), I, Q ... */
} t_DataChunk;

typedef struct
{
	t_WAVRIFF_hdr hdr;
	t_FormatChunk fmt_chunk;
	t_DataChunk data_chunk;
} t_wav_file_hdr;

t_wav_file_hdr wave_file_hdr = 
{
	/* t_WAVRIFF_hdr */
	{
		{ 'R', 'I', 'F', 'F' }, /* groupID */
		0, /* size to update later */
		{ 'W', 'A', 'V', 'E' }
	},
	/* t_FormatChunk */
	{
		{ 'f', 'm', 't', ' ' }, /* char		chunkID[4];  */
		16, /* uint32_t	chunkSize; */
		1, /* uint16_t	wFormatTag; 1 fixed */
		2, /* uint16_t	wChannels; 2 fixed */
		0, /* uint32_t	dwSamplesPerSec; Freq Hz sampling to update later */
		0, /* uint32_t	dwAvgBytesPerSec; Freq Hz sampling x 2 to update later */
		2, /* uint16_t	wBlockAlign; 2 fixed */
		8, /* uint16_t	wBitsPerSample; 8 fixed */
	},
	/* t_DataChunk */
	{
	    { 'd', 'a', 't', 'a' }, /* char chunkID[4]; */
		0, /* uint32_t	chunkSize; to update later */
	}
};

typedef enum {
	TRANSCEIVER_MODE_OFF = 0,
	TRANSCEIVER_MODE_RX = 1,
	TRANSCEIVER_MODE_TX = 2,
	TRANSCEIVER_MODE_SS = 3

} transceiver_mode_t;
static transceiver_mode_t transceiver_mode = TRANSCEIVER_MODE_RX;

#define U64TOA_MAX_DIGIT (31)
typedef struct 
{
		char data[U64TOA_MAX_DIGIT+1];
} t_u64toa;

t_u64toa ascii_u64_data1;
t_u64toa ascii_u64_data2;

static float
TimevalDiff(const struct timeval *a, const struct timeval *b)
{
   return (a->tv_sec - b->tv_sec) + 1e-6f * (a->tv_usec - b->tv_usec);
}

int parse_u64(char* s, uint64_t* const value) {
	uint_fast8_t base = 10;
	char* s_end;
	uint64_t u64_value;

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
	u64_value = strtoull(s, &s_end, base);
	if( (s != s_end) && (*s_end == 0) ) {
		*value = u64_value;
		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_INVALID_PARAM;
	}
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


static char *stringrev(char *str)
{
	char *p1, *p2;

	if(! str || ! *str)
		return str;

	for(p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
	{
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

	do
	{
		digit = (sum % BASE);
		str->data[pos] = digit + '0';
		pos++;

		sum /= BASE;
	}while( (sum>0) && (pos < max_len) );

	if( (pos == max_len) && (sum>0) )
		return NULL;

	str->data[pos] = '\0';
	res = stringrev(str->data);

	return res;
}

volatile bool do_exit = false;

FILE* fd = NULL;
volatile uint32_t byte_count = 0;

bool signalsource = false;
uint32_t amplitude = 0;

bool receive = false;
bool receive_wav = false;

bool transmit = false;
struct timeval time_start;
struct timeval t_start;

bool automatic_tuning = false;
uint64_t freq_hz;

bool if_freq = false;
uint64_t if_freq_hz;

bool lo_freq = false;
uint64_t lo_freq_hz = DEFAULT_LO_HZ;

bool image_reject = false;
uint32_t image_reject_selection;

bool amp = false;
uint32_t amp_enable;

bool antenna = false;
uint32_t antenna_enable;

bool sample_rate = false;
uint32_t sample_rate_hz;

bool limit_num_samples = false;
uint64_t samples_to_xfer = 0;
size_t bytes_to_xfer = 0;

bool baseband_filter_bw = false;
uint32_t baseband_filter_bw_hz = 0;

int rx_callback(hackrf_transfer* transfer) {
	size_t bytes_to_write;
	int i;

	if( fd != NULL ) 
	{
		ssize_t bytes_written;
		byte_count += transfer->valid_length;
		bytes_to_write = transfer->valid_length;
		if (limit_num_samples) {
			if (bytes_to_write >= bytes_to_xfer) {
				bytes_to_write = bytes_to_xfer;
			}
			bytes_to_xfer -= bytes_to_write;
		}
		if (receive_wav) {
			/* convert .wav contents from signed to unsigned */
			for (i = 0; i < bytes_to_write; i++) {
				transfer->buffer[i] ^= (uint8_t)0x80;
			}
		}
		bytes_written = fwrite(transfer->buffer, 1, bytes_to_write, fd);
		if ((bytes_written != bytes_to_write)
				|| (limit_num_samples && (bytes_to_xfer == 0))) {
			return -1;
		} else {
			return 0;
		}
	} else {
		return -1;
	}
}

int tx_callback(hackrf_transfer* transfer) {
	size_t bytes_to_read;
	int i;

	if( fd != NULL )
	{
		ssize_t bytes_read;
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
		bytes_read = fread(transfer->buffer, 1, bytes_to_read, fd);
		if ((bytes_read != bytes_to_read)
				|| (limit_num_samples && (bytes_to_xfer == 0))) {
			return -1;
		} else {
			return 0;
		}
	} else if (transceiver_mode == TRANSCEIVER_MODE_SS) {
		/* Transmit continuous wave with specific amplitude */
		ssize_t bytes_read;
		byte_count += transfer->valid_length;
		bytes_to_read = transfer->valid_length;
		if (limit_num_samples) {
			if (bytes_to_read >= bytes_to_xfer) {
				bytes_to_read = bytes_to_xfer;
			}
			bytes_to_xfer -= bytes_to_read;
		}

		for(i = 0;i<bytes_to_read;i++)
			transfer->buffer[i] = amplitude;

		if (limit_num_samples && (bytes_to_xfer == 0)) {
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
	printf("\t-w # Receive data into file with WAV header and automatic name.\n");
	printf("\t   # This is for SDR# compatibility and may not work with other software.\n");
	printf("\t[-f freq_hz] # Frequency in Hz [%sMHz to %sMHz].\n",
		u64toa((FREQ_MIN_HZ/FREQ_ONE_MHZ),&ascii_u64_data1),
		u64toa((FREQ_MAX_HZ/FREQ_ONE_MHZ),&ascii_u64_data2));
	printf("\t[-i if_freq_hz] # Intermediate Frequency (IF) in Hz [%sMHz to %sMHz].\n",
		u64toa((IF_MIN_HZ/FREQ_ONE_MHZ),&ascii_u64_data1),
		u64toa((IF_MAX_HZ/FREQ_ONE_MHZ),&ascii_u64_data2));
	printf("\t[-o lo_freq_hz] # Front-end Local Oscillator (LO) frequency in Hz [%sMHz to %sMHz].\n",
		u64toa((LO_MIN_HZ/FREQ_ONE_MHZ),&ascii_u64_data1),
		u64toa((LO_MAX_HZ/FREQ_ONE_MHZ),&ascii_u64_data2));
	printf("\t[-m image_reject] # Image rejection filter selection, 0=bypass, 1=low pass, 2=high pass.\n");
	printf("\t[-a amp_enable] # RX/TX RF amplifier 1=Enable, 0=Disable.\n");
	printf("\t[-p antenna_enable] # Antenna port power, 1=Enable, 0=Disable.\n");
	printf("\t[-l gain_db] # RX LNA (IF) gain, 0-40dB, 8dB steps\n");
	printf("\t[-g gain_db] # RX VGA (baseband) gain, 0-62dB, 2dB steps\n");
	printf("\t[-x gain_db] # TX VGA (IF) gain, 0-47dB, 1dB steps\n");
	printf("\t[-s sample_rate_hz] # Sample rate in Hz (8/10/12.5/16/20MHz, default %sMHz).\n",
		u64toa((DEFAULT_SAMPLE_RATE_HZ/FREQ_ONE_MHZ),&ascii_u64_data1));
	printf("\t[-n num_samples] # Number of samples to transfer (default is unlimited).\n");
	printf("\t[-c amplitude] # CW signal source mode, amplitude 0-127 (DC value to DAC).\n");
	printf("\t[-b baseband_filter_bw_hz] # Set baseband filter bandwidth in MHz.\n\tPossible values: 1.75/2.5/3.5/5/5.5/6/7/8/9/10/12/14/15/20/24/28MHz, default < sample_rate_hz.\n" );
}

static hackrf_device* device = NULL;

#ifdef _MSC_VER
BOOL WINAPI
sighandler(int signum)
{
	if (CTRL_C_EVENT == signum) {
		fprintf(stdout, "Caught signal %d\n", signum);
		do_exit = true;
		return TRUE;
	}
	return FALSE;
}
#else
void sigint_callback_handler(int signum) 
{
	fprintf(stdout, "Caught signal %d\n", signum);
	do_exit = true;
}
#endif

#define PATH_FILE_MAX_LEN (FILENAME_MAX)
#define DATE_TIME_MAX_LEN (32)

int main(int argc, char** argv) {
	int opt;
	char path_file[PATH_FILE_MAX_LEN];
	char date_time[DATE_TIME_MAX_LEN];
	const char* path = NULL;
	int result;
	time_t rawtime;
	struct tm * timeinfo;
	long int file_pos;
	int exit_code = EXIT_SUCCESS;
	struct timeval t_end;
	float time_diff;
	unsigned int lna_gain=8, vga_gain=20, txvga_gain=0;
  
	while( (opt = getopt(argc, argv, "wr:t:f:i:o:m:a:p:s:n:b:l:g:x:c:")) != EOF )
	{
		result = HACKRF_SUCCESS;
		switch( opt ) 
		{
		case 'w':
			receive_wav = true;
			break;
		
		case 'r':
			receive = true;
			path = optarg;
			break;
		
		case 't':
			transmit = true;
			path = optarg;
			break;

		case 'f':
			automatic_tuning = true;
			result = parse_u64(optarg, &freq_hz);
			break;

		case 'i':
			if_freq = true;
			result = parse_u64(optarg, &if_freq_hz);
			break;

		case 'o':
			lo_freq = true;
			result = parse_u64(optarg, &lo_freq_hz);
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
			sample_rate = true;
			result = parse_u32(optarg, &sample_rate_hz);
			break;

		case 'n':
			limit_num_samples = true;
			result = parse_u64(optarg, &samples_to_xfer);
			bytes_to_xfer = samples_to_xfer * 2ull;
			break;

		case 'b':
			baseband_filter_bw = true;
			result = parse_u32(optarg, &baseband_filter_bw_hz);
			break;

		case 'c':
			transmit = true;
			signalsource = true;
			result = parse_u32(optarg, &amplitude);
			break;

		default:
			printf("unknown argument '-%c %s'\n", opt, optarg);
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
		printf("argument error: num_samples must be less than %s/%sMio\n",
			u64toa(SAMPLES_TO_XFER_MAX,&ascii_u64_data1),
			u64toa((SAMPLES_TO_XFER_MAX/FREQ_ONE_MHZ),&ascii_u64_data2));
		usage();
		return EXIT_FAILURE;
	}

	if (if_freq || lo_freq || image_reject) {
		/* explicit tuning selected */
		if (!if_freq) {
			printf("argument error: if_freq_hz must be specified for explicit tuning.\n");
			usage();
			return EXIT_FAILURE;
		}
		if (!image_reject) {
			printf("argument error: image_reject must be specified for explicit tuning.\n");
			usage();
			return EXIT_FAILURE;
		}
		if (!lo_freq && (image_reject_selection != RF_PATH_FILTER_BYPASS)) {
			printf("argument error: lo_freq_hz must be specified for explicit tuning unless image_reject is set to bypass.\n");
			usage();
			return EXIT_FAILURE;
		}
		if ((if_freq_hz > IF_MAX_HZ) || (if_freq_hz < IF_MIN_HZ)) {
			printf("argument error: if_freq_hz shall be between %s and %s.\n",
				u64toa(IF_MIN_HZ,&ascii_u64_data1),
				u64toa(IF_MAX_HZ,&ascii_u64_data2));
			usage();
			return EXIT_FAILURE;
		}
		if ((lo_freq_hz > LO_MAX_HZ) || (lo_freq_hz < LO_MIN_HZ)) {
			printf("argument error: lo_freq_hz shall be between %s and %s.\n",
				u64toa(LO_MIN_HZ,&ascii_u64_data1),
				u64toa(LO_MAX_HZ,&ascii_u64_data2));
			usage();
			return EXIT_FAILURE;
		}
		if (image_reject_selection > 2) {
			printf("argument error: image_reject must be 0, 1, or 2 .\n");
			usage();
			return EXIT_FAILURE;
		}
		if (automatic_tuning) {
			printf("warning: freq_hz ignored by explicit tuning selection.\n");
			automatic_tuning = false;
		}
		switch (image_reject_selection) {
		case RF_PATH_FILTER_BYPASS:
			freq_hz = if_freq_hz;
			break;
		case RF_PATH_FILTER_LOW_PASS:
			freq_hz = abs(if_freq_hz - lo_freq_hz);
			break;
		case RF_PATH_FILTER_HIGH_PASS:
			freq_hz = if_freq_hz + lo_freq_hz;
			break;
		default:
			freq_hz = DEFAULT_FREQ_HZ;
			break;
		}
		printf("explicit tuning specified for %s Hz.\n",
			u64toa(freq_hz,&ascii_u64_data1));

	} else if (automatic_tuning) {
		if( (freq_hz > FREQ_MAX_HZ) || (freq_hz < FREQ_MIN_HZ) )
		{
			printf("argument error: freq_hz shall be between %s and %s.\n",
				u64toa(FREQ_MIN_HZ,&ascii_u64_data1),
				u64toa(FREQ_MAX_HZ,&ascii_u64_data2));
			usage();
			return EXIT_FAILURE;
		}
	} else {
		/* Use default freq */
		freq_hz = DEFAULT_FREQ_HZ;
		automatic_tuning = true;
	}

	if( amp ) {
		if( amp_enable > 1 )
		{
			printf("argument error: amp_enable shall be 0 or 1.\n");
			usage();
			return EXIT_FAILURE;
		}
	}

	if (antenna) {
		if (antenna_enable > 1) {
			printf("argument error: antenna_enable shall be 0 or 1.\n");
			usage();
			return EXIT_FAILURE;
		}
	}

	if( sample_rate == false ) 
	{
		sample_rate_hz = DEFAULT_SAMPLE_RATE_HZ;
	}

	if( baseband_filter_bw )
	{
		/* Compute nearest freq for bw filter */
		baseband_filter_bw_hz = hackrf_compute_baseband_filter_bw(baseband_filter_bw_hz);
	}else
	{
		/* Compute default value depending on sample rate */
		baseband_filter_bw_hz = hackrf_compute_baseband_filter_bw_round_down_lt(sample_rate_hz);
	}

	if (baseband_filter_bw_hz > BASEBAND_FILTER_BW_MAX) {
		printf("argument error: baseband_filter_bw_hz must be less or equal to %u Hz/%.03f MHz\n",
				BASEBAND_FILTER_BW_MAX, (float)(BASEBAND_FILTER_BW_MAX/FREQ_ONE_MHZ));
		usage();
		return EXIT_FAILURE;
	}

	if (baseband_filter_bw_hz < BASEBAND_FILTER_BW_MIN) {
		printf("argument error: baseband_filter_bw_hz must be greater or equal to %u Hz/%.03f MHz\n",
				BASEBAND_FILTER_BW_MIN, (float)(BASEBAND_FILTER_BW_MIN/FREQ_ONE_MHZ));
		usage();
		return EXIT_FAILURE;
	}

	if( (transmit == false) && (receive == receive_wav) )
	{
		printf("receive -r and receive_wav -w options are mutually exclusive\n");
		usage();
		return EXIT_FAILURE;
	}
	
	if( receive_wav == false )
	{
		if( transmit == receive ) 
		{
			if( transmit == true ) 
			{
				printf("receive -r and transmit -t options are mutually exclusive\n");
			} else
			{
				printf("specify either transmit -t or receive -r or receive_wav -w option\n");
			}
			usage();
			return EXIT_FAILURE;
		}
	}
	
	if( receive ) {
		transceiver_mode = TRANSCEIVER_MODE_RX;
	}
	
	if( transmit ) {
		transceiver_mode = TRANSCEIVER_MODE_TX;
	}

	if (signalsource) {
		transceiver_mode = TRANSCEIVER_MODE_SS;
		if (amplitude >127) {
			printf("argument error: amplitude shall be in between 0 and 128.\n");
			usage();
			return EXIT_FAILURE;
		}
	}

	if( receive_wav )
	{
		time (&rawtime);
		timeinfo = localtime (&rawtime);
		transceiver_mode = TRANSCEIVER_MODE_RX;
		/* File format HackRF Year(2013), Month(11), Day(28), Hour Min Sec+Z, Freq kHz, IQ.wav */
		strftime(date_time, DATE_TIME_MAX_LEN, "%Y%m%d_%H%M%S", timeinfo);
		snprintf(path_file, PATH_FILE_MAX_LEN, "HackRF_%sZ_%ukHz_IQ.wav", date_time, (uint32_t)(freq_hz/(1000ull)) );
		path = path_file;
		printf("Receive wav file: %s\n", path);
	}	

	// In signal source mode, the PATH argument is neglected.
	if (transceiver_mode != TRANSCEIVER_MODE_SS) {
		if( path == NULL ) {
			printf("specify a path to a file to transmit/receive\n");
			usage();
			return EXIT_FAILURE;
		}
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
		usage();
		return EXIT_FAILURE;
	}
	
	if (transceiver_mode != TRANSCEIVER_MODE_SS) {
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
		/* Change fd buffer to have bigger one to store or read data on/to HDD */
		result = setvbuf(fd , NULL , _IOFBF , FD_BUFFER_SIZE);
		if( result != 0 ) {
			printf("setvbuf() failed: %d\n", result);
			usage();
			return EXIT_FAILURE;
		}
	}

	/* Write Wav header */
	if( receive_wav ) 
	{
		fwrite(&wave_file_hdr, 1, sizeof(t_wav_file_hdr), fd);
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
	printf("call hackrf_sample_rate_set(%u Hz/%.03f MHz)\n", sample_rate_hz,((float)sample_rate_hz/(float)FREQ_ONE_MHZ));
	result = hackrf_set_sample_rate_manual(device, sample_rate_hz, 1);
	if( result != HACKRF_SUCCESS ) {
		printf("hackrf_sample_rate_set() failed: %s (%d)\n", hackrf_error_name(result), result);
		usage();
		return EXIT_FAILURE;
	}

	printf("call hackrf_baseband_filter_bandwidth_set(%d Hz/%.03f MHz)\n",
			baseband_filter_bw_hz, ((float)baseband_filter_bw_hz/(float)FREQ_ONE_MHZ));
	result = hackrf_set_baseband_filter_bandwidth(device, baseband_filter_bw_hz);
	if( result != HACKRF_SUCCESS ) {
		printf("hackrf_baseband_filter_bandwidth_set() failed: %s (%d)\n", hackrf_error_name(result), result);
		usage();
		return EXIT_FAILURE;
	}

	if( transceiver_mode == TRANSCEIVER_MODE_RX ) {
		result = hackrf_set_vga_gain(device, vga_gain);
		result |= hackrf_set_lna_gain(device, lna_gain);
		result |= hackrf_start_rx(device, rx_callback, NULL);
	} else {
		result = hackrf_set_txvga_gain(device, txvga_gain);
		result |= hackrf_start_tx(device, tx_callback, NULL);
	}
	if( result != HACKRF_SUCCESS ) {
		printf("hackrf_start_?x() failed: %s (%d)\n", hackrf_error_name(result), result);
		usage();
		return EXIT_FAILURE;
	}

	if (automatic_tuning) {
		printf("call hackrf_set_freq(%s Hz/%.03f MHz)\n",
			u64toa(freq_hz, &ascii_u64_data1),((double)freq_hz/(double)FREQ_ONE_MHZ) );
		result = hackrf_set_freq(device, freq_hz);
		if( result != HACKRF_SUCCESS ) {
			printf("hackrf_set_freq() failed: %s (%d)\n", hackrf_error_name(result), result);
			usage();
			return EXIT_FAILURE;
		}
	} else {
		printf("call hackrf_set_freq_explicit() with %s Hz IF, %s Hz LO, %s\n",
				u64toa(if_freq_hz,&ascii_u64_data1),
				u64toa(lo_freq_hz,&ascii_u64_data2),
				hackrf_filter_path_name(image_reject_selection));
		result = hackrf_set_freq_explicit(device, if_freq_hz, lo_freq_hz,
				image_reject_selection);
		if (result != HACKRF_SUCCESS) {
			printf("hackrf_set_freq_explicit() failed: %s (%d)\n",
					hackrf_error_name(result), result);
			usage();
			return EXIT_FAILURE;
		}
	}

	if( amp ) {
		printf("call hackrf_set_amp_enable(%u)\n", amp_enable);
		result = hackrf_set_amp_enable(device, (uint8_t)amp_enable);
		if( result != HACKRF_SUCCESS ) {
			printf("hackrf_set_amp_enable() failed: %s (%d)\n", hackrf_error_name(result), result);
			usage();
			return EXIT_FAILURE;
		}
	}

	if (antenna) {
		printf("call hackrf_set_antenna_enable(%u)\n", antenna_enable);
		result = hackrf_set_antenna_enable(device, (uint8_t)antenna_enable);
		if (result != HACKRF_SUCCESS) {
			printf("hackrf_set_antenna_enable() failed: %s (%d)\n", hackrf_error_name(result), result);
			usage();
			return EXIT_FAILURE;
		}
	}

	if( limit_num_samples ) {
		printf("samples_to_xfer %s/%sMio\n",
		u64toa(samples_to_xfer,&ascii_u64_data1),
		u64toa((samples_to_xfer/FREQ_ONE_MHZ),&ascii_u64_data2) );
	}
	
	gettimeofday(&t_start, NULL);
	gettimeofday(&time_start, NULL);

	printf("Stop with Ctrl-C\n");
	while( (hackrf_is_streaming(device) == HACKRF_TRUE) &&
			(do_exit == false) ) 
	{
		uint32_t byte_count_now;
		struct timeval time_now;
		float time_difference, rate;
		sleep(1);
		
		gettimeofday(&time_now, NULL);
		
		byte_count_now = byte_count;
		byte_count = 0;
		
		time_difference = TimevalDiff(&time_now, &time_start);
		rate = (float)byte_count_now / time_difference;
		printf("%4.1f MiB / %5.3f sec = %4.1f MiB/second\n",
				(byte_count_now / 1e6f), time_difference, (rate / 1e6f) );

		time_start = time_now;

		if (byte_count_now == 0) {
			exit_code = EXIT_FAILURE;
			printf("\nCouldn't transfer any bytes for one second.\n");
			break;
		}
	}
	
	result = hackrf_is_streaming(device);	
	if (do_exit)
	{
		printf("\nUser cancel, exiting...\n");
	} else {
		printf("\nExiting... hackrf_is_streaming() result: %s (%d)\n", hackrf_error_name(result), result);
	}
	
	gettimeofday(&t_end, NULL);
	time_diff = TimevalDiff(&t_end, &t_start);
	printf("Total time: %5.5f s\n", time_diff);
	
	if(device != NULL)
	{
		if( receive ) 
		{
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
		if( receive_wav ) 
		{
			/* Get size of file */
			file_pos = ftell(fd);
			/* Update Wav Header */
			wave_file_hdr.hdr.size = file_pos+8;
			wave_file_hdr.fmt_chunk.dwSamplesPerSec = sample_rate_hz;
			wave_file_hdr.fmt_chunk.dwAvgBytesPerSec = wave_file_hdr.fmt_chunk.dwSamplesPerSec*2;
			wave_file_hdr.data_chunk.chunkSize = file_pos - sizeof(t_wav_file_hdr);
			/* Overwrite header with updated data */
			rewind(fd);
			fwrite(&wave_file_hdr, 1, sizeof(t_wav_file_hdr), fd);
		}	
		fclose(fd);
		fd = NULL;
		printf("fclose(fd) done\n");
	}
	printf("exit\n");
	return exit_code;
}
