/*
 * Copyright 2012 Jared Boone
 *
 * This file is part of HackRF.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <stdint.h>
#include <stdbool.h>

#include "usb.h"
#include "usb_type.h"
#include "usb_queue.h"
#include "usb_standard_request.h"

#include <libopencm3/lpc43xx/creg.h>
#include <libopencm3/lpc43xx/m4/nvic.h>
#include <libopencm3/lpc43xx/rgu.h>
#include <libopencm3/lpc43xx/usb.h>

usb_device_t* usb_device_usb0 = 0;

usb_queue_head_t usb_qh[12] ATTR_ALIGNED(2048);

#define USB_QH_INDEX(endpoint_address) (((endpoint_address & 0xF) * 2) + ((endpoint_address >> 7) & 1))

usb_queue_head_t* usb_queue_head(
	const uint_fast8_t endpoint_address
) {
	return &usb_qh[USB_QH_INDEX(endpoint_address)];
}

static usb_endpoint_t* usb_endpoint_from_address(
	const uint_fast8_t endpoint_address
) {
	return (usb_endpoint_t*)usb_queue_head(endpoint_address)->_reserved_0;
}

static uint_fast8_t usb_endpoint_address(
	const usb_transfer_direction_t direction,
	const uint_fast8_t number
) {
	return ((direction == USB_TRANSFER_DIRECTION_IN) ? 0x80 : 0x00) + number;
}

static bool usb_endpoint_is_in(const uint_fast8_t endpoint_address) {
	return (endpoint_address & 0x80) ? true : false;
}

static uint_fast8_t usb_endpoint_number(const uint_fast8_t endpoint_address) {
	return (endpoint_address & 0xF);
}

void usb_peripheral_reset() {
	RESET_CTRL0 = RESET_CTRL0_USB0_RST;
	RESET_CTRL0 = 0;
	
	while( (RESET_ACTIVE_STATUS0 & RESET_CTRL0_USB0_RST) == 0 );
}

static void usb_phy_enable() {
	CREG_CREG0 &= ~CREG_CREG0_USB0PHY;
}

static void usb_clear_pending_interrupts(const uint32_t mask) {
	USB0_ENDPTNAK = mask;
	USB0_ENDPTNAKEN = mask;
	USB0_USBSTS_D = mask;
	USB0_ENDPTSETUPSTAT = USB0_ENDPTSETUPSTAT & mask;
	USB0_ENDPTCOMPLETE = USB0_ENDPTCOMPLETE & mask;
}

static void usb_clear_all_pending_interrupts() {
	usb_clear_pending_interrupts(0xFFFFFFFF);
}

static void usb_wait_for_endpoint_priming_to_finish(const uint32_t mask) {
	// Wait until controller has parsed new transfer descriptors and prepared
	// receive buffers.
	while( USB0_ENDPTPRIME & mask );
}

static void usb_flush_endpoints(const uint32_t mask) {
	// Clear any primed buffers. If a packet is in progress, that transfer
	// will continue until completion.
	USB0_ENDPTFLUSH = mask;
}

static void usb_wait_for_endpoint_flushing_to_finish(const uint32_t mask) {
	// Wait until controller has flushed all endpoints / cleared any primed
	// buffers.
	while( USB0_ENDPTFLUSH & mask );
}

static void usb_flush_primed_endpoints(const uint32_t mask) {
	usb_wait_for_endpoint_priming_to_finish(mask);
	usb_flush_endpoints(mask);
	usb_wait_for_endpoint_flushing_to_finish(mask);
}

static void usb_flush_all_primed_endpoints() {
	usb_flush_primed_endpoints(0xFFFFFFFF);
}

static void usb_endpoint_set_type(
	const usb_endpoint_t* const endpoint,
	const usb_transfer_type_t transfer_type
) {
	// NOTE: UM10503 section 23.6.24 "Endpoint 1 to 5 control registers" says
	// that the disabled side of an endpoint must be set to a non-control type
	// (e.g. bulk, interrupt, or iso).
	const uint_fast8_t endpoint_number = usb_endpoint_number(endpoint->address);
	USB0_ENDPTCTRL(endpoint_number)
		= ( USB0_ENDPTCTRL(endpoint_number)
		  & ~(USB0_ENDPTCTRL_TXT1_0_MASK | USB0_ENDPTCTRL_RXT_MASK)
		  )
		| ( USB0_ENDPTCTRL_TXT1_0(transfer_type)
		  | USB0_ENDPTCTRL_RXT(transfer_type)
		  );
}

static void usb_endpoint_enable(
	const usb_endpoint_t* const endpoint
) {
	const uint_fast8_t endpoint_number = usb_endpoint_number(endpoint->address);
	if( usb_endpoint_is_in(endpoint->address) ) {
		USB0_ENDPTCTRL(endpoint_number) |= (USB0_ENDPTCTRL_TXE | USB0_ENDPTCTRL_TXR);
	} else {
		USB0_ENDPTCTRL(endpoint_number) |= (USB0_ENDPTCTRL_RXE | USB0_ENDPTCTRL_RXR);
	}
}

static void usb_endpoint_clear_pending_interrupts(
	const usb_endpoint_t* const endpoint
) {
	const uint_fast8_t endpoint_number = usb_endpoint_number(endpoint->address);
	if( usb_endpoint_is_in(endpoint->address) ) {
		usb_clear_pending_interrupts(USB0_ENDPTCOMPLETE_ETCE(1 << endpoint_number));
	} else {
		usb_clear_pending_interrupts(USB0_ENDPTCOMPLETE_ERCE(1 << endpoint_number));
	}
}

void usb_endpoint_disable(
	const usb_endpoint_t* const endpoint
) {
	const uint_fast8_t endpoint_number = usb_endpoint_number(endpoint->address);
	if( usb_endpoint_is_in(endpoint->address) ) {
		USB0_ENDPTCTRL(endpoint_number) &= ~(USB0_ENDPTCTRL_TXE);
	} else {
		USB0_ENDPTCTRL(endpoint_number) &= ~(USB0_ENDPTCTRL_RXE);
	}
        usb_queue_flush_endpoint(endpoint);
	usb_endpoint_clear_pending_interrupts(endpoint);
	usb_endpoint_flush(endpoint);
}

void usb_endpoint_prime(
	const usb_endpoint_t* const endpoint,
	usb_transfer_descriptor_t* const first_td	
) {
	usb_queue_head_t* const qh = usb_queue_head(endpoint->address);
	
	qh->next_dtd_pointer = first_td;
	qh->total_bytes
		&= ~( USB_TD_DTD_TOKEN_STATUS_ACTIVE
		    | USB_TD_DTD_TOKEN_STATUS_HALTED
			)
		;
	
	const uint_fast8_t endpoint_number = usb_endpoint_number(endpoint->address);
	if( usb_endpoint_is_in(endpoint->address) ) {
		USB0_ENDPTPRIME = USB0_ENDPTPRIME_PETB(1 << endpoint_number);
	} else {
		USB0_ENDPTPRIME = USB0_ENDPTPRIME_PERB(1 << endpoint_number);
	}
}

static bool usb_endpoint_is_priming(
	const usb_endpoint_t* const endpoint
) {
	const uint_fast8_t endpoint_number = usb_endpoint_number(endpoint->address);
	if( usb_endpoint_is_in(endpoint->address) ) {
		return USB0_ENDPTPRIME & USB0_ENDPTPRIME_PETB(1 << endpoint_number);
	} else {
		return USB0_ENDPTPRIME & USB0_ENDPTPRIME_PERB(1 << endpoint_number);
	}
}

// Schedule an already filled-in transfer descriptor for execution on
// the given endpoint, waiting until the endpoint has finished.
void usb_endpoint_schedule_wait(
	const usb_endpoint_t* const endpoint,
	usb_transfer_descriptor_t* const td
) {
	// Ensure that endpoint is ready to be primed.
	// It may have been flushed due to an aborted transaction.
	// TODO: This should be preceded by a flush?
	while( usb_endpoint_is_ready(endpoint) );

	td->next_dtd_pointer = USB_TD_NEXT_DTD_POINTER_TERMINATE;

	usb_endpoint_prime(endpoint, td);
}

// Schedule an already filled-in transfer descriptor for execution on
// the given endpoint, appending to the end of the endpoint's queue if
// there are pending TDs. Note that this requires that one knows the
// tail of the endpoint's TD queue. Moreover, the user is responsible
// for setting the TERMINATE bit of next_dtd_pointer if needed.
void usb_endpoint_schedule_append(
	const usb_endpoint_t* const endpoint,
	usb_transfer_descriptor_t* const tail_td,
	usb_transfer_descriptor_t* const new_td
) {
	bool done;

	tail_td->next_dtd_pointer = new_td;

	if (usb_endpoint_is_priming(endpoint)) {
		return;
	}

	do {
		USB0_USBCMD_D |= USB0_USBCMD_D_ATDTW;
		done = usb_endpoint_is_ready(endpoint);
	} while (!(USB0_USBCMD_D & USB0_USBCMD_D_ATDTW));

	USB0_USBCMD_D &= ~USB0_USBCMD_D_ATDTW;
	if(!done) {
		usb_endpoint_prime(endpoint, new_td);
	}
}

void usb_endpoint_flush(
	const usb_endpoint_t* const endpoint
) {
	const uint_fast8_t endpoint_number = usb_endpoint_number(endpoint->address);
	usb_queue_flush_endpoint(endpoint);
	if( usb_endpoint_is_in(endpoint->address) ) {
		usb_flush_primed_endpoints(USB0_ENDPTFLUSH_FETB(1 << endpoint_number));
	} else {
		usb_flush_primed_endpoints(USB0_ENDPTFLUSH_FERB(1 << endpoint_number));
	}
}
/*
static bool usb_endpoint_is_flushing(
	const usb_endpoint_t* const endpoint
) {
	const uint_fast8_t endpoint_number = usb_endpoint_number(endpoint->address);
	if( usb_endpoint_is_in(endpoint->address) ) {
		return USB0_ENDPTFLUSH & USB0_ENDPTFLUSH_FETB(1 << endpoint_number);
	} else {
		return USB0_ENDPTFLUSH & USB0_ENDPTFLUSH_FERB(1 << endpoint_number);
	}
}
*/
bool usb_endpoint_is_ready(
	const usb_endpoint_t* const endpoint
) {
	const uint_fast8_t endpoint_number = usb_endpoint_number(endpoint->address);
	if( usb_endpoint_is_in(endpoint->address) ) {
		return USB0_ENDPTSTAT & USB0_ENDPTSTAT_ETBR(1 << endpoint_number);
	} else {
		return USB0_ENDPTSTAT & USB0_ENDPTSTAT_ERBR(1 << endpoint_number);
	}
}

