#ifndef __USB_H__
#define __USB_H__

// TODO: Refactor to support high performance operations without having to
// expose usb_transfer_descriptor_t. Or usb_endpoint_prime(). Or, or, or...
#include <libopencm3/lpc43xx/usb.h>

#include "usb_type.h"

#define ATTR_ALIGNED(x)	__attribute__ ((aligned(x)))
#define ATTR_SECTION(x) __attribute__ ((section(x)))

void usb_peripheral_reset();

void usb_device_init(
	const uint_fast8_t device_ordinal,
	usb_device_t* const device
);

void usb_run(
	usb_device_t* const device
);

void usb_run_tasks(
	const usb_device_t* const device
);

usb_speed_t usb_speed(
	const usb_device_t* const device
);

void usb_set_address_immediate(
	const usb_device_t* const device,
	const uint_fast8_t address
);

void usb_set_address_deferred(
	const usb_device_t* const device,
	const uint_fast8_t address
);

void usb_endpoint_init(
	const usb_endpoint_t* const endpoint
);

void usb_endpoint_schedule(
	const usb_endpoint_t* const endpoint,
	void* const data,
	const uint32_t maximum_length
);

void usb_endpoint_schedule_ack(
	const usb_endpoint_t* const endpoint
);

void usb_endpoint_stall(
	const usb_endpoint_t* const endpoint
);

void usb_endpoint_disable(
	const usb_endpoint_t* const endpoint
);

void usb_endpoint_flush(
	const usb_endpoint_t* const endpoint
);

bool usb_endpoint_is_ready(
	const usb_endpoint_t* const endpoint
);

void usb_endpoint_prime(
	const usb_endpoint_t* const endpoint,
	usb_transfer_descriptor_t* const first_td
);

#endif//__USB_H__
