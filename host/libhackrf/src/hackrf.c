/*
Copyright (c) 2012-2022 Great Scott Gadgets <info@greatscottgadgets.com>
Copyright (c) 2012, Jared Boone <jared@sharebrained.com>
Copyright (c) 2013, Benjamin Vernoux <titanmkd@gmail.com>

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the 
	documentation and/or other materials provided with the distribution.
    Neither the name of Great Scott Gadgets nor the names of its contributors may be used to endorse or promote products derived from this software
	without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "hackrf.h"

#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
	#include <unistd.h>
	#include <signal.h>
#endif
#include <libusb.h>

#ifdef _WIN32
	/* Avoid redefinition of timespec from time.h (included by libusb.h) */
	#define HAVE_STRUCT_TIMESPEC 1
	#define strdup               _strdup
#endif
#include <pthread.h>

#ifndef bool
typedef int bool;
	#define true 1
	#define false 0
#endif

#ifdef HACKRF_BIG_ENDIAN
	#define TO_LE(x)     __builtin_bswap32(x)
	#define TO_LE64(x)   __builtin_bswap64(x)
	#define FROM_LE16(x) __builtin_bswap16(x)
	#define FROM_LE32(x) __builtin_bswap32(x)
#else
	#define TO_LE(x)     x
	#define TO_LE64(x)   x
	#define FROM_LE16(x) x
	#define FROM_LE32(x) x
#endif

// TODO: Factor this into a shared #include so that firmware can use
// the same values.
typedef enum {
	HACKRF_VENDOR_REQUEST_SET_TRANSCEIVER_MODE = 1,
	HACKRF_VENDOR_REQUEST_MAX2837_WRITE = 2,
	HACKRF_VENDOR_REQUEST_MAX2837_READ = 3,
	HACKRF_VENDOR_REQUEST_SI5351C_WRITE = 4,
	HACKRF_VENDOR_REQUEST_SI5351C_READ = 5,
	HACKRF_VENDOR_REQUEST_SAMPLE_RATE_SET = 6,
	HACKRF_VENDOR_REQUEST_BASEBAND_FILTER_BANDWIDTH_SET = 7,
	HACKRF_VENDOR_REQUEST_RFFC5071_WRITE = 8,
	HACKRF_VENDOR_REQUEST_RFFC5071_READ = 9,
	HACKRF_VENDOR_REQUEST_SPIFLASH_ERASE = 10,
	HACKRF_VENDOR_REQUEST_SPIFLASH_WRITE = 11,
	HACKRF_VENDOR_REQUEST_SPIFLASH_READ = 12,
	HACKRF_VENDOR_REQUEST_BOARD_ID_READ = 14,
	HACKRF_VENDOR_REQUEST_VERSION_STRING_READ = 15,
	HACKRF_VENDOR_REQUEST_SET_FREQ = 16,
	HACKRF_VENDOR_REQUEST_AMP_ENABLE = 17,
	HACKRF_VENDOR_REQUEST_BOARD_PARTID_SERIALNO_READ = 18,
	HACKRF_VENDOR_REQUEST_SET_LNA_GAIN = 19,
	HACKRF_VENDOR_REQUEST_SET_VGA_GAIN = 20,
	HACKRF_VENDOR_REQUEST_SET_TXVGA_GAIN = 21,
	HACKRF_VENDOR_REQUEST_ANTENNA_ENABLE = 23,
	HACKRF_VENDOR_REQUEST_SET_FREQ_EXPLICIT = 24,
	HACKRF_VENDOR_REQUEST_USB_WCID_VENDOR_REQ = 25,
	HACKRF_VENDOR_REQUEST_INIT_SWEEP = 26,
	HACKRF_VENDOR_REQUEST_OPERACAKE_GET_BOARDS = 27,
	HACKRF_VENDOR_REQUEST_OPERACAKE_SET_PORTS = 28,
	HACKRF_VENDOR_REQUEST_SET_HW_SYNC_MODE = 29,
	HACKRF_VENDOR_REQUEST_RESET = 30,
	HACKRF_VENDOR_REQUEST_OPERACAKE_SET_RANGES = 31,
	HACKRF_VENDOR_REQUEST_CLKOUT_ENABLE = 32,
	HACKRF_VENDOR_REQUEST_SPIFLASH_STATUS = 33,
	HACKRF_VENDOR_REQUEST_SPIFLASH_CLEAR_STATUS = 34,
	HACKRF_VENDOR_REQUEST_OPERACAKE_GPIO_TEST = 35,
	HACKRF_VENDOR_REQUEST_CPLD_CHECKSUM = 36,
	HACKRF_VENDOR_REQUEST_UI_ENABLE = 37,
	HACKRF_VENDOR_REQUEST_OPERACAKE_SET_MODE = 38,
	HACKRF_VENDOR_REQUEST_OPERACAKE_GET_MODE = 39,
	HACKRF_VENDOR_REQUEST_OPERACAKE_SET_DWELL_TIMES = 40,
	HACKRF_VENDOR_REQUEST_GET_M0_STATE = 41,
	HACKRF_VENDOR_REQUEST_SET_TX_UNDERRUN_LIMIT = 42,
	HACKRF_VENDOR_REQUEST_SET_RX_OVERRUN_LIMIT = 43,
	HACKRF_VENDOR_REQUEST_GET_CLKIN_STATUS = 44,
	HACKRF_VENDOR_REQUEST_BOARD_REV_READ = 45,
	HACKRF_VENDOR_REQUEST_SUPPORTED_PLATFORM_READ = 46,
	HACKRF_VENDOR_REQUEST_SET_LEDS = 47,
	HACKRF_VENDOR_REQUEST_SET_USER_BIAS_T_OPTS = 48,
} hackrf_vendor_request;

#define USB_CONFIG_STANDARD 0x1

#define RX_ENDPOINT_ADDRESS (LIBUSB_ENDPOINT_IN | 1)
#define TX_ENDPOINT_ADDRESS (LIBUSB_ENDPOINT_OUT | 2)

typedef enum {
	HACKRF_TRANSCEIVER_MODE_OFF = 0,
	HACKRF_TRANSCEIVER_MODE_RECEIVE = 1,
	HACKRF_TRANSCEIVER_MODE_TRANSMIT = 2,
	HACKRF_TRANSCEIVER_MODE_SS = 3,
	TRANSCEIVER_MODE_CPLD_UPDATE = 4,
	TRANSCEIVER_MODE_RX_SWEEP = 5,
} hackrf_transceiver_mode;

typedef enum {
	HACKRF_HW_SYNC_MODE_OFF = 0,
	HACKRF_HW_SYNC_MODE_ON = 1,
} hackrf_hw_sync_mode;

#define TRANSFER_COUNT        4
#define TRANSFER_BUFFER_SIZE  262144
#define DEVICE_BUFFER_SIZE    32768
#define USB_MAX_SERIAL_LENGTH 32

struct hackrf_device {
	libusb_device_handle* usb_device;
	struct libusb_transfer** transfers;
	hackrf_sample_block_cb_fn callback;
	volatile bool
		transfer_thread_started; /* volatile shared between threads (read only) */
	pthread_t transfer_thread;
	volatile bool streaming; /* volatile shared between threads (read only) */
	void* rx_ctx;
	void* tx_ctx;
	volatile bool do_exit;
	unsigned char buffer[TRANSFER_COUNT * TRANSFER_BUFFER_SIZE];
	bool transfers_setup;           /* true if the USB transfers have been setup */
	pthread_mutex_t transfer_lock;  /* must be held to cancel or restart transfers */
	volatile int active_transfers;  /* number of active transfers */
	pthread_cond_t all_finished_cv; /* signalled when all transfers have finished */
	bool flush;
	struct libusb_transfer* flush_transfer;
	hackrf_flush_cb_fn flush_callback;
	hackrf_tx_block_complete_cb_fn tx_completion_callback;
	void* flush_ctx;
};

typedef struct {
	uint32_t bandwidth_hz;
} max2837_ft_t;

static const max2837_ft_t max2837_ft[] = {
	{1750000},
	{2500000},
	{3500000},
	{5000000},
	{5500000},
	{6000000},
	{7000000},
	{8000000},
	{9000000},
	{10000000},
	{12000000},
	{14000000},
	{15000000},
	{20000000},
	{24000000},
	{28000000},
	{0}};

#define USB_API_REQUIRED(device, version)                  \
	uint16_t usb_version = 0;                          \
	hackrf_usb_api_version_read(device, &usb_version); \
	if (usb_version < version)                         \
		return HACKRF_ERROR_USB_API_VERSION;

static const uint16_t hackrf_usb_vid = 0x1d50;
static const uint16_t hackrf_jawbreaker_usb_pid = 0x604b;
static const uint16_t hackrf_one_usb_pid = 0x6089;
static const uint16_t rad1o_usb_pid = 0xcc15;
static uint16_t open_devices = 0;

static int create_transfer_thread(hackrf_device* device);

static libusb_context* g_libusb_context = NULL;
int last_libusb_error = LIBUSB_SUCCESS;