bool usb_endpoint_is_complete(
	const usb_endpoint_t* const endpoint
) {
	const uint_fast8_t endpoint_number = usb_endpoint_number(endpoint->address);
	if( usb_endpoint_is_in(endpoint->address) ) {
		return USB0_ENDPTCOMPLETE & USB0_ENDPTCOMPLETE_ETCE(1 << endpoint_number);
	} else {
		return USB0_ENDPTCOMPLETE & USB0_ENDPTCOMPLETE_ERCE(1 << endpoint_number);
	}
}

void usb_endpoint_stall(
	const usb_endpoint_t* const endpoint
) {
	// Endpoint is to be stalled as a pair -- both OUT and IN.
	// See UM10503 section 23.10.5.2 "Stalling"
	const uint_fast8_t endpoint_number = usb_endpoint_number(endpoint->address);
	USB0_ENDPTCTRL(endpoint_number) |= (USB0_ENDPTCTRL_RXS | USB0_ENDPTCTRL_TXS);
	
	// TODO: Also need to reset data toggle in both directions?
}

static void usb_controller_run() {
	USB0_USBCMD_D |= USB0_USBCMD_D_RS;
}

static void usb_controller_stop() {
	USB0_USBCMD_D &= ~USB0_USBCMD_D_RS;
}

