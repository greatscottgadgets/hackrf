/*
 * Copyright (C) 2019-2022 by Alexander Protasov <wakko@acmelabs.spb.ru>
 * Copyright (C) 2012-2013 by Hoernchen <la@tfc-server.de>
 * Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
 *
 * When developing this program,
 * the author was inspired by the following great works:
 *	https://github.com/SDRplay/RSPTCPServer
 *	https://github.com/librtlsdr/librtlsdr
 *	https://github.com/zefie/hackrf
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

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
	#include <arpa/inet.h>
	#include <fcntl.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <unistd.h>
#else
	#define HAVE_STRUCT_TIMESPEC
#endif

#include <pthread.h>

#ifndef bool
typedef int bool;
	#define true 1
	#define false 0
#endif

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
	#define closesocket  close
	#define SOCKADDR     struct sockaddr
	#define SOCKET       int
	#define SOCKET_ERROR -1
#endif

#define DEFAULT_FREQ_HZ         (100000000)
#define DEFAULT_SAMPLE_RATE_HZ  (2048000)
#define DEFAULT_MAX_NUM_BUFFERS (500)

#define HACKRF_TCP_VERSION_MAJOR (0)
#define HACKRF_TCP_VERSION_MINOR (4)

#define HACKRF_AMP_DEFAULT true
#define HACKRF_LNA_DEFAULT 2
#define HACKRF_LNA_STEP    8
#define HACKRF_VGA_DEFAULT 8
#define HACKRF_VGA_STEP    2
#define RTLSDR_TUNER_R820T 5
#define WORKER_TIMEOUT_SEC 1

#define HACKRF_INDEX_GAIN_STEPS 37 // Count of elements in arrays hackrf_*_gains
const uint8_t hackrf_lna_gains[] = {0,  0,  0,  0,  0,  0,  8,  8,  8,  8,  8,  8,  16,
				    16, 16, 16, 16, 16, 24, 24, 24, 24, 24, 24, 32, 32,
				    32, 32, 32, 32, 40, 40, 40, 40, 40, 40, 40};
const uint8_t hackrf_vga_gains[] = {0,  2,  4,  6,  8,  10, 10, 12, 14, 16, 18, 20, 20,
				    22, 24, 26, 28, 30, 30, 32, 34, 36, 38, 40, 40, 42,
				    44, 46, 48, 50, 50, 52, 54, 56, 58, 60, 62};

#define RTL_TCP_COMMAND_BASE         0x00
#define HACKRF_TCP_COMMAND_BASE      0xb0
#define HACKRF_EXTENDED_INFO_MAGIC   ("HRF0")
#define HACKRF_EXTENDED_INFO_VERSION (0x00000001)

typedef enum {
	RTL_TCP_COMMAND_SET_FREQ = RTL_TCP_COMMAND_BASE + 0x01,
	RTL_TCP_COMMAND_SET_FREQ_HIGH32 = RTL_TCP_COMMAND_BASE + 0x56,
	RTL_TCP_COMMAND_SET_SAMPLE_RATE = RTL_TCP_COMMAND_BASE + 0x02,
	RTL_TCP_COMMAND_SET_TUNER_GAIN_MODE = RTL_TCP_COMMAND_BASE + 0x03,
	RTL_TCP_COMMAND_SET_GAIN = RTL_TCP_COMMAND_BASE + 0x04,
	RTL_TCP_COMMAND_SET_FREQ_COR = RTL_TCP_COMMAND_BASE + 0x05,
	RTL_TCP_COMMAND_SET_IF_STAGE_GAIN = RTL_TCP_COMMAND_BASE + 0x06,
	RTL_TCP_COMMAND_SET_TEST_MODE = RTL_TCP_COMMAND_BASE + 0x07,
	RTL_TCP_COMMAND_SET_AGC_MODE = RTL_TCP_COMMAND_BASE + 0x08,
	RTL_TCP_COMMAND_SET_DIRECT_SAMPLING = RTL_TCP_COMMAND_BASE + 0x09,
	RTL_TCP_COMMAND_SET_OFFSET_TUNING = RTL_TCP_COMMAND_BASE + 0x0a,
	RTL_TCP_COMMAND_SET_RTL_XTAL = RTL_TCP_COMMAND_BASE + 0x0b,
	RTL_TCP_COMMAND_SET_TUNER_XTAL = RTL_TCP_COMMAND_BASE + 0x0c,
	RTL_TCP_COMMAND_SET_TUNER_GAIN_INDEX = RTL_TCP_COMMAND_BASE + 0x0d,
	RTL_TCP_COMMAND_SET_BIAS_TEE = RTL_TCP_COMMAND_BASE + 0x0e,
	HACKRF_TCP_COMMAND_SET_AMP = HACKRF_TCP_COMMAND_BASE + 0x00,
	HACKRF_TCP_COMMAND_SET_LNA_GAIN = HACKRF_TCP_COMMAND_BASE + 1,
	HACKRF_TCP_COMMAND_SET_VGA_GAIN = HACKRF_TCP_COMMAND_BASE + 2,
} rtl_tcp_commands_t;

typedef enum {
	SDR_CAPABILITY_NONE = 0,
	SDR_CAPABILITY_BIAS_T = (1 << 0),
	SDR_CAPABILITY_REF_OUT = (1 << 1),
	SDR_CAPABILITY_REF_IN = (1 << 2),
} sdr_capabilities;

typedef enum {
	SDR_SAMPLE_FORMAT_UINT8 = 0x01,
	SDR_SAMPLE_FORMAT_INT16 = 0x02,
	SDR_SAMPLE_FORMAT_INT8 = 0x03,
} sdr_sample_format_t;

typedef struct { /* structure size must be multiple of 2 bytes */
	char magic[4];
	uint32_t tuner_type;
	uint32_t tuner_gain_count;
} dongle_info_t;