/*
 * Check if the transfers are setup and owned by libusb.
 *
 * Returns true if the device transfers are currently setup
 * in libusb, false otherwise.
 */
static int transfers_check_setup(hackrf_device* device)
{
	if ((device->transfers != NULL) && (device->transfers_setup == true))
		return true;
	return false;
}

/*
 * Cancel any transfers that are in-flight.
 *
 * This cancels any transfers that hvae been given to libusb for
 * either transmit or receive.
 *
 * This must be done whilst the libusb thread is running, as
 * on some platforms cancelling transfers requires some work
 * to be done inside the libusb thread to completely cancel
 * pending transfers.
 *
 * Returns HACKRF_SUCCESS if OK, HACKRF_ERROR_OTHER if the
 * transfers aren't currently setup.
 */
static int cancel_transfers(hackrf_device* device)
{
	uint32_t transfer_index;

	// If we're cancelling transfers for any reason, we're shutting down.
	device->streaming = false;

	if (transfers_check_setup(device) == true) {
		// Take lock while cancelling transfers. This blocks the
		// transfer completion callback from restarting a transfer
		// while we're in the middle of trying to cancel them all.
		pthread_mutex_lock(&device->transfer_lock);

		for (transfer_index = 0; transfer_index < TRANSFER_COUNT;
		     transfer_index++) {
			if (device->transfers[transfer_index] != NULL) {
				libusb_cancel_transfer(device->transfers[transfer_index]);
			}
		}

		if (device->flush_transfer != NULL)
			libusb_cancel_transfer(device->flush_transfer);

		device->transfers_setup = false;
		device->flush = false;

		// Now wait for the transfer thread to signal that all transfers
		// have finished, either by completing or being fully cancelled.
		while (device->active_transfers > 0) {
			pthread_cond_wait(
				&device->all_finished_cv,
				&device->transfer_lock);
		}
		pthread_mutex_unlock(&device->transfer_lock);

		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_OTHER;
	}
}

static int free_transfers(hackrf_device* device)
{
	uint32_t transfer_index;

	if (device->transfers != NULL) {
		// libusb_close() should free all transfers referenced from this array.
		for (transfer_index = 0; transfer_index < TRANSFER_COUNT;
		     transfer_index++) {
			if (device->transfers[transfer_index] != NULL) {
				libusb_free_transfer(device->transfers[transfer_index]);
				device->transfers[transfer_index] = NULL;
			}
		}
		free(device->transfers);
		device->transfers = NULL;
	}

	libusb_free_transfer(device->flush_transfer);

	return HACKRF_SUCCESS;
}

static int allocate_transfers(hackrf_device* const device)
{
	if (device->transfers == NULL) {
		uint32_t transfer_index;
		device->transfers = (struct libusb_transfer**) calloc(
			TRANSFER_COUNT,
			sizeof(struct libusb_transfer*));
		if (device->transfers == NULL) {
			return HACKRF_ERROR_NO_MEM;
		}

		memset(device->buffer, 0, TRANSFER_COUNT * TRANSFER_BUFFER_SIZE);

		for (transfer_index = 0; transfer_index < TRANSFER_COUNT;
		     transfer_index++) {
			device->transfers[transfer_index] = libusb_alloc_transfer(0);
			if (device->transfers[transfer_index] == NULL) {
				return HACKRF_ERROR_LIBUSB;
			}

			libusb_fill_bulk_transfer(
				device->transfers[transfer_index],
				device->usb_device,
				0,
				&device->buffer[transfer_index * TRANSFER_BUFFER_SIZE],
				TRANSFER_BUFFER_SIZE,
				NULL,
				device,
				0);

			if (device->transfers[transfer_index]->buffer == NULL) {
				return HACKRF_ERROR_NO_MEM;
			}
		}
		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_BUSY;
	}
}

static int prepare_transfers(
	hackrf_device* device,
	const uint_fast8_t endpoint_address,
	libusb_transfer_cb_fn callback)
{
	int error;
	uint32_t transfer_index;
	uint32_t ready_transfers = 0;

	if (device->transfers == NULL) {
		// This shouldn't happen.
		return HACKRF_ERROR_OTHER;
	}

	// If setting up for TX, call the TX callback to fill each
	// transfer buffer.

	// All transfers must be filled before any are submitted.
	// Otherwise a transfer might complete whilst the others are
	// still being filled, causing the transfer thread to make a
	// concurrent call to the TX callback.

	// We also need to handle the case where the callback returns
	// nonzero to indicate completion, so keep count of how many
	// transfers were made ready to submit at this stage.

	if (endpoint_address == TX_ENDPOINT_ADDRESS) {
		for (transfer_index = 0; transfer_index < TRANSFER_COUNT;
		     transfer_index++) {
			hackrf_transfer transfer = {
				.device = device,
				.buffer = device->transfers[transfer_index]->buffer,
				.buffer_length = TRANSFER_BUFFER_SIZE,
				.valid_length = TRANSFER_BUFFER_SIZE,
				.rx_ctx = device->rx_ctx,
				.tx_ctx = device->tx_ctx,
			};
			if ((device->callback(&transfer) == 0) &&
			    (transfer.valid_length > 0)) {
				device->transfers[transfer_index]->length =
					transfer.valid_length;
				ready_transfers++;
			} else {
				break;
			}
		}

	} else {
		// For RX, all transfers are already ready for use.
		ready_transfers = TRANSFER_COUNT;
	}

	// Now everything is ready, go ahead and submit the ready transfers. We must hold
	// the transfer lock whilst doing this, so that completion callbacks cannot resubmit
	// any transfers until all transfers have been initially submitted.
	pthread_mutex_lock(&device->transfer_lock);

	for (transfer_index = 0; transfer_index < ready_transfers; transfer_index++) {
		struct libusb_transfer* transfer = device->transfers[transfer_index];
		transfer->endpoint = endpoint_address;
		transfer->callback = callback;

		// Pad the size of a short transfer to the next 512-byte boundary.
		if (endpoint_address == TX_ENDPOINT_ADDRESS) {
			while (transfer->length % 512 != 0)
				transfer->buffer[transfer->length++] = 0;
		}

		error = libusb_submit_transfer(transfer);
		if (error != 0) {
			last_libusb_error = error;
			break;
		}
		device->active_transfers++;
	}

	if (error == 0) {
		// We should only continue streaming if all transfers were made ready
		// and submitted above. Otherwise, set streaming to false so that the
		// libusb completion callback won't submit further transfers.
		device->streaming = (ready_transfers == TRANSFER_COUNT);
		device->transfers_setup = true;

		// If we're not continuing streaming, follow up with a flush if needed.
		if (!device->streaming && device->flush) {
			error = libusb_submit_transfer(device->flush_transfer);
			if (error != 0) {
				last_libusb_error = error;
			}
		}
	}

	// Now we can release the transfer lock.
	pthread_mutex_unlock(&device->transfer_lock);

	if (error == 0) {
		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_LIBUSB;
	}
}

static int detach_kernel_drivers(libusb_device_handle* usb_device_handle)
{
	int i, num_interfaces, result;
	libusb_device* dev;
	struct libusb_config_descriptor* config;

	dev = libusb_get_device(usb_device_handle);
	result = libusb_get_active_config_descriptor(dev, &config);
	if (result < 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	}

	num_interfaces = config->bNumInterfaces;
	libusb_free_config_descriptor(config);
	for (i = 0; i < num_interfaces; i++) {
		result = libusb_kernel_driver_active(usb_device_handle, i);
		if (result < 0) {
			if (result == LIBUSB_ERROR_NOT_SUPPORTED) {
				return 0;
			}
			last_libusb_error = result;
			return HACKRF_ERROR_LIBUSB;
		} else if (result == 1) {
			result = libusb_detach_kernel_driver(usb_device_handle, i);
			if (result != 0) {
				last_libusb_error = result;
				return HACKRF_ERROR_LIBUSB;
			}
		}
	}
	return HACKRF_SUCCESS;
}

static int set_hackrf_configuration(libusb_device_handle* usb_device, int config)
{
	int result, curr_config;

	result = libusb_get_configuration(usb_device, &curr_config);
	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	}

	if (curr_config != config) {
		result = detach_kernel_drivers(usb_device);
		if (result != 0) {
			return result;
		}
		result = libusb_set_configuration(usb_device, config);
		if (result != 0) {
			last_libusb_error = result;
			return HACKRF_ERROR_LIBUSB;
		}
	}

	result = detach_kernel_drivers(usb_device);
	if (result != 0) {
		return result;
	}
	return LIBUSB_SUCCESS;
}