static uint_fast8_t usb_controller_is_resetting() {
	return (USB0_USBCMD_D & USB0_USBCMD_D_RST) != 0;
}

static void usb_controller_set_device_mode() {
	// Set USB0 peripheral mode
	USB0_USBMODE_D = USB0_USBMODE_D_CM1_0(2);
	
	// Set device-related OTG flags
	// OTG termination: controls pull-down on USB_DM
	// VBUS_Discharge: VBUS discharges through resistor
	USB0_OTGSC = USB0_OTGSC_OT | USB0_OTGSC_VD;
}

usb_speed_t usb_speed(
	const usb_device_t* const device
) {
	if( device == usb_device_usb0 ) {
		switch( USB0_PORTSC1_D & USB0_PORTSC1_D_PSPD_MASK ) {
		case USB0_PORTSC1_D_PSPD(0):
			return USB_SPEED_FULL;
		
		case USB0_PORTSC1_D_PSPD(2):
			return USB_SPEED_HIGH;
		
		default:
			// TODO: What to do/return here? Is this even possible?
			return USB_SPEED_FULL;
		}
	} else {
		// TODO: This should not be possible with a more class-like
		// implementation.
		return USB_SPEED_FULL;
	}
}

static void usb_clear_status(const uint32_t status) {
	USB0_USBSTS_D = status;
}