#ifdef _WIN32
	#define __attribute__(x)
	#pragma pack(push, 1)
#endif
typedef struct {
	char magic[4];           // HACKRF_CAPABILITIES_MAGIC: "HRF0"
	uint32_t version;        // HACKRF_EXTENDED_INFO_VERSION: Struct version
	uint32_t capabilities;   // Capabilities bitmap
	uint32_t __reserved__;   // Reserved for future use
	uint32_t board_id;       // Board ID
	uint32_t sample_format;  // Always 1 (8-bit)
	char extended_data[512]; // Space for additional data about SDR
} __attribute__((packed)) hackrf_extended_info_t;
#ifdef _WIN32
	#pragma pack(pop)
#endif

static SOCKET s;
static pthread_t tcp_worker_thread;
static pthread_t command_thread;
static pthread_mutex_t ll_mutex;
static pthread_cond_t cond;

struct llist {
	char* data;
	int len;
	struct llist* next;
};

static hackrf_device* device = NULL;
static char device_serial_number[32 + 1];
static uint8_t device_board_id = BOARD_ID_INVALID;
static int global_numq = 0;
static struct llist* ll_buffers = 0;
static uint32_t llbuf_num = DEFAULT_MAX_NUM_BUFFERS;
static sdr_sample_format_t sample_format = SDR_SAMPLE_FORMAT_UINT8;

static volatile bool do_exit = false;
static bool enable_verbose = false;
static bool enable_extended = false;

double atofs(const char* s)
{
	/* 100.5M --> 100500000,
	   2.048m --> 2048000 */
	double lf;
	char c = '\0';
	sscanf(s, "%lf%c", &lf, &c);
	switch (c) {
	case 'g':
	case 'G':
		lf *= 1e9;
		break;
	case 'm':
	case 'M':
		lf *= 1e6;
		break;
	case 'k':
	case 'K':
		lf *= 1e3;
		break;
	}
	return lf;
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

#ifdef _WIN32
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
	#ifdef _MSC_VER
		tmp -= 11644473600000000Ui64;
	#else
		tmp -= 11644473600000000ULL;
	#endif
		tv->tv_sec = (long) (tmp / 1000000UL);
		tv->tv_usec = (long) (tmp % 1000000UL);
	}
	return 0;
}