#ifdef __cplusplus
extern "C" {
#endif

int ADDCALL hackrf_init(void)
{
	int libusb_error;
	if (g_libusb_context != NULL) {
		return HACKRF_SUCCESS;
	}

	libusb_error = libusb_init(&g_libusb_context);
	if (libusb_error != 0) {
		last_libusb_error = libusb_error;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_exit(void)
{
	if (open_devices == 0) {
		if (g_libusb_context != NULL) {
			libusb_exit(g_libusb_context);
			g_libusb_context = NULL;
		}

		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_NOT_LAST_DEVICE;
	}
}

#ifndef LIBRARY_VERSION
	#define LIBRARY_VERSION "unknown"
#endif
const char* ADDCALL hackrf_library_version()
{
	return LIBRARY_VERSION;
}

#ifndef LIBRARY_RELEASE
	#define LIBRARY_RELEASE "unknown"
#endif
const char* ADDCALL hackrf_library_release()
{
	return LIBRARY_RELEASE;
}

hackrf_device_list_t* ADDCALL hackrf_device_list()
{
	int i;
	libusb_device_handle* usb_device = NULL;
	uint8_t serial_descriptor_index;
	char serial_number[64];
	uint8_t idx, serial_number_length;

	hackrf_device_list_t* list = calloc(1, sizeof(*list));
	if (list == NULL)
		return NULL;

	list->usb_devicecount = (int) libusb_get_device_list(
		g_libusb_context,
		(libusb_device***) &list->usb_devices);

	list->serial_numbers = calloc(list->usb_devicecount, sizeof(void*));
	list->usb_board_ids =
		calloc(list->usb_devicecount, sizeof(enum hackrf_usb_board_id));
	list->usb_device_index = calloc(list->usb_devicecount, sizeof(int));

	if (list->serial_numbers == NULL || list->usb_board_ids == NULL ||
	    list->usb_device_index == NULL) {
		hackrf_device_list_free(list);
		return NULL;
	}

	for (i = 0; i < list->usb_devicecount; i++) {
		struct libusb_device_descriptor device_descriptor;
		libusb_get_device_descriptor(list->usb_devices[i], &device_descriptor);

		if (device_descriptor.idVendor == hackrf_usb_vid) {
			if ((device_descriptor.idProduct == hackrf_one_usb_pid) ||
			    (device_descriptor.idProduct == hackrf_jawbreaker_usb_pid) ||
			    (device_descriptor.idProduct == rad1o_usb_pid)) {
				idx = list->devicecount++;
				list->usb_board_ids[idx] = device_descriptor.idProduct;
				list->usb_device_index[idx] = i;

				serial_descriptor_index = device_descriptor.iSerialNumber;
				if (serial_descriptor_index > 0) {
					if (libusb_open(
						    list->usb_devices[i],
						    &usb_device) != 0) {
						usb_device = NULL;
						continue;
					}
					serial_number_length =
						libusb_get_string_descriptor_ascii(
							usb_device,
							serial_descriptor_index,
							(unsigned char*) serial_number,
							sizeof(serial_number));
					if (serial_number_length >= USB_MAX_SERIAL_LENGTH)
						serial_number_length =
							USB_MAX_SERIAL_LENGTH;

					serial_number[serial_number_length] = 0;
					list->serial_numbers[idx] = strdup(serial_number);

					libusb_close(usb_device);
					usb_device = NULL;
				}
			}
		}
	}

	return list;
}

void ADDCALL hackrf_device_list_free(hackrf_device_list_t* list)
{
	int i;

	libusb_free_device_list((libusb_device**) list->usb_devices, 1);

	for (i = 0; i < list->devicecount; i++) {
		if (list->serial_numbers[i])
			free(list->serial_numbers[i]);
	}

	free(list->serial_numbers);
	free(list->usb_board_ids);
	free(list->usb_device_index);
	free(list);
}

libusb_device_handle* hackrf_open_usb(const char* const desired_serial_number)
{
	libusb_device_handle* usb_device = NULL;
	libusb_device** devices = NULL;
	const ssize_t list_length = libusb_get_device_list(g_libusb_context, &devices);
	ssize_t match_len = 0;
	ssize_t i;
	char serial_number[64];
	int serial_number_length;

	if (desired_serial_number) {
		/* If a shorter serial number is specified, only match against the suffix.
		 * Should probably complain if the match is not unique, currently doesn't.
		 */
		match_len = strlen(desired_serial_number);
		if (match_len > 32)
			return NULL;
	}

	for (i = 0; i < list_length; i++) {
		struct libusb_device_descriptor device_descriptor;
		libusb_get_device_descriptor(devices[i], &device_descriptor);

		if (device_descriptor.idVendor == hackrf_usb_vid) {
			if ((device_descriptor.idProduct == hackrf_one_usb_pid) ||
			    (device_descriptor.idProduct == hackrf_jawbreaker_usb_pid) ||
			    (device_descriptor.idProduct == rad1o_usb_pid)) {
				if (desired_serial_number != NULL) {
					const uint_fast8_t serial_descriptor_index =
						device_descriptor.iSerialNumber;
					if (serial_descriptor_index > 0) {
						if (libusb_open(
							    devices[i],
							    &usb_device) != 0) {
							usb_device = NULL;
							continue;
						}
						serial_number_length =
							libusb_get_string_descriptor_ascii(
								usb_device,
								serial_descriptor_index,
								(unsigned char*)
									serial_number,
								sizeof(serial_number));
						if (serial_number_length >=
						    USB_MAX_SERIAL_LENGTH)
							serial_number_length =
								USB_MAX_SERIAL_LENGTH;
						serial_number[serial_number_length] = 0;
						if (strncmp(serial_number +
								    serial_number_length -
								    match_len,
							    desired_serial_number,
							    match_len) == 0) {
							break;
						} else {
							libusb_close(usb_device);
							usb_device = NULL;
						}
					}
				} else {
					libusb_open(devices[i], &usb_device);
					break;
				}
			}
		}
	}

	libusb_free_device_list(devices, 1);

	return usb_device;
}

static int hackrf_open_setup(libusb_device_handle* usb_device, hackrf_device** device)
{
	int result;
	hackrf_device* lib_device;

	//int speed = libusb_get_device_speed(usb_device);
	// TODO: Error or warning if not high speed USB?

	result = set_hackrf_configuration(usb_device, USB_CONFIG_STANDARD);
	if (result != LIBUSB_SUCCESS) {
		libusb_close(usb_device);
		return result;
	}

	result = libusb_claim_interface(usb_device, 0);
	if (result != LIBUSB_SUCCESS) {
		last_libusb_error = result;
		libusb_close(usb_device);
		return HACKRF_ERROR_LIBUSB;
	}

	lib_device = NULL;
	lib_device = (hackrf_device*) calloc(1, sizeof(*lib_device));
	if (lib_device == NULL) {
		libusb_release_interface(usb_device, 0);
		libusb_close(usb_device);
		return HACKRF_ERROR_NO_MEM;
	}

	lib_device->usb_device = usb_device;
	lib_device->transfers = NULL;
	lib_device->callback = NULL;
	lib_device->transfer_thread_started = false;
	lib_device->streaming = false;
	lib_device->do_exit = false;
	lib_device->active_transfers = 0;
	lib_device->flush = false;
	lib_device->flush_transfer = NULL;
	lib_device->flush_callback = NULL;
	lib_device->flush_ctx = NULL;
	lib_device->tx_completion_callback = NULL;

	result = pthread_mutex_init(&lib_device->transfer_lock, NULL);
	if (result != 0) {
		free(lib_device);
		libusb_release_interface(usb_device, 0);
		libusb_close(usb_device);
		return HACKRF_ERROR_THREAD;
	}

	result = pthread_cond_init(&lib_device->all_finished_cv, NULL);
	if (result != 0) {
		free(lib_device);
		libusb_release_interface(usb_device, 0);
		libusb_close(usb_device);
		return HACKRF_ERROR_THREAD;
	}

	result = allocate_transfers(lib_device);
	if (result != 0) {
		free(lib_device);
		libusb_release_interface(usb_device, 0);
		libusb_close(usb_device);
		return HACKRF_ERROR_NO_MEM;
	}

	result = create_transfer_thread(lib_device);
	if (result != 0) {
		free(lib_device);
		libusb_release_interface(usb_device, 0);
		libusb_close(usb_device);
		return result;
	}

	*device = lib_device;
	open_devices++;

	return HACKRF_SUCCESS;
}

int ADDCALL hackrf_open(hackrf_device** device)
{
	libusb_device_handle* usb_device;

	if (device == NULL) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	usb_device = libusb_open_device_with_vid_pid(
		g_libusb_context,
		hackrf_usb_vid,
		hackrf_one_usb_pid);

	if (usb_device == NULL) {
		usb_device = libusb_open_device_with_vid_pid(
			g_libusb_context,
			hackrf_usb_vid,
			hackrf_jawbreaker_usb_pid);
	}

	if (usb_device == NULL) {
		usb_device = libusb_open_device_with_vid_pid(
			g_libusb_context,
			hackrf_usb_vid,
			rad1o_usb_pid);
	}

	if (usb_device == NULL) {
		return HACKRF_ERROR_NOT_FOUND;
	}

	return hackrf_open_setup(usb_device, device);
}

int ADDCALL hackrf_open_by_serial(
	const char* const desired_serial_number,
	hackrf_device** device)
{
	libusb_device_handle* usb_device;

	if (desired_serial_number == NULL) {
		return hackrf_open(device);
	}

	if (device == NULL) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	usb_device = hackrf_open_usb(desired_serial_number);

	if (usb_device == NULL) {
		return HACKRF_ERROR_NOT_FOUND;
	}

	return hackrf_open_setup(usb_device, device);
}

int ADDCALL hackrf_device_list_open(
	hackrf_device_list_t* list,
	int idx,
	hackrf_device** device)
{
	libusb_device_handle* usb_device;
	int i, result;

	if (device == NULL || list == NULL || idx < 0 || idx >= list->devicecount) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	i = list->usb_device_index[idx];

	result = libusb_open(list->usb_devices[i], &usb_device);
	if (result != 0) {
		usb_device = NULL;
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	}

	return hackrf_open_setup(usb_device, device);
}

int ADDCALL hackrf_set_transceiver_mode(
	hackrf_device* device,
	hackrf_transceiver_mode value)
{
	int result;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SET_TRANSCEIVER_MODE,
		value,
		0,
		NULL,
		0,
		0);

	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_max2837_read(
	hackrf_device* device,
	uint8_t register_number,
	uint16_t* value)
{
	int result;

	if (register_number >= 32) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_MAX2837_READ,
		0,
		register_number,
		(unsigned char*) value,
		2,
		0);

	if (result < 2) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_max2837_write(
	hackrf_device* device,
	uint8_t register_number,
	uint16_t value)
{
	int result;

	if (register_number >= 32) {
		return HACKRF_ERROR_INVALID_PARAM;
	}
	if (value >= 0x400) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_MAX2837_WRITE,
		value,
		register_number,
		NULL,
		0,
		0);

	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_si5351c_read(
	hackrf_device* device,
	uint16_t register_number,
	uint16_t* value)
{
	uint8_t temp_value;
	int result;

	if (register_number >= 256) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	temp_value = 0;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SI5351C_READ,
		0,
		register_number,
		(unsigned char*) &temp_value,
		1,
		0);

	if (result < 1) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		*value = temp_value;
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_si5351c_write(
	hackrf_device* device,
	uint16_t register_number,
	uint16_t value)
{
	int result;

	if (register_number >= 256) {
		return HACKRF_ERROR_INVALID_PARAM;
	}
	if (value >= 256) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SI5351C_WRITE,
		value,
		register_number,
		NULL,
		0,
		0);

	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_set_baseband_filter_bandwidth(
	hackrf_device* device,
	const uint32_t bandwidth_hz)
{
	int result;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_BASEBAND_FILTER_BANDWIDTH_SET,
		bandwidth_hz & 0xffff,
		bandwidth_hz >> 16,
		NULL,
		0,
		0);

	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_rffc5071_read(
	hackrf_device* device,
	uint8_t register_number,
	uint16_t* value)
{
	int result;

	if (register_number >= 31) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_RFFC5071_READ,
		0,
		register_number,
		(unsigned char*) value,
		2,
		0);

	if (result < 2) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_rffc5071_write(
	hackrf_device* device,
	uint8_t register_number,
	uint16_t value)
{
	int result;

	if (register_number >= 31) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_RFFC5071_WRITE,
		value,
		register_number,
		NULL,
		0,
		0);

	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_get_m0_state(hackrf_device* device, hackrf_m0_state* state)
{
	USB_API_REQUIRED(device, 0x0106)
	int result;

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_GET_M0_STATE,
		0,
		0,
		(unsigned char*) state,
		sizeof(hackrf_m0_state),
		0);

	if (result < sizeof(hackrf_m0_state)) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		state->request_flag = FROM_LE16(state->request_flag);
		state->requested_mode = FROM_LE16(state->requested_mode);
		state->active_mode = FROM_LE32(state->active_mode);
		state->m0_count = FROM_LE32(state->m0_count);
		state->m4_count = FROM_LE32(state->m4_count);
		state->num_shortfalls = FROM_LE32(state->num_shortfalls);
		state->longest_shortfall = FROM_LE32(state->longest_shortfall);
		state->shortfall_limit = FROM_LE32(state->shortfall_limit);
		state->threshold = FROM_LE32(state->threshold);
		state->next_mode = FROM_LE32(state->next_mode);
		state->error = FROM_LE32(state->error);
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_set_tx_underrun_limit(hackrf_device* device, uint32_t value)
{
	USB_API_REQUIRED(device, 0x0106)
	int result;

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SET_TX_UNDERRUN_LIMIT,
		value & 0xffff,
		value >> 16,
		NULL,
		0,
		0);

	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_set_rx_overrun_limit(hackrf_device* device, uint32_t value)
{
	USB_API_REQUIRED(device, 0x0106)
	int result;

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SET_RX_OVERRUN_LIMIT,
		value & 0xffff,
		value >> 16,
		NULL,
		0,
		0);

	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_spiflash_erase(hackrf_device* device)
{
	int result;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SPIFLASH_ERASE,
		0,
		0,
		NULL,
		0,
		0);

	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_spiflash_write(
	hackrf_device* device,
	const uint32_t address,
	const uint16_t length,
	unsigned char* const data)
{
	int result;

	if (address > 0x0FFFFF) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SPIFLASH_WRITE,
		address >> 16,
		address & 0xFFFF,
		data,
		length,
		0);

	if (result < length) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_spiflash_read(
	hackrf_device* device,
	const uint32_t address,
	const uint16_t length,
	unsigned char* data)
{
	int result;

	if (address > 0x0FFFFF) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SPIFLASH_READ,
		address >> 16,
		address & 0xFFFF,
		data,
		length,
		0);

	if (result < length) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_spiflash_status(hackrf_device* device, uint8_t* data)
{
	USB_API_REQUIRED(device, 0x0103)
	int result;

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SPIFLASH_STATUS,
		0,
		0,
		data,
		2,
		0);

	if (result < 1) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_spiflash_clear_status(hackrf_device* device)
{
	USB_API_REQUIRED(device, 0x0103)
	int result;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SPIFLASH_CLEAR_STATUS,
		0,
		0,
		NULL,
		0,
		0);

	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_cpld_write(
	hackrf_device* device,
	unsigned char* const data,
	const unsigned int total_length)
{
	const unsigned int chunk_size = 512;
	unsigned int i;
	int result, transferred = 0;

	result = hackrf_set_transceiver_mode(device, TRANSCEIVER_MODE_CPLD_UPDATE);
	if (result != 0)
		return result;

	for (i = 0; i < total_length; i += chunk_size) {
		result = libusb_bulk_transfer(
			device->usb_device,
			TX_ENDPOINT_ADDRESS,
			&data[i],
			chunk_size,
			&transferred,
			10000 // long timeout to allow for CPLD programming
		);

		if (result != LIBUSB_SUCCESS) {
			last_libusb_error = result;
			return HACKRF_ERROR_LIBUSB;
		}
	}

	return HACKRF_SUCCESS;
}

int ADDCALL hackrf_board_id_read(hackrf_device* device, uint8_t* value)
{
	int result;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_BOARD_ID_READ,
		0,
		0,
		value,
		1,
		0);

	if (result < 1) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_version_string_read(
	hackrf_device* device,
	char* version,
	uint8_t length)
{
	int result;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_VERSION_STRING_READ,
		0,
		0,
		(unsigned char*) version,
		length,
		0);

	if (result < 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		version[result] = '\0';
		return HACKRF_SUCCESS;
	}
}

extern ADDAPI int ADDCALL hackrf_usb_api_version_read(
	hackrf_device* device,
	uint16_t* version)
{
	int result;
	libusb_device* dev;
	struct libusb_device_descriptor desc;
	dev = libusb_get_device(device->usb_device);
	result = libusb_get_device_descriptor(dev, &desc);
	if (result < 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	}

	*version = desc.bcdDevice;
	return HACKRF_SUCCESS;
}

typedef struct {
	uint32_t freq_mhz; /* From 0 to 6000+MHz */
	uint32_t freq_hz;  /* From 0 to 999999Hz */
			   /* Final Freq = freq_mhz+freq_hz */
} set_freq_params_t;

#define FREQ_ONE_MHZ (1000 * 1000ull)

int ADDCALL hackrf_set_freq(hackrf_device* device, const uint64_t freq_hz)
{
	uint32_t l_freq_mhz;
	uint32_t l_freq_hz;
	set_freq_params_t set_freq_params;
	uint8_t length;
	int result;

	/* Convert Freq Hz 64bits to Freq MHz (32bits) & Freq Hz (32bits) */
	l_freq_mhz = (uint32_t) (freq_hz / FREQ_ONE_MHZ);
	l_freq_hz = (uint32_t) (freq_hz - (((uint64_t) l_freq_mhz) * FREQ_ONE_MHZ));
	set_freq_params.freq_mhz = TO_LE(l_freq_mhz);
	set_freq_params.freq_hz = TO_LE(l_freq_hz);
	length = sizeof(set_freq_params_t);

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SET_FREQ,
		0,
		0,
		(unsigned char*) &set_freq_params,
		length,
		0);

	if (result < length) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

struct set_freq_explicit_params {
	uint64_t if_freq_hz; /* intermediate frequency */
	uint64_t lo_freq_hz; /* front-end local oscillator frequency */
	uint8_t path;        /* image rejection filter path */
};

int ADDCALL hackrf_set_freq_explicit(
	hackrf_device* device,
	const uint64_t if_freq_hz,
	const uint64_t lo_freq_hz,
	const enum rf_path_filter path)
{
	struct set_freq_explicit_params params;
	uint8_t length;
	int result;

	/*
	 * Restriction to the range 2170-2740 MHz is strongly recommended for
	 * HackRF One and Jawbreaker.  We permit IF as low as 2000 MHz and as
	 * high as 3000 MHz for backwards compatibility and for
	 * experimentation, but settings outside the recommended range are
	 * unlikely to work.
	 */
	if (if_freq_hz < 2000000000 || if_freq_hz > 3000000000) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	if ((path != RF_PATH_FILTER_BYPASS) &&
	    (lo_freq_hz < 84375000 || lo_freq_hz > 5400000000)) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	if (path > 2) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	params.if_freq_hz = TO_LE(if_freq_hz);
	params.lo_freq_hz = TO_LE(lo_freq_hz);
	params.path = (uint8_t) path;
	length = sizeof(struct set_freq_explicit_params);

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SET_FREQ_EXPLICIT,
		0,
		0,
		(unsigned char*) &params,
		length,
		0);

	if (result < length) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

typedef struct {
	uint32_t freq_hz;
	uint32_t divider;
} set_fracrate_params_t;

/*
 * You should probably use hackrf_set_sample_rate() below instead of this
 * function.  They both result in automatic baseband filter selection as
 * described below.
 */
int ADDCALL hackrf_set_sample_rate_manual(
	hackrf_device* device,
	const uint32_t freq_hz,
	const uint32_t divider)
{
	set_fracrate_params_t set_fracrate_params;
	uint8_t length;
	int result;

	set_fracrate_params.freq_hz = TO_LE(freq_hz);
	set_fracrate_params.divider = TO_LE(divider);
	length = sizeof(set_fracrate_params_t);

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SAMPLE_RATE_SET,
		0,
		0,
		(unsigned char*) &set_fracrate_params,
		length,
		0);

	if (result < length) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return hackrf_set_baseband_filter_bandwidth(
			device,
			hackrf_compute_baseband_filter_bw(
				(uint32_t) (0.75 * freq_hz / divider)));
	}
}