static uint32_t usb_get_status() {
    // Mask status flags with enabled flag interrupts.
	const uint32_t status = USB0_USBSTS_D & USB0_USBINTR_D;

    // Clear flags that were just read, leaving alone any flags that
    // were just set (after the read). It's important to read and
    // reset flags atomically! :-)
	usb_clear_status(status);

	return status;
}

static void usb_clear_endpoint_setup_status(const uint32_t endpoint_setup_status) {
	USB0_ENDPTSETUPSTAT = endpoint_setup_status;
}

static uint32_t usb_get_endpoint_setup_status() {
 	return USB0_ENDPTSETUPSTAT;
}

static void usb_clear_endpoint_complete(const uint32_t endpoint_complete) {
	USB0_ENDPTCOMPLETE = endpoint_complete;
}

static uint32_t usb_get_endpoint_complete() {
	return USB0_ENDPTCOMPLETE;
}

static void usb_disable_all_endpoints() {
	// Endpoint 0 is always enabled. TODO: So why set ENDPTCTRL0?
	USB0_ENDPTCTRL0 &= ~(USB0_ENDPTCTRL0_RXE | USB0_ENDPTCTRL0_TXE);
	USB0_ENDPTCTRL1 &= ~(USB0_ENDPTCTRL1_RXE | USB0_ENDPTCTRL1_TXE);
	USB0_ENDPTCTRL2 &= ~(USB0_ENDPTCTRL2_RXE | USB0_ENDPTCTRL2_TXE);
	USB0_ENDPTCTRL3 &= ~(USB0_ENDPTCTRL3_RXE | USB0_ENDPTCTRL3_TXE);
	USB0_ENDPTCTRL4 &= ~(USB0_ENDPTCTRL4_RXE | USB0_ENDPTCTRL4_TXE);
	USB0_ENDPTCTRL5 &= ~(USB0_ENDPTCTRL5_RXE | USB0_ENDPTCTRL5_TXE);
}

void usb_set_address_immediate(
	const usb_device_t* const device,
	const uint_fast8_t address
) {
	if( device == usb_device_usb0 ) {
		USB0_DEVICEADDR = USB0_DEVICEADDR_USBADR(address);
	}
}

void usb_set_address_deferred(
	const usb_device_t* const device,
	const uint_fast8_t address
) {
	if( device == usb_device_usb0 ) {
		USB0_DEVICEADDR
			= USB0_DEVICEADDR_USBADR(address)
		    | USB0_DEVICEADDR_USBADRA
			;
	}
}

static void usb_reset_all_endpoints() {
	usb_disable_all_endpoints();
	usb_clear_all_pending_interrupts();
	usb_flush_all_primed_endpoints();
}

static void usb_controller_reset() {
	// TODO: Good to disable some USB interrupts to avoid priming new
	// new endpoints before the controller is reset?
	usb_reset_all_endpoints();
	usb_controller_stop();

	// Reset controller. Resets internal pipelines, timers, counters, state
	// machines to initial values. Not recommended when device is in attached
	// state -- effect on attached host is undefined. Detach first by flushing
	// all primed endpoints and stopping controller.
	USB0_USBCMD_D = USB0_USBCMD_D_RST;

	while( usb_controller_is_resetting() );
}

