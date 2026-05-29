#ifndef MOCK_LIBUSB_H
#define MOCK_LIBUSB_H

#include <stdint.h>

#ifndef LIBUSB_ERROR_PIPE
#define LIBUSB_ERROR_PIPE -9
#endif

#ifndef LIBUSB_ERROR_TIMEOUT
#define LIBUSB_ERROR_TIMEOUT -7
#endif

typedef struct {
	uint8_t request;
	uint16_t value;
	uint16_t index;
	uint16_t expected_length;
	const unsigned char *response_data;
	uint16_t response_length;
	int return_code;
} mock_transfer_t;

void mock_libusb_init(void);
void mock_libusb_queue_transfer(const mock_transfer_t *transfer);
void mock_libusb_reset(void);

#endif /* MOCK_LIBUSB_H */