/*
 * For anti-aliasing, the baseband filter bandwidth is automatically set to the
 * widest available setting that is no more than 75% of the sample rate.  This
 * happens every time the sample rate is set.  If you want to override the
 * baseband filter selection, you must do so after setting the sample rate.
 */
int ADDCALL hackrf_set_sample_rate(hackrf_device* device, const double freq)
{
	const int MAX_N = 32;
	uint32_t freq_hz, divider;
	double freq_frac = 1.0 + freq - (int) freq;
	uint64_t a, m;
	int i, e;

	union {
		uint64_t u64;
		double d;
	} v;

	v.d = freq;

	e = (v.u64 >> 52) - 1023;

	m = ((1ULL << 52) - 1);

	v.d = freq_frac;
	v.u64 &= m;

	m &= ~((1 << (e + 4)) - 1);

	a = 0;

	for (i = 1; i < MAX_N; i++) {
		a += v.u64;
		if (!(a & m) || !(~a & m))
			break;
	}

	if (i == MAX_N)
		i = 1;

	freq_hz = (uint32_t) (freq * i + 0.5);
	divider = i;

	return hackrf_set_sample_rate_manual(device, freq_hz, divider);
}

int ADDCALL hackrf_set_amp_enable(hackrf_device* device, const uint8_t value)
{
	int result;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_AMP_ENABLE,
		value,
		0,
		NULL,
		0,
		0);

	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_board_partid_serialno_read(
	hackrf_device* device,
	read_partid_serialno_t* read_partid_serialno)
{
	uint8_t length;
	int result;

	length = sizeof(read_partid_serialno_t);
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_BOARD_PARTID_SERIALNO_READ,
		0,
		0,
		(unsigned char*) read_partid_serialno,
		length,
		0);

	if (result < length) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		read_partid_serialno->part_id[0] =
			TO_LE(read_partid_serialno->part_id[0]);
		read_partid_serialno->part_id[1] =
			TO_LE(read_partid_serialno->part_id[1]);
		read_partid_serialno->serial_no[0] =
			TO_LE(read_partid_serialno->serial_no[0]);
		read_partid_serialno->serial_no[1] =
			TO_LE(read_partid_serialno->serial_no[1]);
		read_partid_serialno->serial_no[2] =
			TO_LE(read_partid_serialno->serial_no[2]);
		read_partid_serialno->serial_no[3] =
			TO_LE(read_partid_serialno->serial_no[3]);

		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_set_lna_gain(hackrf_device* device, uint32_t value)
{
	int result;
	uint8_t retval;

	if (value > 40) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	value &= ~0x07;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SET_LNA_GAIN,
		0,
		value,
		&retval,
		1,
		0);

	if (result != 1 || !retval) {
		return HACKRF_ERROR_INVALID_PARAM;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_set_vga_gain(hackrf_device* device, uint32_t value)
{
	int result;
	uint8_t retval;

	if (value > 62) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	value &= ~0x01;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SET_VGA_GAIN,
		0,
		value,
		&retval,
		1,
		0);

	if (result != 1 || !retval) {
		return HACKRF_ERROR_INVALID_PARAM;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_set_txvga_gain(hackrf_device* device, uint32_t value)
{
	int result;
	uint8_t retval;

	if (value > 47) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SET_TXVGA_GAIN,
		0,
		value,
		&retval,
		1,
		0);

	if (result != 1 || !retval) {
		return HACKRF_ERROR_INVALID_PARAM;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_set_antenna_enable(hackrf_device* device, const uint8_t value)
{
	int result;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_ANTENNA_ENABLE,
		value,
		0,
		NULL,
		0,
		0);

	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

static void* transfer_threadproc(void* arg)
{
	hackrf_device* device = (hackrf_device*) arg;
	int error;
	struct timeval timeout = {0, 500000};

	/*
	 * hackrf_transfer uses pause() and SIGALRM to print statistics and
	 * POSIX doesn't specify which thread must recieve the signal, block all
	 * signals here, so we don't interrupt their reception by
	 * hackrf_transfer or any other app which uses the library (#1323)
	 */
#ifndef _WIN32
	sigset_t signal_mask;
	sigfillset(&signal_mask);
	if (pthread_sigmask(SIG_BLOCK, &signal_mask, NULL) != 0) {
		return NULL;
	}
#endif

	while (device->do_exit == false) {
		error = libusb_handle_events_timeout(g_libusb_context, &timeout);
		if ((error != 0) && (error != LIBUSB_ERROR_INTERRUPTED)) {
			device->streaming = false;
		}
	}

	return NULL;
}

static void LIBUSB_CALL hackrf_libusb_flush_callback(struct libusb_transfer* usb_transfer)
{
	bool success = usb_transfer->status == LIBUSB_TRANSFER_COMPLETED;

	// All transfers have now ended, so proceed with signalling completion.
	hackrf_device* device = (hackrf_device*) usb_transfer->user_data;
	pthread_mutex_lock(&device->transfer_lock);
	device->flush = false;
	device->active_transfers = 0;
	pthread_cond_broadcast(&device->all_finished_cv);
	pthread_mutex_unlock(&device->transfer_lock);

	if (device->flush_callback)
		device->flush_callback(device->flush_ctx, success);
}

static void LIBUSB_CALL
hackrf_libusb_transfer_callback(struct libusb_transfer* usb_transfer)
{
	hackrf_device* device = (hackrf_device*) usb_transfer->user_data;
	bool success, resubmit = false;
	int result;

	hackrf_transfer transfer = {
		.device = device,
		.buffer = usb_transfer->buffer,
		.buffer_length = TRANSFER_BUFFER_SIZE,
		.valid_length = usb_transfer->actual_length,
		.rx_ctx = device->rx_ctx,
		.tx_ctx = device->tx_ctx};

	success = usb_transfer->status == LIBUSB_TRANSFER_COMPLETED;

	if (device->tx_completion_callback != NULL) {
		device->tx_completion_callback(&transfer, success);
	}

	// Take lock to make sure that we don't restart a
	// transfer whilst cancel_transfers() is in the middle
	// of stopping them.
	pthread_mutex_lock(&device->transfer_lock);
	if (success) {
		if (device->streaming && (device->callback(&transfer) == 0) &&
		    (transfer.valid_length > 0)) {
			if ((resubmit = device->transfers_setup)) {
				if (usb_transfer->endpoint == TX_ENDPOINT_ADDRESS) {
					usb_transfer->length = transfer.valid_length;
					// Pad to the next 512-byte boundary.
					uint8_t* buffer = usb_transfer->buffer;
					while (usb_transfer->length % 512 != 0)
						buffer[usb_transfer->length++] = 0;
				}
				result = libusb_submit_transfer(usb_transfer);
			}
		} else if (device->flush) {
			result = libusb_submit_transfer(device->flush_transfer);
			if (result != LIBUSB_SUCCESS) {
				device->streaming = false;
				device->flush = false;
			}
		}
	} else {
		device->streaming = false;
		device->flush = false;
	}

	// If a data transfer was resubmitted successfully, we're done.
	if (!resubmit || result != LIBUSB_SUCCESS) {
		// No further calls should be made to the TX callback.
		device->streaming = false;

		// If this is the last transfer, signal that all are now finished.
		if (device->active_transfers == 1) {
			if (!device->flush) {
				device->active_transfers = 0;
				pthread_cond_broadcast(&device->all_finished_cv);
			}
		} else {
			device->active_transfers--;
		}
	}

	// Now we can release the lock. Our transfer was either
	// cancelled or restarted, not both.
	pthread_mutex_unlock(&device->transfer_lock);
}

static int kill_transfer_thread(hackrf_device* device)
{
	void* value;
	int result;

	if (device->transfer_thread_started != false) {
		/*
		 * Cancel transfers. This call will block until the transfer
		 * thread has handled all completion callbacks.
		 */
		cancel_transfers(device);

		// Set flag to tell the thread to exit.
		device->do_exit = true;

		/*
		 * Interrupt the event handling thread instead of
		 * waiting for timeout.
		 */
		libusb_interrupt_event_handler(g_libusb_context);

		value = NULL;
		result = pthread_join(device->transfer_thread, &value);
		if (result != 0) {
			return HACKRF_ERROR_THREAD;
		}
		device->transfer_thread_started = false;
	}

	/*
	 * Reset do_exit; we're now done here and the thread was
	 * already dead or is now dead.
	 */
	device->do_exit = false;

	return HACKRF_SUCCESS;
}

static int prepare_setup_transfers(
	hackrf_device* device,
	const uint8_t endpoint_address,
	hackrf_sample_block_cb_fn callback)
{
	if (device->transfers_setup == true) {
		return HACKRF_ERROR_BUSY;
	}

	device->callback = callback;
	return prepare_transfers(
		device,
		endpoint_address,
		hackrf_libusb_transfer_callback);
}

static int create_transfer_thread(hackrf_device* device)
{
	int result;

	if (device->transfer_thread_started == false) {
		device->streaming = false;
		device->do_exit = false;
		result = pthread_create(
			&device->transfer_thread,
			0,
			transfer_threadproc,
			device);
		if (result == 0) {
			device->transfer_thread_started = true;
		} else {
			return HACKRF_ERROR_THREAD;
		}
	} else {
		return HACKRF_ERROR_BUSY;
	}

	return HACKRF_SUCCESS;
}

int ADDCALL hackrf_is_streaming(hackrf_device* device)
{
	/* return hackrf is streaming only when streaming, transfer_thread_started are true and do_exit equal false */

	if ((device->transfer_thread_started == true) && (device->streaming == true) &&
	    (device->do_exit == false)) {
		return HACKRF_TRUE;
	} else {
		if (device->transfer_thread_started == false) {
			return HACKRF_ERROR_STREAMING_THREAD_ERR;
		}

		if (device->streaming == false) {
			return HACKRF_ERROR_STREAMING_STOPPED;
		}

		return HACKRF_ERROR_STREAMING_EXIT_CALLED;
	}
}

int ADDCALL hackrf_start_rx(
	hackrf_device* device,
	hackrf_sample_block_cb_fn callback,
	void* rx_ctx)
{
	int result;
	const uint8_t endpoint_address = RX_ENDPOINT_ADDRESS;
	device->rx_ctx = rx_ctx;
	result = hackrf_set_transceiver_mode(device, HACKRF_TRANSCEIVER_MODE_RECEIVE);
	if (result == HACKRF_SUCCESS) {
		result = prepare_setup_transfers(device, endpoint_address, callback);
	}
	return result;
}

static int hackrf_stop_cmd(hackrf_device* device)
{
	int result;
	result = hackrf_set_transceiver_mode(device, HACKRF_TRANSCEIVER_MODE_OFF);
	return result;
}

/*
 * Stop any pending receive.
 *
 * This call stops transfers and halts recieve if it is enabled.
 *
 * It returns HACKRF_SUCCESS if receive was started and it was
 * properly stopped, an error otherwise.
 */
int ADDCALL hackrf_stop_rx(hackrf_device* device)
{
	int result;

	result = cancel_transfers(device);
	if (result != HACKRF_SUCCESS) {
		return result;
	}

	return hackrf_stop_cmd(device);
}

int ADDCALL hackrf_start_tx(
	hackrf_device* device,
	hackrf_sample_block_cb_fn callback,
	void* tx_ctx)
{
	int result;
	const uint8_t endpoint_address = TX_ENDPOINT_ADDRESS;
	if (device->flush_transfer != NULL) {
		device->flush = true;
	}
	result = hackrf_set_transceiver_mode(device, HACKRF_TRANSCEIVER_MODE_TRANSMIT);
	if (result == HACKRF_SUCCESS) {
		device->tx_ctx = tx_ctx;
		result = prepare_setup_transfers(device, endpoint_address, callback);
	}
	return result;
}

ADDAPI int ADDCALL hackrf_set_tx_block_complete_callback(
	hackrf_device* device,
	hackrf_tx_block_complete_cb_fn callback)
{
	device->tx_completion_callback = callback;
	return HACKRF_SUCCESS;
}

ADDAPI int ADDCALL hackrf_enable_tx_flush(
	hackrf_device* device,
	hackrf_flush_cb_fn callback,
	void* flush_ctx)
{
	device->flush_callback = callback;
	device->flush_ctx = flush_ctx;

	if (device->flush_transfer) {
		return HACKRF_SUCCESS;
	}

	if ((device->flush_transfer = libusb_alloc_transfer(0)) == NULL) {
		return HACKRF_ERROR_LIBUSB;
	}

	libusb_fill_bulk_transfer(
		device->flush_transfer,
		device->usb_device,
		TX_ENDPOINT_ADDRESS,
		calloc(1, DEVICE_BUFFER_SIZE),
		DEVICE_BUFFER_SIZE,
		hackrf_libusb_flush_callback,
		device,
		0);

	device->flush_transfer->flags = LIBUSB_TRANSFER_FREE_BUFFER;

	return HACKRF_SUCCESS;
}

ADDAPI int ADDCALL hackrf_disable_tx_flush(hackrf_device* device)
{
	libusb_free_transfer(device->flush_transfer);
	device->flush_transfer = NULL;
	device->flush_callback = NULL;
	device->flush_ctx = NULL;

	return HACKRF_SUCCESS;
}

/*
 * Stop any pending transmit.
 *
 * This call stops transfers and halts transmit if it is enabled.
 *
 * It returns HACKRF_SUCCESS if receive was started and it was
 * properly stopped, an error otherwise.
 */
int ADDCALL hackrf_stop_tx(hackrf_device* device)
{
	int result;
	result = cancel_transfers(device);
	if (result != HACKRF_SUCCESS) {
		return result;
	}

	return hackrf_stop_cmd(device);
}

int ADDCALL hackrf_close(hackrf_device* device)
{
	int result1, result2;

	result1 = HACKRF_SUCCESS;
	result2 = HACKRF_SUCCESS;

	if (device != NULL) {
		result1 = hackrf_stop_cmd(device);

		/*
		 * Finally kill the transfer thread, which will
		 * also cancel any pending transmit/receive transfers.
		 */
		result2 = kill_transfer_thread(device);
		if (device->usb_device != NULL) {
			libusb_release_interface(device->usb_device, 0);
			libusb_close(device->usb_device);
			device->usb_device = NULL;
		}

		free_transfers(device);

		pthread_mutex_destroy(&device->transfer_lock);
		pthread_cond_destroy(&device->all_finished_cv);

		free(device);
	}
	open_devices--;

	if (result2 != HACKRF_SUCCESS) {
		return result2;
	}
	return result1;
}

const char* ADDCALL hackrf_error_name(enum hackrf_error errcode)
{
	switch (errcode) {
	case HACKRF_SUCCESS:
		return "HACKRF_SUCCESS";

	case HACKRF_TRUE:
		return "HACKRF_TRUE";

	case HACKRF_ERROR_INVALID_PARAM:
		return "invalid parameter(s)";

	case HACKRF_ERROR_NOT_FOUND:
		return "HackRF not found";

	case HACKRF_ERROR_BUSY:
		return "HackRF busy";

	case HACKRF_ERROR_NO_MEM:
		return "insufficient memory";

	case HACKRF_ERROR_LIBUSB:
#if defined(LIBUSB_API_VERSION) && (LIBUSB_API_VERSION >= 0x01000103)
		if (last_libusb_error != LIBUSB_SUCCESS)
			return libusb_strerror(last_libusb_error);
#endif
		return "USB error";

	case HACKRF_ERROR_THREAD:
		return "transfer thread error";

	case HACKRF_ERROR_STREAMING_THREAD_ERR:
		return "streaming thread encountered an error";

	case HACKRF_ERROR_STREAMING_STOPPED:
		return "streaming stopped";

	case HACKRF_ERROR_STREAMING_EXIT_CALLED:
		return "streaming terminated";

	case HACKRF_ERROR_USB_API_VERSION:
		return "feature not supported by installed firmware";

	case HACKRF_ERROR_NOT_LAST_DEVICE:
		return "one or more HackRFs still in use";

	case HACKRF_ERROR_OTHER:
		return "unspecified error";

	default:
		return "unknown error code";
	}
}

const char* ADDCALL hackrf_board_id_name(enum hackrf_board_id board_id)
{
	switch (board_id) {
	case BOARD_ID_JELLYBEAN:
		return "Jellybean";

	case BOARD_ID_JAWBREAKER:
		return "Jawbreaker";

	case BOARD_ID_HACKRF1_OG:
		return "HackRF One";

	case BOARD_ID_RAD1O:
		return "rad1o";

	case BOARD_ID_HACKRF1_R9:
		return "HackRF One";

	case BOARD_ID_UNRECOGNIZED:
		return "unrecognized";

	case BOARD_ID_UNDETECTED:
		return "undetected";

	default:
		return "unknown";
	}
}

extern ADDAPI uint32_t ADDCALL hackrf_board_id_platform(enum hackrf_board_id board_id)
{
	switch (board_id) {
	case BOARD_ID_JAWBREAKER:
		return HACKRF_PLATFORM_JAWBREAKER;

	case BOARD_ID_HACKRF1_OG:
		return HACKRF_PLATFORM_HACKRF1_OG;

	case BOARD_ID_RAD1O:
		return HACKRF_PLATFORM_RAD1O;

	case BOARD_ID_HACKRF1_R9:
		return HACKRF_PLATFORM_HACKRF1_R9;

	default:
		return 0;
	}
}

extern ADDAPI const char* ADDCALL hackrf_usb_board_id_name(
	enum hackrf_usb_board_id usb_board_id)
{
	switch (usb_board_id) {
	case USB_BOARD_ID_JAWBREAKER:
		return "Jawbreaker";

	case USB_BOARD_ID_HACKRF_ONE:
		return "HackRF One";

	case USB_BOARD_ID_RAD1O:
		return "rad1o";

	case USB_BOARD_ID_INVALID:
		return "Invalid Board ID";

	default:
		return "Unknown Board ID";
	}
}

const char* ADDCALL hackrf_filter_path_name(const enum rf_path_filter path)
{
	switch (path) {
	case RF_PATH_FILTER_BYPASS:
		return "mixer bypass";
	case RF_PATH_FILTER_LOW_PASS:
		return "low pass filter";
	case RF_PATH_FILTER_HIGH_PASS:
		return "high pass filter";
	default:
		return "invalid filter path";
	}
}

/* Return final bw round down and less than expected bw. */
uint32_t ADDCALL hackrf_compute_baseband_filter_bw_round_down_lt(
	const uint32_t bandwidth_hz)
{
	const max2837_ft_t* p = max2837_ft;
	while (p->bandwidth_hz != 0) {
		if (p->bandwidth_hz >= bandwidth_hz) {
			break;
		}
		p++;
	}
	/* Round down (if no equal to first entry) */
	if (p != max2837_ft) {
		p--;
	}
	return p->bandwidth_hz;
}

/* Return final bw. */
uint32_t ADDCALL hackrf_compute_baseband_filter_bw(const uint32_t bandwidth_hz)
{
	const max2837_ft_t* p = max2837_ft;
	while (p->bandwidth_hz != 0) {
		if (p->bandwidth_hz >= bandwidth_hz) {
			break;
		}
		p++;
	}

	/* Round down (if no equal to first entry) and if > bandwidth_hz */
	if (p != max2837_ft) {
		if (p->bandwidth_hz > bandwidth_hz)
			p--;
	}

	return p->bandwidth_hz;
}

/* All features below require USB API version 0x0102 or higher) */

int ADDCALL hackrf_set_hw_sync_mode(hackrf_device* device, const uint8_t value)
{
	USB_API_REQUIRED(device, 0x0102)
	int result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SET_HW_SYNC_MODE,
		value,
		0,
		NULL,
		0,
		0);

	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

/*
 * Initialize sweep mode:
 * frequency_list is a list of start/stop pairs of frequencies in MHz.
 * num_ranges is the number of pairs in frequency_list (1 to 10)
 * num_bytes is the number of sample bytes to capture after each tuning.
 * step_width is the width in Hz of the tuning step.
 * offset is a number of Hz added to every tuning frequency.
 *     Use to select center frequency based on the expected usable bandwidth.
 * sweep_mode
 *     LINEAR means step_width is added to the current frequency at each step.
 *     INTERLEAVED invokes a scheme in which each step is divided into two
 *         interleaved sub-steps, allowing the host to select the best portions
 *         of the FFT of each sub-step and discard the rest.
 */
int ADDCALL hackrf_init_sweep(
	hackrf_device* device,
	const uint16_t* frequency_list,
	const int num_ranges,
	const uint32_t num_bytes,
	const uint32_t step_width,
	const uint32_t offset,
	const enum sweep_style style)
{
	USB_API_REQUIRED(device, 0x0102)
	int result, i;
	unsigned char data[9 + MAX_SWEEP_RANGES * 2 * sizeof(frequency_list[0])];
	int size = 9 + num_ranges * 2 * sizeof(frequency_list[0]);

	if ((num_ranges < 1) || (num_ranges > MAX_SWEEP_RANGES)) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	if (num_bytes % BYTES_PER_BLOCK) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	if (BYTES_PER_BLOCK > num_bytes) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	if (1 > step_width) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	if (INTERLEAVED < style) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	data[0] = step_width & 0xff;
	data[1] = (step_width >> 8) & 0xff;
	data[2] = (step_width >> 16) & 0xff;
	data[3] = (step_width >> 24) & 0xff;
	data[4] = offset & 0xff;
	data[5] = (offset >> 8) & 0xff;
	data[6] = (offset >> 16) & 0xff;
	data[7] = (offset >> 24) & 0xff;
	data[8] = style;
	for (i = 0; i < (num_ranges * 2); i++) {
		data[9 + i * 2] = frequency_list[i] & 0xff;
		data[10 + i * 2] = (frequency_list[i] >> 8) & 0xff;
	}

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_INIT_SWEEP,
		num_bytes & 0xffff,
		(num_bytes >> 16) & 0xffff,
		data,
		size,
		0);

	if (result < size) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

bool hackrf_operacake_valid_address(uint8_t address)
{
	return address < HACKRF_OPERACAKE_MAX_BOARDS;
}

/**
 * Retrieve list of Opera Cake board addresses
 * @param[in]  device
 * @param[out] boards List of board addresses. Must point to a `uint8_t[8]`. If the number of boards is less than 8, extra entries are filled with @ref HACKRF_OPERACAKE_ADDRESS_INVALID.
 * @return @ref HACKRF_SUCCESS
 * @return @ref HACKRF_ERROR_LIBUSB
 */
int ADDCALL hackrf_get_operacake_boards(hackrf_device* device, uint8_t* boards)
{
	USB_API_REQUIRED(device, 0x0105)
	int result;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_OPERACAKE_GET_BOARDS,
		0,
		0,
		boards,
		8,
		0);

	if (result < 8) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

/**
 * Set Opera Cake switching mode.
 * @param[in] device
 * @param[in] address Opera Cake address.
 * @param[in] mode    Switching mode.
 * @return @ref HACKRF_SUCCESS
 * @return @ref HACKRF_ERROR_LIBUSB
 */
int ADDCALL hackrf_set_operacake_mode(
	hackrf_device* device,
	uint8_t address,
	enum operacake_switching_mode mode)
{
	USB_API_REQUIRED(device, 0x0105)

	if (!hackrf_operacake_valid_address(address)) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	int result;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_OPERACAKE_SET_MODE,
		address,
		(uint8_t) mode,
		NULL,
		0,
		0);

	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

/**
 * Get Opera Cake switching mode.
 * @param[in]  device
 * @param[in]  address Opera Cake address.
 * @param[out] mode    Switching mode.
 * @return @ref HACKRF_SUCCESS
 * @return @ref HACKRF_ERROR_LIBUSB
 */
int ADDCALL hackrf_get_operacake_mode(
	hackrf_device* device,
	uint8_t address,
	enum operacake_switching_mode* mode)
{
	USB_API_REQUIRED(device, 0x0105)

	if (!hackrf_operacake_valid_address(address)) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	int result;
	uint8_t buf;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_OPERACAKE_GET_MODE,
		address,
		0,
		&buf,
		1,
		0);

	if (result < 1) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		*mode = buf;
		return HACKRF_SUCCESS;
	}
}

/* Set Operacake manual mode ports. */
int ADDCALL hackrf_set_operacake_ports(
	hackrf_device* device,
	uint8_t address,
	uint8_t port_a,
	uint8_t port_b)
{
	USB_API_REQUIRED(device, 0x0102)

	if (!hackrf_operacake_valid_address(address)) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	int result;
	/* Error checking */
	if ((port_a > OPERACAKE_PB4) || (port_b > OPERACAKE_PB4)) {
		return HACKRF_ERROR_INVALID_PARAM;
	}
	/* Check which side PA and PB are on */
	if (((port_a <= OPERACAKE_PA4) && (port_b <= OPERACAKE_PA4)) ||
	    ((port_a > OPERACAKE_PA4) && (port_b > OPERACAKE_PA4))) {
		return HACKRF_ERROR_INVALID_PARAM;
	}
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_OPERACAKE_SET_PORTS,
		address,
		port_a | (port_b << 8),
		NULL,
		0,
		0);

	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_reset(hackrf_device* device)
{
	USB_API_REQUIRED(device, 0x0102)
	int result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_RESET,
		0,
		0,
		NULL,
		0,
		0);

	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

/**
 * @deprecated This has been replaced by @ref hackrf_set_operacake_freq_ranges
 */
int ADDCALL hackrf_set_operacake_ranges(
	hackrf_device* device,
	uint8_t* ranges,
	uint8_t len_ranges)
{
	USB_API_REQUIRED(device, 0x0103)

	int result;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_OPERACAKE_SET_RANGES,
		0,
		0,
		ranges,
		len_ranges,
		0);

	if (result < len_ranges) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_set_operacake_freq_ranges(
	hackrf_device* device,
	hackrf_operacake_freq_range* freq_ranges,
	uint8_t count)
{
	USB_API_REQUIRED(device, 0x0103)

	uint8_t range_bytes
		[HACKRF_OPERACAKE_MAX_FREQ_RANGES * sizeof(hackrf_operacake_freq_range)];
	uint8_t ptr;
	int i;
	for (i = 0; i < count; i++) {
		ptr = 5 * i;
		range_bytes[ptr] = freq_ranges[i].freq_min >> 8;
		range_bytes[ptr + 1] = freq_ranges[i].freq_min & 0xFF;
		range_bytes[ptr + 2] = freq_ranges[i].freq_max >> 8;
		range_bytes[ptr + 3] = freq_ranges[i].freq_max & 0xFF;
		range_bytes[ptr + 4] = freq_ranges[i].port;
	}

	int result;
	int len_ranges = count * 5;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_OPERACAKE_SET_RANGES,
		0,
		0,
		range_bytes,
		len_ranges,
		0);

	if (result < len_ranges) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

#define DWELL_TIME_SIZE 5
static uint8_t dwell_data[DWELL_TIME_SIZE * HACKRF_OPERACAKE_MAX_DWELL_TIMES];

int ADDCALL hackrf_set_operacake_dwell_times(
	hackrf_device* device,
	hackrf_operacake_dwell_time* dwell_times,
	uint8_t count)
{
	USB_API_REQUIRED(device, 0x0105)

	if (count > HACKRF_OPERACAKE_MAX_DWELL_TIMES) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	int i;
	for (i = 0; i < count; i++) {
		*(uint32_t*) &dwell_data[i * DWELL_TIME_SIZE] =
			TO_LE(dwell_times[i].dwell);
		dwell_data[(i * DWELL_TIME_SIZE) + 4] = dwell_times[i].port;
	}

	int data_len = count * DWELL_TIME_SIZE;
	int result;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_OPERACAKE_SET_DWELL_TIMES,
		0,
		0,
		dwell_data,
		data_len,
		0);

	if (result < data_len) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_set_clkout_enable(hackrf_device* device, const uint8_t value)
{
	USB_API_REQUIRED(device, 0x0103)
	int result;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_CLKOUT_ENABLE,
		value,
		0,
		NULL,
		0,
		0);

	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_get_clkin_status(hackrf_device* device, uint8_t* status)
{
	USB_API_REQUIRED(device, 0x0106)
	int result;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_GET_CLKIN_STATUS,
		0,
		0,
		status,
		1,
		0);
	if (result < 1) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_operacake_gpio_test(
	hackrf_device* device,
	const uint8_t address,
	uint16_t* test_result)
{
	USB_API_REQUIRED(device, 0x0103)

	if (!hackrf_operacake_valid_address(address)) {
		return HACKRF_ERROR_INVALID_PARAM;
	}

	int result;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_OPERACAKE_GPIO_TEST,
		address,
		0,
		(unsigned char*) test_result,
		2,
		0);

	if (result < 1) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

#ifdef HACKRF_ISSUE_609_IS_FIXED
int ADDCALL hackrf_cpld_checksum(hackrf_device* device, uint32_t* crc)
{
	USB_API_REQUIRED(device, 0x0103)
	uint8_t length;
	int result;

	length = sizeof(*crc);
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_CPLD_CHECKSUM,
		0,
		0,
		(unsigned char*) crc,
		length,
		0);

	if (result < length) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		*crc = TO_LE(*crc);
		return HACKRF_SUCCESS;
	}
}
#endif /* HACKRF_ISSUE_609_IS_FIXED */

