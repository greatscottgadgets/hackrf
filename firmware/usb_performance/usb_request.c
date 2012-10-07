#include "usb_request.h"

#include <stdbool.h>

static void usb_request(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage
) {
	usb_request_handler_fn handler = 0;
	
	switch( endpoint->setup.request_type & USB_SETUP_REQUEST_TYPE_mask ) {
	case USB_SETUP_REQUEST_TYPE_STANDARD:
		handler = usb_request_handlers.standard;
		break;
	
	case USB_SETUP_REQUEST_TYPE_CLASS:
		handler = usb_request_handlers.class;
		break;
	
	case USB_SETUP_REQUEST_TYPE_VENDOR:
		handler = usb_request_handlers.vendor;
		break;
		
	case USB_SETUP_REQUEST_TYPE_RESERVED:
		handler = usb_request_handlers.reserved;
		break;
	}
	
	if( handler ) {
		handler(endpoint, stage);
	}
}

void usb_setup_complete(
	usb_endpoint_t* const endpoint
) {
	usb_request(endpoint, USB_TRANSFER_STAGE_SETUP);
}

void usb_control_out_complete(
	usb_endpoint_t* const endpoint
) {
	const bool device_to_host =
		endpoint->setup.request_type >> USB_SETUP_REQUEST_TYPE_DATA_TRANSFER_DIRECTION_shift;
	if( device_to_host ) {
		usb_request(endpoint, USB_TRANSFER_STAGE_STATUS);
	} else {
		usb_request(endpoint, USB_TRANSFER_STAGE_DATA);
	}
}

void usb_control_in_complete(
	usb_endpoint_t* const endpoint
) {
	const bool device_to_host =
		endpoint->setup.request_type >> USB_SETUP_REQUEST_TYPE_DATA_TRANSFER_DIRECTION_shift;
	if( device_to_host ) {
		usb_request(endpoint, USB_TRANSFER_STAGE_DATA);
	} else {
		usb_request(endpoint, USB_TRANSFER_STAGE_STATUS);
	}
}

