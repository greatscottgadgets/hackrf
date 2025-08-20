/*
 * Copyright 2016-2022 Great Scott Gadgets <info@greatscottgadgets.com>
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
#include <inttypes.h>

#define _FILE_OFFSET_BITS 64

#ifndef bool
typedef int bool;
	#define true 1
	#define false 0
#endif

#ifdef _WIN32
	#define _USE_MATH_DEFINES
	#include <windows.h>
	#include <io.h>
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
#include <math.h>

#define FD_BUFFER_SIZE (8 * 1024)

#define FREQ_ONE_MHZ (1000000ull)

#define FREQ_MIN_MHZ (0)    /*    0 MHz */
#define FREQ_MAX_MHZ (7250) /* 7250 MHz */

#define DEFAULT_SAMPLE_RATE_HZ            (20000000) /* 20MHz default sample rate */
#define DEFAULT_BASEBAND_FILTER_BANDWIDTH (15000000) /* 15MHz default */

#define TUNE_STEP (DEFAULT_SAMPLE_RATE_HZ / FREQ_ONE_MHZ)
#define OFFSET    7500000

#define BLOCKS_PER_TRANSFER 16
#define THROWAWAY_BLOCKS    2

#if defined _WIN32
	#define m_sleep(a) Sleep((a))
#else
	#define m_sleep(a) usleep((a * 1000))
#endif

uint32_t num_sweeps = 0;
int num_ranges = 0;
uint16_t frequencies[MAX_SWEEP_RANGES * 2];
int step_count;