int ADDCALL hackrf_set_ui_enable(hackrf_device* device, const uint8_t value)
{
	USB_API_REQUIRED(device, 0x0104)
	int result;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_UI_ENABLE,
		value,
		0,
		NULL,
		0,
		0);

	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_start_rx_sweep(
	hackrf_device* device,
	hackrf_sample_block_cb_fn callback,
	void* rx_ctx)
{
	USB_API_REQUIRED(device, 0x0104)
	int result;
	const uint8_t endpoint_address = RX_ENDPOINT_ADDRESS;
	result = hackrf_set_transceiver_mode(device, TRANSCEIVER_MODE_RX_SWEEP);
	if (HACKRF_SUCCESS == result) {
		device->rx_ctx = rx_ctx;
		result = prepare_setup_transfers(device, endpoint_address, callback);
	}
	return result;
}

/**
 * Get USB transfer buffer size.
 * @return size in bytes
 */
size_t ADDCALL hackrf_get_transfer_buffer_size(hackrf_device* device)
{
	(void) device;
	return TRANSFER_BUFFER_SIZE;
}

/**
 * Get the total number of USB transfer buffers.
 * @return number of buffers
 */
uint32_t ADDCALL hackrf_get_transfer_queue_depth(hackrf_device* device)
{
	(void) device;
	return TRANSFER_COUNT;
}