BOOL WINAPI sighandler(int signum)
{
	if (signum == CTRL_C_EVENT) {
		fprintf(stderr,
			"Signal (%d) caught, ask for exit (%d)!\n",
			signum,
			do_exit);
		if (do_exit) {
			return TRUE;
		}

		do_exit = true;
		pthread_cond_signal(&cond);
		hackrf_stop_rx(device);
		hackrf_close(device);
		//sleep(1);
		hackrf_init();
		hackrf_open_by_serial(device_serial_number, &device);
		return TRUE;
	}
	return FALSE;
}
#else
static void sighandler(int signum)
{
	fprintf(stderr, "Signal (%d) caught, ask for exit (%d)!\n", signum, do_exit);
	if (do_exit) {
		return;
	}

	do_exit = true;
	pthread_cond_signal(&cond);
	hackrf_stop_rx(device);
	hackrf_close(device);
	sleep(1);
	hackrf_init();
	hackrf_open_by_serial(device_serial_number, &device);
}
#endif

int rx_callback(hackrf_transfer* transfer)
{
	if (do_exit) {
		return HACKRF_SUCCESS;
	}

	struct llist* rpt = (struct llist*) malloc(sizeof(struct llist));
	rpt->data = (char*) malloc(transfer->buffer_length);
	memcpy(rpt->data, transfer->buffer, transfer->buffer_length);
	rpt->len = transfer->buffer_length;
	rpt->next = NULL;

	if (sample_format == SDR_SAMPLE_FORMAT_UINT8) {
		// HackRF One returns signed IQ values, convert them to unsigned
		// This workaround greatly increases CPU usage
		int i;
		for (i = 0; i < rpt->len; i++) {
			rpt->data[i] ^= (uint8_t) 0x80;
		}
	}

	pthread_mutex_lock(&ll_mutex);

	if (ll_buffers == NULL) {
		ll_buffers = rpt;
	} else {
		struct llist* cur = ll_buffers;
		int num_queued = 0;

		while (cur->next != NULL) {
			cur = cur->next;
			num_queued++;
		}
		cur->next = rpt;

		if (llbuf_num && llbuf_num == num_queued - 2) {
			struct llist* curelem;

			free(ll_buffers->data);
			curelem = ll_buffers->next;
			free(ll_buffers);
			ll_buffers = curelem;
		}

		if (enable_verbose) {
			if (num_queued > global_numq)
				printf("ll+, now %d\n", num_queued);
			else if (num_queued < global_numq)
				printf("ll-, now %d\n", num_queued);
			global_numq = num_queued;
		}
	}

	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&ll_mutex);

	return HACKRF_SUCCESS;
}

static void* tcp_worker(void* arg)
{
	struct timeval tp;
	struct timeval tv = {1, 0};
	struct timespec ts;
	struct llist *curelem, *prev;
	fd_set writefds;
	int bytesleft, bytessent, index;

	for (;;) {
		if (do_exit) {
			pthread_exit(0);
		}

		pthread_mutex_lock(&ll_mutex);
		gettimeofday(&tp, NULL);
		ts.tv_sec = tp.tv_sec + WORKER_TIMEOUT_SEC;
		ts.tv_nsec = tp.tv_usec * 1000;
		if (pthread_cond_timedwait(&cond, &ll_mutex, &ts) == ETIMEDOUT) {
			pthread_mutex_unlock(&ll_mutex);
			printf("worker cond timeout\n");
			sighandler(0);
			pthread_exit(NULL);
		}

		curelem = ll_buffers;
		ll_buffers = 0;
		pthread_mutex_unlock(&ll_mutex);

		while (curelem != 0) {
			bytesleft = curelem->len;
			index = 0;
			while (bytesleft > 0) {
				FD_ZERO(&writefds);
				FD_SET(s, &writefds);
				tv.tv_sec = 1;
				tv.tv_usec = 0;
				if (select(s + 1, NULL, &writefds, NULL, &tv)) {
					bytessent =
						send(s,
						     &curelem->data[index],
						     bytesleft,
						     0);
					if (bytessent == SOCKET_ERROR || do_exit) {
						printf("worker socket bye, do_exit=%d\n",
						       do_exit);
						sighandler(0);
						pthread_exit(NULL);
					} else {
						bytesleft -= bytessent;
						index += bytessent;
					}
				}
			}
			prev = curelem;
			curelem = curelem->next;
			free(prev->data);
			free(prev);
		}
	}
}