static float TimevalDiff(const struct timeval* a, const struct timeval* b)
{
	return (a->tv_sec - b->tv_sec) + 1e-6f * (a->tv_usec - b->tv_usec);
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

int parse_u32_range(char* s, uint32_t* const value_min, uint32_t* const value_max)
{
	int result;

	char* sep = strchr(s, ':');
	if (!sep) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	*sep = 0;

	result = parse_u32(s, value_min);
	if (result != HACKRF_SUCCESS) {
		return result;
	}
	result = parse_u32(sep + 1, value_max);
	if (result != HACKRF_SUCCESS) {
		return result;
	}

	return HACKRF_SUCCESS;
}

volatile bool do_exit = false;

FILE* outfile = NULL;
volatile uint32_t byte_count = 0;
volatile uint64_t sweep_count = 0;

struct timeval time_start;
struct timeval t_start;

bool amp = false;
uint32_t amp_enable;

bool antenna = false;
uint32_t antenna_enable;

bool timestamp_normalized = false;
bool binary_output = false;
bool ifft_output = false;
bool one_shot = false;
bool finite_mode = false;
volatile bool sweep_started = false;

int fftSize = 20;
double fft_bin_width;
fftwf_complex* fftwIn = NULL;
fftwf_complex* fftwOut = NULL;
fftwf_plan fftwPlan = NULL;
fftwf_complex* ifftwIn = NULL;
fftwf_complex* ifftwOut = NULL;
fftwf_plan ifftwPlan = NULL;
uint32_t ifft_idx = 0;
float* pwr;
float* window;

struct timeval usb_transfer_time;

float logPower(fftwf_complex in, float scale)
{
	float re = in[0] * scale;
	float im = in[1] * scale;
	float magsq = re * re + im * im;
	return (float) (log2(magsq) * 10.0f / log2(10.0f));
}

int rx_callback(hackrf_transfer* transfer)
{
	int8_t* buf;
	uint8_t* ubuf;
	uint64_t frequency; /* in Hz */
	uint64_t band_edge;
	uint32_t record_length;
	int i, j, ifft_bins;
	struct tm* fft_time;
	char time_str[50];

	if (NULL == outfile) {
		return -1;
	}

	if (do_exit) {
		return 0;
	}

	// happens only once with timestamp_normalized == true
	if ((usb_transfer_time.tv_sec == 0 && usb_transfer_time.tv_usec == 0) ||
	    timestamp_normalized == false) {
		// set the timestamp for the first sweep
		gettimeofday(&usb_transfer_time, NULL);
	}

	byte_count += transfer->valid_length;
	buf = (int8_t*) transfer->buffer;
	ifft_bins = fftSize * step_count;
	for (j = 0; j < BLOCKS_PER_TRANSFER; j++) {
		ubuf = (uint8_t*) buf;
		if (ubuf[0] == 0x7F && ubuf[1] == 0x7F) {
			frequency = ((uint64_t) (ubuf[9]) << 56) |
				((uint64_t) (ubuf[8]) << 48) |
				((uint64_t) (ubuf[7]) << 40) |
				((uint64_t) (ubuf[6]) << 32) |
				((uint64_t) (ubuf[5]) << 24) |
				((uint64_t) (ubuf[4]) << 16) |
				((uint64_t) (ubuf[3]) << 8) | ubuf[2];
		} else {
			buf += BYTES_PER_BLOCK;
			continue;
		}
		if (frequency == (uint64_t) (FREQ_ONE_MHZ * frequencies[0])) {
			if (sweep_started) {
				if (ifft_output) {
					fftwf_execute(ifftwPlan);
					for (i = 0; i < ifft_bins; i++) {
						ifftwOut[i][0] *= 1.0f / ifft_bins;
						ifftwOut[i][1] *= 1.0f / ifft_bins;
						fwrite(&ifftwOut[i][0],
						       sizeof(float),
						       1,
						       outfile);
						fwrite(&ifftwOut[i][1],
						       sizeof(float),
						       1,
						       outfile);
					}
				}
				sweep_count++;

				if (timestamp_normalized == true) {
					// set the timestamp of the next sweep
					gettimeofday(&usb_transfer_time, NULL);
				}

				if (one_shot) {
					do_exit = true;
				} else if (finite_mode && sweep_count == num_sweeps) {
					do_exit = true;
				}
			}
			sweep_started = true;
		}
		if (do_exit) {
			return 0;
		}
		if (!sweep_started) {
			buf += BYTES_PER_BLOCK;
			continue;
		}
		if ((FREQ_MAX_MHZ * FREQ_ONE_MHZ) < frequency) {
			buf += BYTES_PER_BLOCK;
			continue;
		}
		/* copy to fftwIn as floats */
		buf += BYTES_PER_BLOCK - (fftSize * 2);
		for (i = 0; i < fftSize; i++) {
			fftwIn[i][0] = buf[i * 2] * window[i] * 1.0f / 128.0f;
			fftwIn[i][1] = buf[i * 2 + 1] * window[i] * 1.0f / 128.0f;
		}
		buf += fftSize * 2;
		fftwf_execute(fftwPlan);
		for (i = 0; i < fftSize; i++) {
			pwr[i] = logPower(fftwOut[i], 1.0f / fftSize);
		}
		if (binary_output) {
			record_length =
				2 * sizeof(band_edge) + (fftSize / 4) * sizeof(float);

			fwrite(&record_length, sizeof(record_length), 1, outfile);
			band_edge = frequency;
			fwrite(&band_edge, sizeof(band_edge), 1, outfile);
			band_edge = frequency + DEFAULT_SAMPLE_RATE_HZ / 4;
			fwrite(&band_edge, sizeof(band_edge), 1, outfile);
			fwrite(&pwr[1 + (fftSize * 5) / 8],
			       sizeof(float),
			       fftSize / 4,
			       outfile);

			fwrite(&record_length, sizeof(record_length), 1, outfile);
			band_edge = frequency + DEFAULT_SAMPLE_RATE_HZ / 2;
			fwrite(&band_edge, sizeof(band_edge), 1, outfile);
			band_edge = frequency + (DEFAULT_SAMPLE_RATE_HZ * 3) / 4;
			fwrite(&band_edge, sizeof(band_edge), 1, outfile);
			fwrite(&pwr[1 + fftSize / 8], sizeof(float), fftSize / 4, outfile);
		} else if (ifft_output) {
			ifft_idx = (uint32_t) round(
				(frequency - (uint64_t) (FREQ_ONE_MHZ * frequencies[0])) /
				fft_bin_width);
			ifft_idx = (ifft_idx + ifft_bins / 2) % ifft_bins;
			for (i = 0; (fftSize / 4) > i; i++) {
				ifftwIn[ifft_idx + i][0] =
					fftwOut[i + 1 + (fftSize * 5) / 8][0];
				ifftwIn[ifft_idx + i][1] =
					fftwOut[i + 1 + (fftSize * 5) / 8][1];
			}
			ifft_idx += fftSize / 2;
			ifft_idx %= ifft_bins;
			for (i = 0; (fftSize / 4) > i; i++) {
				ifftwIn[ifft_idx + i][0] =
					fftwOut[i + 1 + (fftSize / 8)][0];
				ifftwIn[ifft_idx + i][1] =
					fftwOut[i + 1 + (fftSize / 8)][1];
			}
		} else {
			time_t time_stamp_seconds = usb_transfer_time.tv_sec;
			fft_time = localtime(&time_stamp_seconds);
			strftime(time_str, 50, "%Y-%m-%d, %H:%M:%S", fft_time);
			fprintf(outfile,
				"%s.%06ld, %" PRIu64 ", %" PRIu64 ", %.2f, %u",
				time_str,
				(long int) usb_transfer_time.tv_usec,
				(uint64_t) (frequency),
				(uint64_t) (frequency + DEFAULT_SAMPLE_RATE_HZ / 4),
				fft_bin_width,
				fftSize);
			for (i = 0; (fftSize / 4) > i; i++) {
				fprintf(outfile,
					", %.2f",
					pwr[i + 1 + (fftSize * 5) / 8]);
			}
			fprintf(outfile, "\n");
			fprintf(outfile,
				"%s.%06ld, %" PRIu64 ", %" PRIu64 ", %.2f, %u",
				time_str,
				(long int) usb_transfer_time.tv_usec,
				(uint64_t) (frequency + (DEFAULT_SAMPLE_RATE_HZ / 2)),
				(uint64_t) (frequency + ((DEFAULT_SAMPLE_RATE_HZ * 3) / 4)),
				fft_bin_width,
				fftSize);
			for (i = 0; (fftSize / 4) > i; i++) {
				fprintf(outfile, ", %.2f", pwr[i + 1 + (fftSize / 8)]);
			}
			fprintf(outfile, "\n");
		}
	}
	return 0;
}

static void usage()
{
	fprintf(stderr,
		"Usage:\n"
		"\t[-h] # this help\n"
		"\t[-d serial_number] # Serial number of desired HackRF\n"
		"\t[-a amp_enable] # RX RF amplifier 1=Enable, 0=Disable\n"
		"\t[-f freq_min:freq_max] # minimum and maximum frequencies in MHz\n"
		"\t[-p antenna_enable] # Antenna port power, 1=Enable, 0=Disable\n"
		"\t[-l gain_db] # RX LNA (IF) gain, 0-40dB, 8dB steps\n"
		"\t[-g gain_db] # RX VGA (baseband) gain, 0-62dB, 2dB steps\n"
		"\t[-w bin_width] # FFT bin width (frequency resolution) in Hz, 2445-5000000\n"
		"\t[-W wisdom_file] # Use FFTW wisdom file (will be created if necessary)\n"
		"\t[-P estimate|measure|patient|exhaustive] # FFTW plan type, default is 'measure'\n"
		"\t[-1] # one shot mode\n"
		"\t[-N num_sweeps] # Number of sweeps to perform\n"
		"\t[-B] # binary output\n"
		"\t[-I] # binary inverse FFT output\n"
		"\t[-n] # keep the same timestamp within a sweep\n"
		"\t-r filename # output file\n"
		"\n"
		"Output fields:\n"
		"\tdate, time, hz_low, hz_high, hz_bin_width, num_samples, dB, dB, . . .\n");
}

static hackrf_device* device = NULL;

#ifdef _MSC_VER
BOOL WINAPI sighandler(int signum)
{
	if (CTRL_C_EVENT == signum || CTRL_BREAK_EVENT == signum) {
		fprintf(stderr, "Caught signal %d\n", signum);
		do_exit = true;
		return TRUE;
	}
	return FALSE;
}
#else
void sigint_callback_handler(int signum)
{
	fprintf(stderr, "Caught signal %d\n", signum);
	do_exit = true;
}
#endif

int import_wisdom(const char* path)
{
	// Returns nonzero
	if (!fftwf_import_wisdom_from_filename(path)) {
		fprintf(stderr,
			"Wisdom file %s not found; will attempt to create it\n",
			path);
		return 0;
	}

	return 1;
}

int import_default_wisdom()
{
	return fftwf_import_system_wisdom();
}

int export_wisdom(const char* path)
{
	if (path != NULL) {
		if (!fftwf_export_wisdom_to_filename(path)) {
			fprintf(stderr, "Could not write FFTW wisdom file to %s", path);
			return 0;
		}
	}

	return 1;
}

int main(int argc, char** argv)
{
	int opt, i, result = 0;
	const char* path = NULL;
	const char* serial_number = NULL;
	int exit_code = EXIT_SUCCESS;
	struct timeval time_now;
	struct timeval time_prev;
	float time_diff;
	float sweep_rate = 0;
	unsigned int lna_gain = 16, vga_gain = 20;
	uint32_t freq_min = 0;
	uint32_t freq_max = 6000;
	uint32_t requested_fft_bin_width;
	const char* fftwWisdomPath = NULL;
	int fftw_plan_type = FFTW_MEASURE;

	while ((opt = getopt(argc, argv, "a:f:p:l:g:d:N:w:W:P:n1BIr:h?")) != EOF) {
		result = HACKRF_SUCCESS;
		switch (opt) {
		case 'd':
			serial_number = optarg;
			break;

		case 'a':
			amp = true;
			result = parse_u32(optarg, &amp_enable);
			break;

		case 'f':
			result = parse_u32_range(optarg, &freq_min, &freq_max);
			if (freq_min >= freq_max) {
				fprintf(stderr,
					"argument error: freq_max must be greater than freq_min.\n");
				usage();
				return EXIT_FAILURE;
			}
			if (FREQ_MAX_MHZ < freq_max) {
				fprintf(stderr,
					"argument error: freq_max may not be higher than %u.\n",
					FREQ_MAX_MHZ);
				usage();
				return EXIT_FAILURE;
			}
			if (MAX_SWEEP_RANGES <= num_ranges) {
				fprintf(stderr,
					"argument error: specify a maximum of %u frequency ranges.\n",
					MAX_SWEEP_RANGES);
				usage();
				return EXIT_FAILURE;
			}
			frequencies[2 * num_ranges] = (uint16_t) freq_min;
			frequencies[2 * num_ranges + 1] = (uint16_t) freq_max;
			num_ranges++;
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

		case 'N':
			finite_mode = true;
			result = parse_u32(optarg, &num_sweeps);
			break;

		case 'w':
			result = parse_u32(optarg, &requested_fft_bin_width);
			fftSize = DEFAULT_SAMPLE_RATE_HZ / requested_fft_bin_width;
			break;

		case 'W':
			fftwWisdomPath = optarg;
			break;

		case 'P':
			if (strcmp("estimate", optarg) == 0) {
				fftw_plan_type = FFTW_ESTIMATE;
			} else if (strcmp("measure", optarg) == 0) {
				fftw_plan_type = FFTW_MEASURE;
			} else if (strcmp("patient", optarg) == 0) {
				fftw_plan_type = FFTW_PATIENT;
			} else if (strcmp("exhaustive", optarg) == 0) {
				fftw_plan_type = FFTW_EXHAUSTIVE;
			} else {
				fprintf(stderr, "Unknown FFTW plan type '%s'\n", optarg);
				return EXIT_FAILURE;
			}
			break;

		case 'n':
			timestamp_normalized = true;
			break;

		case '1':
			one_shot = true;
			break;

		case 'B':
			binary_output = true;
			break;

		case 'I':
			ifft_output = true;
			break;

		case 'r':
			path = optarg;
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

	// Try to load a wisdom file if specified, otherwise
	// try to load the system-wide wisdom file
	if (fftwWisdomPath) {
		import_wisdom(fftwWisdomPath);
	} else {
		import_default_wisdom();
	}

	if (lna_gain % 8) {
		fprintf(stderr, "warning: lna_gain (-l) must be a multiple of 8\n");
	}

	if (vga_gain % 2) {
		fprintf(stderr, "warning: vga_gain (-g) must be a multiple of 2\n");
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

	if (0 == num_ranges) {
		frequencies[0] = (uint16_t) freq_min;
		frequencies[1] = (uint16_t) freq_max;
		num_ranges++;
	}

	if (binary_output && ifft_output) {
		fprintf(stderr,
			"argument error: binary output (-B) and IFFT output (-I) are mutually exclusive.\n");
		return EXIT_FAILURE;
	}

	if (ifft_output && (1 < num_ranges)) {
		fprintf(stderr,
			"argument error: only one frequency range is supported in IFFT output (-I) mode.\n");
		return EXIT_FAILURE;
	}

	/*
	 * The FFT bin width must be no more than a quarter of the sample rate
	 * for interleaved mode. With our fixed sample rate of 20 Msps, that
	 * results in a maximum bin width of 5000000 Hz.
	 */
	if (4 > fftSize) {
		fprintf(stderr,
			"argument error: FFT bin width (-w) must be no more than 5000000\n");
		return EXIT_FAILURE;
	}

	/*
	 * The maximum number of FFT bins we support is equal to the number of
	 * samples in a block. Each block consists of 16384 bytes minus 10
	 * bytes for the frequency header, leaving room for 8187 two-byte
	 * samples. As we pad fftSize up to the next odd multiple of four, this
	 * makes our maximum supported fftSize 8180.  With our fixed sample
	 * rate of 20 Msps, that results in a minimum bin width of 2445 Hz.
	 */
	if (8180 < fftSize) {
		fprintf(stderr,
			"argument error: FFT bin width (-w) must be no less than 2445\n");
		return EXIT_FAILURE;
	}

	/* In interleaved mode, the FFT bin selection works best if the total
	 * number of FFT bins is equal to an odd multiple of four.
	 * (e.g. 4, 12, 20, 28, 36, . . .)
	 */
	while ((fftSize + 4) % 8) {
		fftSize++;
	}

	fft_bin_width = (double) DEFAULT_SAMPLE_RATE_HZ / fftSize;
	fftwIn = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fftSize);
	fftwOut = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fftSize);
	fftwPlan =
		fftwf_plan_dft_1d(fftSize, fftwIn, fftwOut, FFTW_FORWARD, fftw_plan_type);
	pwr = (float*) fftwf_malloc(sizeof(float) * fftSize);
	window = (float*) fftwf_malloc(sizeof(float) * fftSize);
	for (i = 0; i < fftSize; i++) {
		window[i] = (float) (0.5f * (1.0f - cos(2 * M_PI * i / (fftSize - 1))));
	}

	/* Execute the plan once to make sure it's ready to go when real
	 * data starts to flow.  See issue #1366
	*/
	fftwf_execute(fftwPlan);

	// reset the timestamp
	memset(&usb_transfer_time, 0, sizeof(usb_transfer_time));

#ifdef _MSC_VER
	if (binary_output) {
		_setmode(_fileno(stdout), _O_BINARY);
	}
#endif

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

	if ((NULL == path) || (strcmp(path, "-") == 0)) {
		outfile = stdout;
	} else {
		outfile = fopen(path, "wb");
	}

	if (NULL == outfile) {
		fprintf(stderr, "Failed to open file: %s\n", path);
		return EXIT_FAILURE;
	}
	/* Change outfile buffer to have bigger one to store or read data on/to HDD */
	result = setvbuf(outfile, NULL, _IOFBF, FD_BUFFER_SIZE);
	if (result != 0) {
		fprintf(stderr, "setvbuf() failed: %d\n", result);
		usage();
		return EXIT_FAILURE;
	}

#ifdef _MSC_VER
	SetConsoleCtrlHandler((PHANDLER_ROUTINE) sighandler, TRUE);
#else
	signal(SIGINT, &sigint_callback_handler);
	signal(SIGILL, &sigint_callback_handler);
	signal(SIGFPE, &sigint_callback_handler);
	signal(SIGSEGV, &sigint_callback_handler);
	signal(SIGTERM, &sigint_callback_handler);
	signal(SIGABRT, &sigint_callback_handler);
#endif
	fprintf(stderr,
		"call hackrf_sample_rate_set(%.03f MHz)\n",
		((float) DEFAULT_SAMPLE_RATE_HZ / (float) FREQ_ONE_MHZ));
	result = hackrf_set_sample_rate_manual(device, DEFAULT_SAMPLE_RATE_HZ, 1);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_sample_rate_set() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		usage();
		return EXIT_FAILURE;
	}

	fprintf(stderr,
		"call hackrf_baseband_filter_bandwidth_set(%.03f MHz)\n",
		((float) DEFAULT_BASEBAND_FILTER_BANDWIDTH / (float) FREQ_ONE_MHZ));
	result = hackrf_set_baseband_filter_bandwidth(
		device,
		DEFAULT_BASEBAND_FILTER_BANDWIDTH);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_baseband_filter_bandwidth_set() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		usage();
		return EXIT_FAILURE;
	}

	result = hackrf_set_vga_gain(device, vga_gain);
	result |= hackrf_set_lna_gain(device, lna_gain);

	/*
	 * For each range, plan a whole number of tuning steps of a certain
	 * bandwidth. Increase high end of range if necessary to accommodate a
	 * whole number of steps, minimum 1.
	 */
	for (i = 0; i < num_ranges; i++) {
		step_count =
			1 + (frequencies[2 * i + 1] - frequencies[2 * i] - 1) / TUNE_STEP;
		frequencies[2 * i + 1] =
			(uint16_t) (frequencies[2 * i] + step_count * TUNE_STEP);
		fprintf(stderr,
			"Sweeping from %u MHz to %u MHz\n",
			frequencies[2 * i],
			frequencies[2 * i + 1]);
	}

	if (ifft_output) {
		ifftwIn = (fftwf_complex*) fftwf_malloc(
			sizeof(fftwf_complex) * fftSize * step_count);
		ifftwOut = (fftwf_complex*) fftwf_malloc(
			sizeof(fftwf_complex) * fftSize * step_count);
		ifftwPlan = fftwf_plan_dft_1d(
			fftSize * step_count,
			ifftwIn,
			ifftwOut,
			FFTW_BACKWARD,
			fftw_plan_type);

		/* Execute the plan once to make sure it's ready to go when real
	 	 * data starts to flow.  See issue #1366
		*/
		fftwf_execute(ifftwPlan);
	}

	result = hackrf_init_sweep(
		device,
		frequencies,
		num_ranges,
		BYTES_PER_BLOCK,
		TUNE_STEP * FREQ_ONE_MHZ,
		OFFSET,
		INTERLEAVED);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_init_sweep() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		return EXIT_FAILURE;
	}

	result |= hackrf_start_rx_sweep(device, rx_callback, NULL);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_start_rx_sweep() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		usage();
		return EXIT_FAILURE;
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

	gettimeofday(&t_start, NULL);
	time_prev = t_start;

	fprintf(stderr, "Stop with Ctrl-C\n");
	while ((hackrf_is_streaming(device) == HACKRF_TRUE) && (do_exit == false)) {
		float time_difference;
		m_sleep(50);

		gettimeofday(&time_now, NULL);
		if (TimevalDiff(&time_now, &time_prev) >= 1.0f) {
			time_difference = TimevalDiff(&time_now, &t_start);
			sweep_rate = (float) sweep_count / time_difference;
			fprintf(stderr,
				"%" PRIu64
				" total sweeps completed, %.2f sweeps/second\n",
				sweep_count,
				sweep_rate);

			if (byte_count == 0) {
				exit_code = EXIT_FAILURE;
				fprintf(stderr,
					"\nCouldn't transfer any data for one second.\n");
				break;
			}
			byte_count = 0;
			time_prev = time_now;
		}
	}

	fflush(outfile);
	result = hackrf_is_streaming(device);
	if (do_exit) {
		fprintf(stderr, "\nExiting...\n");
	} else {
		fprintf(stderr,
			"\nExiting... hackrf_is_streaming() result: %s (%d)\n",
			hackrf_error_name(result),
			result);
	}

	gettimeofday(&time_now, NULL);
	time_diff = TimevalDiff(&time_now, &t_start);
	if ((sweep_rate == 0) && (time_diff > 0)) {
		sweep_rate = sweep_count / time_diff;
	}
	fprintf(stderr,
		"Total sweeps: %" PRIu64 " in %.5f seconds (%.2f sweeps/second)\n",
		sweep_count,
		time_diff,
		sweep_rate);

	if (device != NULL) {
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

	fflush(outfile);
	if ((outfile != NULL) && (outfile != stdout)) {
		fclose(outfile);
		outfile = NULL;
		fprintf(stderr, "fclose() done\n");
	}
	fftwf_free(fftwIn);
	fftwf_free(fftwOut);
	fftwf_free(pwr);
	fftwf_free(window);
	fftwf_free(ifftwIn);
	fftwf_free(ifftwOut);
	export_wisdom(fftwWisdomPath);
	fprintf(stderr, "exit\n");
	return exit_code;
}