int ADDCALL hackrf_board_rev_read(hackrf_device* device, uint8_t* value)
{
	USB_API_REQUIRED(device, 0x0106)
	int result;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_BOARD_REV_READ,
		0,
		0,
		value,
		1,
		0);

	if (result < 1) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

extern ADDAPI const char* ADDCALL hackrf_board_rev_name(enum hackrf_board_rev board_rev)
{
	switch (board_rev) {
	case BOARD_REV_HACKRF1_OLD:
		return "older than r6";

	case BOARD_REV_HACKRF1_R6:
	case BOARD_REV_GSG_HACKRF1_R6:
		return "r6";

	case BOARD_REV_HACKRF1_R7:
	case BOARD_REV_GSG_HACKRF1_R7:
		return "r7";

	case BOARD_REV_HACKRF1_R8:
	case BOARD_REV_GSG_HACKRF1_R8:
		return "r8";

	case BOARD_REV_HACKRF1_R9:
	case BOARD_REV_GSG_HACKRF1_R9:
		return "r9";

	case BOARD_REV_HACKRF1_R10:
	case BOARD_REV_GSG_HACKRF1_R10:
		return "r10";

	case BOARD_ID_UNRECOGNIZED:
		return "unrecognized";

	case BOARD_ID_UNDETECTED:
		return "undetected";

	default:
		return "unknown";
	}
}

