#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "hackrf.h"
#include "mock_libusb.h"

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

/* Test 1: successful checksum read */
static void test_cpld_checksum_success(void)
{
	unsigned char crc_bytes[4];
	mock_transfer_t t;
	uint32_t crc;
	int result;
	hackrf_device* dev;

	mock_libusb_reset();

	dev = create_device(0x0103);

	crc_bytes[0] = 0xEF;
	crc_bytes[1] = 0xBE;
	crc_bytes[2] = 0xAD;
	crc_bytes[3] = 0xDE;

	memset(&t, 0, sizeof(t));
	t.request = 36; /* HACKRF_VENDOR_REQUEST_CPLD_CHECKSUM */
	t.value = 0;
	t.index = 0;
	t.expected_length = 4;
	t.response_data = crc_bytes;
	t.response_length = 4;
	t.return_code = 4;
	mock_libusb_queue_transfer(&t);

	crc = 0;
	result = hackrf_cpld_checksum(dev, &crc);
	assert(result == HACKRF_SUCCESS);
	assert(crc == 0xDEADBEEF);
	printf("PASS: hackrf_cpld_checksum success\n");

	free(dev);
}

/* Test 2: device STALLs */
static void test_cpld_checksum_stall(void)
{
	mock_transfer_t t;
	uint32_t crc;
	int result;
	hackrf_device* dev;

	mock_libusb_reset();

	dev = create_device(0x0103);

	memset(&t, 0, sizeof(t));
	t.request = 36;
	t.value = 0;
	t.index = 0;
	t.expected_length = 4;
	t.return_code = LIBUSB_ERROR_PIPE;
	mock_libusb_queue_transfer(&t);

	crc = 0;
	result = hackrf_cpld_checksum(dev, &crc);
	assert(result == HACKRF_ERROR_LIBUSB);
	printf("PASS: hackrf_cpld_checksum stall\n");

	free(dev);
}

/* Test 3: timeout */
static void test_cpld_checksum_timeout(void)
{
	mock_transfer_t t;
	uint32_t crc;
	int result;
	hackrf_device* dev;

	mock_libusb_reset();

	dev = create_device(0x0103);

	memset(&t, 0, sizeof(t));
	t.request = 36;
	t.value = 0;
	t.index = 0;
	t.expected_length = 4;
	t.return_code = LIBUSB_ERROR_TIMEOUT;
	mock_libusb_queue_transfer(&t);

	crc = 0;
	result = hackrf_cpld_checksum(dev, &crc);
	assert(result == HACKRF_ERROR_LIBUSB);
	printf("PASS: hackrf_cpld_checksum timeout\n");

	free(dev);
}

int main(void)
{
	printf("Running hackrf_cpld_checksum tests...\n");

	test_cpld_checksum_success();
	test_cpld_checksum_stall();
	test_cpld_checksum_timeout();

	printf("\nAll tests passed.\n");
	return 0;
}
