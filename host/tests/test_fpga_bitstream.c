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

/* Test 1: successful switch */
static void test_fpga_bitstream_success(void)
{
	mock_transfer_t t;
	int result;
	hackrf_device* dev;

	mock_libusb_reset();

	dev = create_device(0x0109);

	memset(&t, 0, sizeof(t));
	t.request = 54; /* HACKRF_VENDOR_REQUEST_SET_FPGA_BITSTREAM */
	t.value = 1;
	t.index = 0;
	t.expected_length = 0;
	t.return_code = 0;
	mock_libusb_queue_transfer(&t);

	result = hackrf_set_fpga_bitstream(dev, 1);
	assert(result == HACKRF_SUCCESS);
	printf("PASS: hackrf_set_fpga_bitstream success\n");

	free(dev);
}

/* Test 2: device STALLs (streaming active) */
static void test_fpga_bitstream_stall(void)
{
	mock_transfer_t t;
	int result;
	hackrf_device* dev;

	mock_libusb_reset();

	dev = create_device(0x0109);

	memset(&t, 0, sizeof(t));
	t.request = 54;
	t.value = 2;
	t.index = 0;
	t.expected_length = 0;
	t.return_code = LIBUSB_ERROR_PIPE;
	mock_libusb_queue_transfer(&t);

	result = hackrf_set_fpga_bitstream(dev, 2);
	assert(result == HACKRF_ERROR_LIBUSB);
	printf("PASS: hackrf_set_fpga_bitstream stall\n");

	free(dev);
}

int main(void)
{
	printf("Running hackrf_set_fpga_bitstream tests...\n");

	test_fpga_bitstream_success();
	test_fpga_bitstream_stall();

	printf("\nAll tests passed.\n");
	return 0;
}