static int hackrf_set_gain_by_index(hackrf_device* _dev, const uint32_t value)
{
	int index = value;
	int result = HACKRF_SUCCESS;

	if (index < 0)
		index = 0;
	if (index >= HACKRF_INDEX_GAIN_STEPS)
		index = HACKRF_INDEX_GAIN_STEPS - 1;

	if (enable_verbose) {
		printf("set tuner lna gain %d\n", hackrf_lna_gains[index]);
		printf("set tuner vga gain %d\n", hackrf_vga_gains[index]);
	}

	result |= hackrf_set_lna_gain(_dev, hackrf_lna_gains[index]);
	result |= hackrf_set_vga_gain(_dev, hackrf_vga_gains[index]);
	return result;
}

#ifdef _WIN32
	#define __attribute__(x)
	#pragma pack(push, 1)
#endif
struct command {
	unsigned char cmd;
	unsigned int param;
} __attribute__((packed));
#ifdef _WIN32
	#pragma pack(pop)
#endif

static void* command_worker(void* arg)
{
	int bytesleft, bytesrecv = 0;
	struct command cmd = {0, 0};
	struct timeval tv = {1, 0};
	fd_set readfds;
	uint32_t param;

	for (;;) {
		bytesleft = sizeof(cmd);
		while (bytesleft > 0) {
			FD_ZERO(&readfds);
			FD_SET(s, &readfds);
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			if (select(s + 1, &readfds, NULL, NULL, &tv)) {
				bytesrecv =
					recv(s,
					     (char*) &cmd + (sizeof(cmd) - bytesleft),
					     bytesleft,
					     0);
				if (bytesrecv == SOCKET_ERROR || do_exit) {
					printf("command recv bye, do_exit=%d\n", do_exit);
					sighandler(0);
					pthread_exit(NULL);
				} else {
					bytesleft -= bytesrecv;
				}
			}
		}

		param = ntohl(cmd.param);
		switch (cmd.cmd) {
		case RTL_TCP_COMMAND_SET_FREQ:
			printf("set freq %d\n", param);
			hackrf_set_freq(device, param);
			break;
		case RTL_TCP_COMMAND_SET_FREQ_HIGH32:
			printf("set freq high32 %jd\n", 0x100000000 + param);
			hackrf_set_freq(device, 0x100000000 + param);
			break;
		case RTL_TCP_COMMAND_SET_SAMPLE_RATE:
			printf("set sample rate %d\n", param);
			hackrf_set_sample_rate(device, param);
			break;
		case RTL_TCP_COMMAND_SET_TUNER_GAIN_MODE:
			printf("[ignored] set amp mode %d\n", param);
			//rtlsdr_set_tuner_gain_mode(device, param);
			break;
		case RTL_TCP_COMMAND_SET_GAIN:
			printf("[ignored] set gain %d\n", param);
			//rtlsdr_set_tuner_gain(device, param);
			break;
		case RTL_TCP_COMMAND_SET_FREQ_COR:
			printf("[ignored] set freq correction %d\n", param);
			//rtlsdr_set_freq_correction(device, param);
			break;
		case RTL_TCP_COMMAND_SET_IF_STAGE_GAIN:
			printf("[ignored] set if stage %d gain %d\n",
			       param >> 16,
			       (short) (param & 0xffff));
			//rtlsdr_set_tuner_if_gain(device,
			//                         param >> 16, (short) (param & 0xffff));
			break;
		case RTL_TCP_COMMAND_SET_TEST_MODE:
			printf("[ignored] set test mode %d\n", param);
			//rtlsdr_set_testmode(device, param);
			break;
		case RTL_TCP_COMMAND_SET_AGC_MODE:
			printf("set agc mode %d\n", param);
			hackrf_set_amp_enable(device, param);
			break;
		case RTL_TCP_COMMAND_SET_DIRECT_SAMPLING:
			printf("[ignored] set direct sampling %d\n", param);
			//rtlsdr_set_direct_sampling(device, param);
			break;
		case RTL_TCP_COMMAND_SET_OFFSET_TUNING:
			printf("[ignored] set offset tuning %d\n", param);
			//rtlsdr_set_offset_tuning(device, param);
			break;
		case RTL_TCP_COMMAND_SET_RTL_XTAL:
			printf("[ignored] set rtl xtal %d\n", param);
			//rtlsdr_set_xtal_freq(device, param, 0);
			break;
		case RTL_TCP_COMMAND_SET_TUNER_XTAL:
			printf("[ignored] set tuner xtal %d\n", param);
			//rtlsdr_set_xtal_freq(device, 0, param);
			break;
		case RTL_TCP_COMMAND_SET_TUNER_GAIN_INDEX:
			printf("set tuner gain by index %d\n", param);
			hackrf_set_gain_by_index(device, param);
			break;
		case RTL_TCP_COMMAND_SET_BIAS_TEE:
			printf("set bias tee %d\n", param);
			hackrf_set_antenna_enable(device, param);
			break;
		case HACKRF_TCP_COMMAND_SET_AMP:
			printf("set tuner amp %d\n", param);
			hackrf_set_amp_enable(device, param);
			break;
		case HACKRF_TCP_COMMAND_SET_LNA_GAIN:
			printf("set tuner LNA gain %d\n", param);
			hackrf_set_lna_gain(device, param);
			break;
		case HACKRF_TCP_COMMAND_SET_VGA_GAIN:
			printf("set tuner VGA gain %d\n", param);
			hackrf_set_vga_gain(device, param);
			break;
		default:
			printf("[ignored] unknown command 0x%02x (%d), value %d\n",
			       cmd.cmd,
			       cmd.cmd,
			       param);
			break;
		}
		cmd.cmd = 0xff;
	}
}

