/*
 * Unit tests for hackrf_spiflash_clear_status()
 *
 * This exercises the "-C fix": the control transfer must use
 * DEFAULT_REQUEST_TIMEOUT (100 ms) instead of the original 0 ms
 * timeout that caused hangs.
 */

#include <assert.h>
#include <stdio.h>
#include <libusb.h>
#include "hackrf.h"
#include "mock_libusb.h"

#define SPIFLASH_CLEAR_STATUS_REQUEST 34

static void test_success(void)
{
	hackrf_device* device;
	int result;

	result = hackrf_open(&device);
	assert(result == HACKRF_SUCCESS);

	mock_reset();
	mock_set_ctrl_result(MOCK_CTRL_SUCCESS);

	result = hackrf_spiflash_clear_status(device);
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

	mock_set_ctrl_result(MOCK_CTRL_SUCCESS);
	hackrf_close(device);
}

static void test_stall(void)
{
	hackrf_device* device;
	int result;

	result = hackrf_open(&device);
	assert(result == HACKRF_SUCCESS);

	mock_reset();
	mock_set_ctrl_result(MOCK_CTRL_STALL);

	result = hackrf_spiflash_clear_status(device);
	assert(result == HACKRF_ERROR_LIBUSB);

	printf("PASS: hackrf_spiflash_clear_status STALL returns LIBUSB error\n");

	mock_set_ctrl_result(MOCK_CTRL_SUCCESS);
	hackrf_close(device);
}

int main(void)
{
	printf("Running hackrf_spiflash_clear_status tests...\n");

	hackrf_init();
	test_success();
	test_stall();
	hackrf_exit();

	printf("\nAll tests passed.\n");
	return 0;
}