static void usb_bus_reset(usb_device_t* const device) {
	// According to UM10503 v1.4 section 23.10.3 "Bus reset":
	usb_reset_all_endpoints();
	usb_set_address_immediate(device, 0);
	usb_set_configuration(device, 0);
	
	// TODO: Enable endpoint 0, which might not actually be necessary,
	// as the datasheet claims it can't be disabled.

	//wait_ms(3);
	//
	//if( USB0_PORTSC1 & USB0_PORTSC1_PR ) {
	//	// Port still is in the reset state.
	//} else {
	//	usb_hardware_reset();
	//}
}

static void usb_interrupt_enable(
	usb_device_t* const device
) {
	if( device == usb_device_usb0 ) {
		nvic_enable_irq(NVIC_USB0_IRQ);
	}
}

void usb_device_init(
	const uint_fast8_t device_ordinal,
	usb_device_t* const device
) {
	if( device_ordinal == 0 ) {
		usb_device_usb0 = device;
	
		usb_phy_enable();
		usb_controller_reset();
		usb_controller_set_device_mode();
	
		// Set interrupt threshold interval to 0
		USB0_USBCMD_D &= ~USB0_USBCMD_D_ITC_MASK;

		// Configure endpoint list address 
		USB0_ENDPOINTLISTADDR = (uint32_t)usb_qh;
	
		// Enable interrupts
		USB0_USBINTR_D =
			  USB0_USBINTR_D_UE
			| USB0_USBINTR_D_UEE
			| USB0_USBINTR_D_PCE
			| USB0_USBINTR_D_URE
			//| USB0_USBINTR_D_SRE
			| USB0_USBINTR_D_SLE
			//| USB0_USBINTR_D_NAKE
			;
	}
}

void usb_run(
	usb_device_t* const device
) {
	usb_interrupt_enable(device);
	usb_controller_run(device);
}

static void copy_setup(usb_setup_t* const dst, const volatile uint8_t* const src) {
	dst->request_type = src[0];
	dst->request = src[1];
	dst->value_l = src[2];
	dst->value_h = src[3];
	dst->index_l = src[4];
	dst->index_h = src[5];
	dst->length_l = src[6];
	dst->length_h = src[7];
}

void usb_endpoint_init(
	const usb_endpoint_t* const endpoint
) {
	usb_endpoint_flush(endpoint);
	
	uint_fast16_t max_packet_size = endpoint->device->descriptor[7];
	usb_transfer_type_t transfer_type = USB_TRANSFER_TYPE_CONTROL;
	const uint8_t* const endpoint_descriptor = usb_endpoint_descriptor(endpoint);
	if( endpoint_descriptor ) {
		max_packet_size = usb_endpoint_descriptor_max_packet_size(endpoint_descriptor);
		transfer_type = usb_endpoint_descriptor_transfer_type(endpoint_descriptor);
	}
	
	// TODO: There are more capabilities to adjust based on the endpoint
	// descriptor.
	usb_queue_head_t* const qh = usb_queue_head(endpoint->address);
	qh->capabilities
		= USB_QH_CAPABILITIES_MULT(0)
		| USB_QH_CAPABILITIES_ZLT
		| USB_QH_CAPABILITIES_MPL(max_packet_size)
		| ((transfer_type == USB_TRANSFER_TYPE_CONTROL) ? USB_QH_CAPABILITIES_IOS : 0)
		;
	qh->current_dtd_pointer = 0;
	qh->next_dtd_pointer = USB_TD_NEXT_DTD_POINTER_TERMINATE;
	qh->total_bytes
		= USB_TD_DTD_TOKEN_TOTAL_BYTES(0)
		| USB_TD_DTD_TOKEN_MULTO(0)
		;
	qh->buffer_pointer_page[0] = 0;
	qh->buffer_pointer_page[1] = 0;
	qh->buffer_pointer_page[2] = 0;
	qh->buffer_pointer_page[3] = 0;
	qh->buffer_pointer_page[4] = 0;
	
	// This is how we look up an endpoint structure from an endpoint address:
	qh->_reserved_0 = (uint32_t)endpoint;

	// TODO: Should NAK be enabled? I'm kinda squishy on this...
	//USB0_ENDPTNAKEN |=
	//	USB0_ENDPTNAKEN_EPRNE(1 << endpoint_out->number);

	usb_endpoint_set_type(endpoint, transfer_type);
	
	usb_endpoint_enable(endpoint);
}

