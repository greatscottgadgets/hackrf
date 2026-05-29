#include "mock_libusb.h"
#include <string.h>

#define MAX_MOCK_TRANSFERS 16

struct libusb_device_handle;

static mock_transfer_t transfer_queue[MAX_MOCK_TRANSFERS];
static int queue_len = 0;

void mock_libusb_init(void)
{
	queue_len = 0;
}

void mock_libusb_reset(void)
{
	queue_len = 0;
}

void mock_libusb_queue_transfer(const mock_transfer_t *transfer)
{
	if (queue_len < MAX_MOCK_TRANSFERS) {
		transfer_queue[queue_len] = *transfer;
		queue_len++;
	}
}

/* Minimal replacement for libusb_control_transfer */
int libusb_control_transfer(
	struct libusb_device_handle *dev_handle,
	uint8_t bmRequestType,
	uint8_t bRequest,
	uint16_t wValue,
	uint16_t wIndex,
	unsigned char *data,
	uint16_t wLength,
	unsigned int timeout)
{
	int i;

	(void) dev_handle;
	(void) bmRequestType;
	(void) wLength;
	(void) timeout;

	for (i = 0; i < queue_len; i++) {
		if (transfer_queue[i].request == bRequest &&
		    transfer_queue[i].value == wValue) {
			if (transfer_queue[i].return_code < 0) {
				return transfer_queue[i].return_code;
			}
			if (data != NULL && transfer_queue[i].response_data != NULL) {
				uint16_t copy_len;
				if (wLength < transfer_queue[i].response_length) {
					copy_len = wLength;
				} else {
					copy_len = transfer_queue[i].response_length;
				}
				memcpy(data, transfer_queue[i].response_data, copy_len);
				return transfer_queue[i].return_code;
			}
			return transfer_queue[i].return_code;
		}
	}

	return LIBUSB_ERROR_PIPE;
}