int verbose_device_search(char* id)
{
	hackrf_device_list_t* list;
	hackrf_device* device;
	int result;
	uint8_t board_id = BOARD_ID_INVALID;
	char version[255 + 1];
	uint16_t usb_version;
	read_partid_serialno_t read_partid_serialno;
	int device_index = -1, device_index_ok = -1;
	int i;

	list = hackrf_device_list();
	if (list->devicecount < 1) {
		fprintf(stderr, "No supported devices found.\n");
		hackrf_device_list_free(list);
		return HACKRF_ERROR_OTHER;
	}

	printf("Found %d device(s):\n", list->devicecount);
	for (i = 0; i < list->devicecount; i++) {
		device = NULL;

		if (list->serial_numbers[i]) {
			printf("  %d:\tSerial: %s\n", i, list->serial_numbers[i]);
			if (strtol(list->serial_numbers[i], NULL, 16) ==
			    strtol(id, NULL, 16)) {
				// Save the device index, with the device
				// serial number specified by the user
				device_index = i;
			}
		}

		result = hackrf_device_list_open(list, i, &device);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_device_list_open() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			continue;
		}

		result = hackrf_board_id_read(device, &board_id);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_board_id_read() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			continue;
		}
		printf("\tBoard ID Number: %d (%s)",
		       board_id,
		       hackrf_board_id_name(board_id));
		result = hackrf_si5351c_read(device, 0, &usb_version);
		if (result == HACKRF_SUCCESS) {
			if (usb_version == 0x01) {
				printf(", TCXO");
			} else if (usb_version == 0x51) {
				printf(", no TCXO");
			} else {
				printf(", TCXO error [0x%02X]", usb_version);
			}
		}
		printf("\n");

		result = hackrf_version_string_read(device, &version[0], 255);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_version_string_read() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			continue;
		}
		result = hackrf_usb_api_version_read(device, &usb_version);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_usb_api_version_read() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			continue;
		}
		printf("\tFirmware Version: %s (API:%x.%02x)\n",
		       version,
		       (usb_version >> 8) & 0xFF,
		       usb_version & 0xFF);

		result = hackrf_board_partid_serialno_read(device, &read_partid_serialno);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_board_partid_serialno_read() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			continue;
		}
		printf("\tPart ID Number: 0x%08x 0x%08x\n",
		       read_partid_serialno.part_id[0],
		       read_partid_serialno.part_id[1]);

		result = hackrf_close(device);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_close() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			continue;
		}

		if (device_index == -1 && i == atoi(id)) {
			// Save the device index equal to the index specified by the user
			device_index = i;
		}
		if (device_index_ok == -1) {
			// Save the index of the first working device
			device_index_ok = i;
		}
	}

	if (device_index == -1 && id != NULL && id[0] == '\0') {
		// The user did not specify any id, so open the first working device
		device_index = device_index_ok;
	}
	if (device_index != -1) {
		strncpy(device_serial_number, list->serial_numbers[device_index], 32);
		result = HACKRF_SUCCESS;
	} else {
		fprintf(stderr, "Could not found device with id %s.\n", id);
		result = HACKRF_ERROR_OTHER;
	}

	hackrf_device_list_free(list);

	return result;
}