static void usb_check_for_setup_events() {
	const uint32_t endptsetupstat = usb_get_endpoint_setup_status();
	if( endptsetupstat ) {
		for( uint_fast8_t i=0; i<6; i++ ) {
			const uint32_t endptsetupstat_bit = USB0_ENDPTSETUPSTAT_ENDPTSETUPSTAT(1 << i);
			if( endptsetupstat & endptsetupstat_bit ) {
				usb_endpoint_t* const endpoint = 
					usb_endpoint_from_address(
						usb_endpoint_address(USB_TRANSFER_DIRECTION_OUT, i)
					);
				if( endpoint && endpoint->setup_complete ) {
					copy_setup(&endpoint->setup, usb_queue_head(endpoint->address)->setup);
					// TODO: Clean up this duplicated effort by providing
					// a cleaner way to get the SETUP data.
					copy_setup(&endpoint->in->setup, usb_queue_head(endpoint->address)->setup);
					usb_clear_endpoint_setup_status(endptsetupstat_bit);
					endpoint->setup_complete(endpoint);
				} else {
					usb_clear_endpoint_setup_status(endptsetupstat_bit);
				}
			}
		}
	}
}

static void usb_check_for_transfer_events() {	
	const uint32_t endptcomplete = usb_get_endpoint_complete();
	if( endptcomplete ) {
		for( uint_fast8_t i=0; i<6; i++ ) {
			
			const uint32_t endptcomplete_out_bit = USB0_ENDPTCOMPLETE_ERCE(1 << i);
			if( endptcomplete & endptcomplete_out_bit ) {
				usb_clear_endpoint_complete(endptcomplete_out_bit);
			 	usb_endpoint_t* const endpoint = 
					usb_endpoint_from_address(
						usb_endpoint_address(USB_TRANSFER_DIRECTION_OUT, i)
					);
				if( endpoint && endpoint->transfer_complete ) {
					endpoint->transfer_complete(endpoint);
				}
			}

			const uint32_t endptcomplete_in_bit = USB0_ENDPTCOMPLETE_ETCE(1 << i);
			if( endptcomplete & endptcomplete_in_bit ) {
				usb_clear_endpoint_complete(endptcomplete_in_bit);
				usb_endpoint_t* const endpoint = 
					usb_endpoint_from_address(
						usb_endpoint_address(USB_TRANSFER_DIRECTION_IN, i)
					);
				if( endpoint && endpoint->transfer_complete ) {
					endpoint->transfer_complete(endpoint);
				}
			}
		}
	}
}

void usb0_isr() {
	const uint32_t status = usb_get_status();
	
	if( status == 0 ) {
		// Nothing to do.
		return;
	}
	
	if( status & USB0_USBSTS_D_UI ) {
		// USB:
		// - Completed transaction transfer descriptor has IOC set.
		// - Short packet detected.
		// - SETUP packet received.

		usb_check_for_setup_events();
		usb_check_for_transfer_events();
		
		// TODO: Reset ignored ENDPTSETUPSTAT and ENDPTCOMPLETE flags?
	}

	if( status & USB0_USBSTS_D_SRI ) {
		// Start Of Frame received.
	}

	if( status & USB0_USBSTS_D_PCI ) {
		// Port change detect:
		// Port controller entered full- or high-speed operational state.
	}

	if( status & USB0_USBSTS_D_SLI ) {
		// Device controller suspend.
	}

	if( status & USB0_USBSTS_D_URI ) {
		// USB reset received.
		usb_bus_reset(usb_device_usb0);
	}

	if( status & USB0_USBSTS_D_UEI ) {
		// USB error:
		// Completion of a USB transaction resulted in an error condition.
		// Set along with USBINT if the TD on which the error interrupt
		// occurred also had its interrupt on complete (IOC) bit set.
		// The device controller detects resume signalling only.
	}

	if( status & USB0_USBSTS_D_NAKI ) {
		// Both the TX/RX endpoint NAK bit and corresponding TX/RX endpoint
		// NAK enable bit are set.
	}
}
