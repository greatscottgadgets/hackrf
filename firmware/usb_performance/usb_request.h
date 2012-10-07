#ifndef __USB_REQUEST_H__
#define __USB_REQUEST_H__

#include "usb_type.h"

typedef enum {
	USB_RESPONSE_NONE,
	USB_RESPONSE_IN,
	USB_RESPONSE_OUT,
	USB_RESPONSE_STALL,
} usb_endpoint_type_t;

typedef enum {
	USB_TRANSFER_STAGE_SETUP,
	USB_TRANSFER_STAGE_DATA,
	USB_TRANSFER_STAGE_STATUS,
} usb_transfer_stage_t;

typedef void (*usb_request_handler_fn)(
	usb_endpoint_t* const response,
	const usb_transfer_stage_t stage
);

typedef struct {
	usb_request_handler_fn standard;
	usb_request_handler_fn class;
	usb_request_handler_fn vendor;
	usb_request_handler_fn reserved;
} usb_request_handlers_t;

extern const usb_request_handlers_t usb_request_handlers;

void usb_setup_complete(
	usb_endpoint_t* const endpoint
);

void usb_control_in_complete(
	usb_endpoint_t* const endpoint
);

void usb_control_out_complete(
	usb_endpoint_t* const endpoint
);

#endif//__USB_REQUEST_H__