static void usage(void)
{
	fprintf(stderr,
		"hackrf_tcp, an I/Q spectrum server for HackRF receivers\n\n"
		"Usage:\t[-a listen address (default: 0.0.0.0)]\n"
		"\t[-p listen port (default: 1234)]\n"
		"\t[-d device index or serial number (default: first found)]\n"
		"\t[-f frequency to tune to (100.5M, 100500000)]\n"
		"\t[-s samplerate in Hz (default: 2048000 Hz)]\n"
		"\t[-r amp_enable, RF amplifier, 1=Enable, 0=Disable (default: 0)]\n"
		"\t[-l lna_gain, LNA (IF) gain, 0-40dB, 8dB steps (default: 16)]\n"
		"\t[-g vga_gain, VGA (BB) gain, 0-62dB, 2dB steps (default: 16)]\n"
		"\t[-n max number of linked list buffers to keep (default: 500)]\n"
		//"\t[-P ppm_error (default: 0)]\n"
		"\t[-T enable bias-T on antenna port (default: disabled)]\n"
		"\t[-v enable verbose mode (default: disabled)]\n"
		"\t[-E enable extended mode (default: disabled)]\n");
}

int main(int argc, char** argv)
{
	int result, opt;
	char* addr = "0.0.0.0";
	char* dev_id = "";
	uint32_t port = 1234;
	uint64_t frequency = DEFAULT_FREQ_HZ;
	double samplerate = DEFAULT_SAMPLE_RATE_HZ;
	bool amp = HACKRF_AMP_DEFAULT;
	uint32_t amp_enable = 0;
	uint32_t lna_gain = HACKRF_LNA_DEFAULT * HACKRF_LNA_STEP;
	uint32_t vga_gain = HACKRF_VGA_DEFAULT * HACKRF_VGA_STEP;
	//int ppm_error = 0;
	bool enable_biastee = false;
	char version[255 + 1];
	struct sockaddr_in local, remote;
	SOCKET listensocket;
	struct linger ling = {1, 0};
	struct timeval tv = {1, 0};
	fd_set readfds;
	socklen_t rlen;
	dongle_info_t dongle_info;
	pthread_attr_t attr;
	void* status;
	struct llist *curelem, *prev;
#ifdef _WIN32
	unsigned int i;
	WSADATA wsd;
	i = WSAStartup(MAKEWORD(2, 2), &wsd);
#else
	struct sigaction sigact, sigign;
#endif

	result = hackrf_init();
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_init() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		return EXIT_FAILURE;
	}

	printf("hackrf_tcp version: %d.%d\n",
	       HACKRF_TCP_VERSION_MAJOR,
	       HACKRF_TCP_VERSION_MINOR);
	printf("libhackrf version: %s (%s)\n",
	       hackrf_library_release(),
	       hackrf_library_version());

	while ((opt = getopt(argc, argv, "a:p:d:f:s:r:l:g:n:P:TvE")) != -1) {
		switch (opt) {
		case 'a':
			addr = optarg;
			break;
		case 'p':
			result = parse_u32(optarg, &port);
			break;
		case 'd':
			dev_id = optarg;
			break;
		case 'f':
			frequency = (uint64_t) atofs(optarg);
			break;
		case 's':
			samplerate = atofs(optarg);
			break;
		case 'r':
			amp = true;
			result = parse_u32(optarg, &amp_enable);
			break;
		case 'l':
			result = parse_u32(optarg, &lna_gain);
			break;
		case 'g':
			result = parse_u32(optarg, &vga_gain);
			break;
		case 'n':
			result = parse_u32(optarg, &llbuf_num);
			break;
		//case 'P':
		//	ppm_error = atoi(optarg);
		//	break;
		case 'T':
			enable_biastee = true;
			break;
		case 'v':
			enable_verbose = true;
			break;
		case 'E':
			enable_extended = true;
			sample_format = SDR_SAMPLE_FORMAT_INT8;
			break;
		default:
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

	if (amp) {
		if (amp_enable > 1) {
			fprintf(stderr, "argument error: amp_enable shall be 0 or 1.\n");
			usage();
			return EXIT_FAILURE;
		}
	}

	if (lna_gain % 8) {
		fprintf(stderr, "warning: lna_gain (-l) must be a multiple of 8\n");
	}

	if (vga_gain % 2) {
		fprintf(stderr, "warning: vga_gain (-g) must be a multiple of 2\n");
	}

	result = verbose_device_search(dev_id);
	if (result != HACKRF_SUCCESS) {
		return EXIT_FAILURE;
	}

	result = hackrf_open_by_serial(device_serial_number, &device);
	if (result != HACKRF_SUCCESS || device == NULL) {
		fprintf(stderr,
			"hackrf_open() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		return EXIT_FAILURE;
	}

	result = hackrf_board_id_read(device, &device_board_id);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_board_id_read() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		return EXIT_FAILURE;
	}
	result = hackrf_version_string_read(device, &version[0], 255);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr,
			"hackrf_version_string_read() failed: %s (%d)\n",
			hackrf_error_name(result),
			result);
		return EXIT_FAILURE;
	}
	printf("Using device:\n\tSerial: %s\n\t%s with firmware %s\n",
	       device_serial_number,
	       hackrf_board_id_name(device_board_id),
	       version);

