/*
 * Unit tests for hackrf_spiflash_clear_status()
 *
 * This exercises the "-C fix": the control transfer must use
 * DEFAULT_REQUEST_TIMEOUT (100 ms) instead of the original 0 ms
 * timeout that caused hangs.
 */

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "hackrf.h"
#include "mock_libusb.h"

#ifndef LIBUSB_ENDPOINT_OUT
#define LIBUSB_ENDPOINT_OUT 0x00
#endif
#ifndef LIBUSB_REQUEST_TYPE_VENDOR
#define LIBUSB_REQUEST_TYPE_VENDOR 0x40
#endif
#ifndef LIBUSB_RECIPIENT_DEVICE
#define LIBUSB_RECIPIENT_DEVICE 0x00
#endif

#define SPIFLASH_CLEAR_STATUS_REQUEST 34

/* Minimal prefix of the real struct to satisfy internal field access */
struct libusb_device_handle;

struct hackrf_device {
	struct libusb_device_handle* usb_device;
	uint16_t usb_api_version;
};

static hackrf_device* create_device(uint16_t api_version)
{
	hackrf_device* d = calloc(1, sizeof(*d));
	d->usb_device = (struct libusb_device_handle*)0x1234;
	d->usb_api_version = api_version;
	return d;
}

static void test_success(void)
{
	mock_transfer_t t;
	int result;
	hackrf_device* dev;

	mock_libusb_reset();

	dev = create_device(0x0103);

	memset(&t, 0, sizeof(t));
	t.request = SPIFLASH_CLEAR_STATUS_REQUEST;
	t.value = 0;
	t.index = 0;
	t.expected_length = 0;
	t.return_code = 0;
	mock_libusb_queue_transfer(&t);

	result = hackrf_spiflash_clear_status(dev);
	assert(result == HACKRF_SUCCESS);

	/* The -C fix: timeout must be DEFAULT_REQUEST_TIMEOUT (100), not 0 */
	assert(mock_last_ctrl.timeout == 100);
	assert(mock_last_ctrl.bRequest == SPIFLASH_CLEAR_STATUS_REQUEST);
	assert(mock_last_ctrl.wLength == 0);
	assert(mock_last_ctrl.bmRequestType ==
	       (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
		LIBUSB_RECIPIENT_DEVICE));

	printf("PASS: hackrf_spiflash_clear_status success (timeout=%u ms, "
	       "request=%d)\n",
	       mock_last_ctrl.timeout,
	       mock_last_ctrl.bRequest);

	free(dev);
}

static void test_stall(void)
{
	mock_transfer_t t;
	int result;
	hackrf_device* dev;

	mock_libusb_reset();

	dev = create_device(0x0103);

	memset(&t, 0, sizeof(t));
	t.request = SPIFLASH_CLEAR_STATUS_REQUEST;
	t.value = 0;
	t.index = 0;
	t.expected_length = 0;
	t.return_code = LIBUSB_ERROR_PIPE;
	mock_libusb_queue_transfer(&t);

	result = hackrf_spiflash_clear_status(dev);
	assert(result == HACKRF_ERROR_LIBUSB);

	printf("PASS: hackrf_spiflash_clear_status STALL returns LIBUSB error\n");

	free(dev);
}

int main(void)
{
	printf("Running hackrf_spiflash_clear_status tests...\n");

	test_success();
	test_stall();

	printf("\nAll tests passed.\n");
	return 0;
}