int ADDCALL hackrf_supported_platform_read(hackrf_device* device, uint32_t* value)
{
	unsigned char data[4];
	USB_API_REQUIRED(device, 0x0106)
	int result;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SUPPORTED_PLATFORM_READ,
		0,
		0,
		&data[0],
		4,
		0);

	if (result < 1) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		*value = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_set_leds(hackrf_device* device, const uint8_t state)
{
	USB_API_REQUIRED(device, 0x0107)
	int result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SET_LEDS,
		state,
		0,
		NULL,
		0,
		0);

	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

int ADDCALL hackrf_set_user_bias_t_opts(
	hackrf_device* device,
	hackrf_bias_t_user_settting_req* req)
{
	USB_API_REQUIRED(device, 0x0108)
	uint16_t state = 0; // Assume no modifications
	if (req->off.do_update) {
		state |= 0x4;
		if (req->off.change_on_mode_entry) {
			state |= 0x2 + req->off.enabled;
		}
	}

	if (req->rx.do_update) {
		state |= 0x20;
		if (req->rx.change_on_mode_entry) {
			state |= 0x10 + (req->rx.enabled << 3);
		}
	}

	if (req->tx.do_update) {
		state |= 0x100;
		if (req->tx.change_on_mode_entry) {
			state |= 0x80 + (req->tx.enabled << 6);
		}
	}

	int result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SET_USER_BIAS_T_OPTS,
		state,
		0,
		NULL,
		0,
		0);

	if (result != 0) {
		last_libusb_error = result;
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

#ifdef __cplusplus
} // __cplusplus defined.
#endif