#ifndef _WIN32
	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigign.sa_handler = SIG_IGN;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGPIPE, &sigign, NULL);
#else
	SetConsoleCtrlHandler((PHANDLER_ROUTINE) sighandler, TRUE);
#endif

	pthread_mutex_init(&ll_mutex, NULL);
	pthread_cond_init(&cond, NULL);

	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	//local.sin_addr.s_addr = inet_addr(addr);
	inet_pton(AF_INET, addr, (void*) &local.sin_addr.s_addr);

	listensocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	result = 1;
	setsockopt(listensocket, SOL_SOCKET, SO_REUSEADDR, (char*) &result, sizeof(int));
	setsockopt(listensocket, SOL_SOCKET, SO_LINGER, (char*) &ling, sizeof(ling));
	bind(listensocket, (struct sockaddr*) &local, sizeof(local));

#ifdef _WIN32
	opt = 1;
	ioctlsocket(listensocket, FIONBIO, &opt);
#else
	result = fcntl(listensocket, F_GETFL, 0);
	result = fcntl(listensocket, F_SETFL, result | O_NONBLOCK);
#endif

	for (;;) {
		printf("\nlistening...\n");
		// Set the tuner error
		//verbose_ppm_set(device, ppm_error);

		// Set the sample rate
		if (enable_verbose) {
			fprintf(stderr,
				"initial call hackrf_sample_rate_set(%.0f)\n",
				samplerate);
		}
		result = hackrf_set_sample_rate(device, samplerate);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_set_sample_rate() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			return EXIT_FAILURE;
		}

		// Set the amplifier (RF) enable/disable
		if (amp) {
			if (enable_verbose) {
				fprintf(stderr,
					"initial call hackrf_set_amp_enable(%u)\n",
					amp_enable);
			}
			result = hackrf_set_amp_enable(device, (uint8_t) amp_enable);
			if (result != HACKRF_SUCCESS) {
				fprintf(stderr,
					"hackrf_set_amp_enable() failed: %s (%d)\n",
					hackrf_error_name(result),
					result);
			}
		}

		// Set the LNA (IF) gain
		if (enable_verbose) {
			fprintf(stderr,
				"initial call hackrf_set_lna_gain(%u)\n",
				lna_gain);
		}
		result = hackrf_set_lna_gain(device, lna_gain);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_set_lna_gain() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
		}

		// Set the VGA (baseband, BB) gain
		if (enable_verbose) {
			fprintf(stderr,
				"initial call hackrf_set_vga_gain(%u)\n",
				vga_gain);
		}
		result = hackrf_set_vga_gain(device, vga_gain);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_set_vga_gain() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
		}

		// Set the frequency
		fprintf(stderr, "initial call hackrf_set_freq(%" PRIu64 ")\n", frequency);
		result = hackrf_set_freq(device, frequency);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_set_freq() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			return EXIT_FAILURE;
		}

		// Set the antenna power control
		if (enable_biastee) {
			fprintf(stderr, "initial activating bias-T on antenna port\n");
			result = hackrf_set_antenna_enable(device, enable_biastee);
			if (result != HACKRF_SUCCESS) {
				fprintf(stderr,
					"hackrf_set_antenna_enable() failed: %s (%d)\n",
					hackrf_error_name(result),
					result);
			}
		}

		printf("Use the device argument 'rtl_tcp=%s:%d' in OsmoSDR "
		       "(gr-osmosdr) source\n"
		       "to receive samples in GRC and control "
		       "rtl_tcp parameters (frequency, gain, ...).\n",
		       addr,
		       port);
		listen(listensocket, 1);

		for (;;) {
			FD_ZERO(&readfds);
			FD_SET(listensocket, &readfds);
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			result = select(listensocket + 1, &readfds, NULL, NULL, &tv);
			if (do_exit) {
				goto out;
			}
			if (result) {
				rlen = sizeof(remote);
				s =
					accept(listensocket,
					       (struct sockaddr*) &remote,
					       &rlen);
				break;
			}
		}

		setsockopt(s, SOL_SOCKET, SO_LINGER, (char*) &ling, sizeof(ling));

		printf("client accepted!\n");

		memset(&dongle_info, 0, sizeof(dongle_info));
		memcpy(&dongle_info.magic, "RTL0", 4);
		dongle_info.tuner_type = htonl(RTLSDR_TUNER_R820T);
		dongle_info.tuner_gain_count = htonl(HACKRF_INDEX_GAIN_STEPS);

		result = send(s, (const char*) &dongle_info, sizeof(dongle_info), 0);
		if (result != sizeof(dongle_info)) {
			printf("failed to send dongle information\n");
		}

		if (enable_extended) {
			printf("sending HackRF extended capabilities structure\n");

			hackrf_extended_info_t hackrf_ext_info;
			memset(&hackrf_ext_info, 0, sizeof(hackrf_extended_info_t));
			memcpy(&hackrf_ext_info.magic, HACKRF_EXTENDED_INFO_MAGIC, 4);
			hackrf_ext_info.version = htonl(HACKRF_EXTENDED_INFO_VERSION);
			hackrf_ext_info.board_id = htonl(device_board_id);
			hackrf_ext_info.capabilities = SDR_CAPABILITY_NONE;
			if (device_board_id == BOARD_ID_HACKRF_ONE)
				hackrf_ext_info.capabilities = SDR_CAPABILITY_BIAS_T +
					(SDR_CAPABILITY_REF_IN + SDR_CAPABILITY_REF_OUT);
			hackrf_ext_info.capabilities =
				htonl(hackrf_ext_info.capabilities);
			hackrf_ext_info.sample_format = htonl(sample_format);

			result =
				send(s,
				     (const char*) &hackrf_ext_info,
				     sizeof(hackrf_ext_info),
				     0);
			if (result != sizeof(hackrf_ext_info)) {
				printf("failed to send HackRF "
				       "extended capabilities information\n");
			}
		}

		// must start the tcp_worker before the first samples are available from the
		// rx because the rx_callback tries to send a condition to the worker thread
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		result = pthread_create(&tcp_worker_thread, &attr, tcp_worker, NULL);
		if (result != 0) {
			printf("failed to create tcp worker thread\n");
			break;
		}
		// the rx must be started before accepting commands from the command worker
		result = pthread_create(&command_thread, &attr, command_worker, NULL);
		if (result != 0) {
			printf("failed to create command thread\n");
			break;
		}
		pthread_attr_destroy(&attr);

		result = hackrf_start_rx(device, rx_callback, NULL);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr,
				"hackrf_start_rx() failed: %s (%d)\n",
				hackrf_error_name(result),
				result);
			break;
		}

		// wait for the workers to exit
		pthread_join(tcp_worker_thread, &status);
		pthread_join(command_thread, &status);

		closesocket(s);

		printf("all threads dead..\n");
		curelem = ll_buffers;
		ll_buffers = 0;

		while (curelem != 0) {
			prev = curelem;
			curelem = curelem->next;
			free(prev->data);
			free(prev);
		}

		do_exit = false;
		global_numq = 0;
	}

out:
	hackrf_close(device);
	hackrf_exit();

	closesocket(listensocket);
	closesocket(s);
#ifdef _WIN32
	WSACleanup();
#endif
	printf("bye!\n");

	return result >= 0 ? result : -result;
}
