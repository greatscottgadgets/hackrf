#include <string.h>

#include <hackrf_core.h>

#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/lpc43xx/gpio.h>

#include "usb.h"
#include "usb_type.h"
#include "usb_request.h"
#include "usb_descriptor.h"
#include "usb_standard_request.h"

volatile uint_fast8_t usb_bulk_buffer_index = 0;
uint8_t usb_bulk_buffer[2][16384] ATTR_SECTION(".usb_data");
const uint_fast8_t usb_bulk_buffer_count =
	sizeof(usb_bulk_buffer) / sizeof(usb_bulk_buffer[0]);

static void usb_endpoint_bulk_transfer(usb_endpoint_t* const endpoint) {
	usb_endpoint_schedule(endpoint, usb_bulk_buffer[usb_bulk_buffer_index], sizeof(usb_bulk_buffer[usb_bulk_buffer_index]));
	usb_bulk_buffer_index = (usb_bulk_buffer_index + 1) % usb_bulk_buffer_count;
}

usb_configuration_t usb_configuration_high_speed = {
	.number = 1,
	.speed = USB_SPEED_HIGH,
	.descriptor = usb_descriptor_configuration_high_speed,
};

usb_configuration_t usb_configuration_full_speed = {
	.number = 1,
	.speed = USB_SPEED_FULL,
	.descriptor = usb_descriptor_configuration_full_speed,
};

usb_configuration_t* usb_configurations[] = {
	&usb_configuration_high_speed,
	&usb_configuration_full_speed,
	0,
};

usb_device_t usb_device = {
	.descriptor = usb_descriptor_device,
	.configurations = &usb_configurations,
	.configuration = 0,
};

usb_endpoint_t usb_endpoint_control_out;
usb_endpoint_t usb_endpoint_control_in;

usb_endpoint_t usb_endpoint_control_out = {
	.address = 0x00,
	.device = &usb_device,
	.in = &usb_endpoint_control_in,
	.out = &usb_endpoint_control_out,
	.setup_complete = usb_setup_complete,
	.transfer_complete = usb_control_out_complete,
};

usb_endpoint_t usb_endpoint_control_in = {
	.address = 0x80,
	.device = &usb_device,
	.in = &usb_endpoint_control_in,
	.out = &usb_endpoint_control_out,
	.setup_complete = 0,
	.transfer_complete = usb_control_in_complete,
};

// NOTE: Endpoint number for IN and OUT are different. I wish I had some
// evidence that having BULK IN and OUT on separate endpoint numbers was
// actually a good idea. Seems like everybody does it that way, but why?

usb_endpoint_t usb_endpoint_bulk_in = {
	.address = 0x81,
	.device = &usb_device,
	.in = &usb_endpoint_bulk_in,
	.out = 0,
	.setup_complete = 0,
	.transfer_complete = usb_endpoint_bulk_transfer,
};

usb_endpoint_t usb_endpoint_bulk_out = {
	.address = 0x02,
	.device = &usb_device,
	.in = 0,
	.out = &usb_endpoint_bulk_out,
	.setup_complete = 0,
	.transfer_complete = usb_endpoint_bulk_transfer,
};

const usb_request_handlers_t usb_request_handlers = {
	.standard = usb_standard_request,
	.class = 0,
	.vendor = 0,
	.reserved = 0,
};

// TODO: Seems like this should live in usb_standard_request.c.
bool usb_set_configuration(
	usb_device_t* const device,
	const uint_fast8_t configuration_number
) {
	const usb_configuration_t* new_configuration = 0;
	if( configuration_number != 0 ) {
		
		// Locate requested configuration.
		if( device->configurations ) {
			usb_configuration_t** configurations = *(device->configurations);
			uint32_t i = 0;
			const usb_speed_t usb_speed_current = usb_speed(device);
			while( configurations[i] ) {
				if( (configurations[i]->speed == usb_speed_current) &&
				    (configurations[i]->number == configuration_number) ) {
					new_configuration = configurations[i];
					break;
				}
				i++;
			}
		}

		// Requested configuration not found: request error.
		if( new_configuration == 0 ) {
			return false;
		}
	}
	
	if( new_configuration != device->configuration ) {
		// Configuration changed.
		device->configuration = new_configuration;

		// TODO: This is lame. There should be a way to link stuff together so
		// that changing the configuration will initialize the related
		// endpoints. No hard-coding crap like this! Then, when there's no more
		// hard-coding, this whole function can move into a shared/reusable
		// library.
		if( device->configuration && (device->configuration->number == 1) ) {
			usb_endpoint_init(&usb_endpoint_bulk_in);
			usb_endpoint_init(&usb_endpoint_bulk_out);
			
			usb_endpoint_bulk_transfer(&usb_endpoint_bulk_out);
			//usb_endpoint_bulk_transfer(&usb_endpoint_bulk_in);
		} else {
			usb_endpoint_disable(&usb_endpoint_bulk_in);
			usb_endpoint_disable(&usb_endpoint_bulk_out);
		}

		if( device->configuration ) {
			gpio_set(PORT_LED1_3, PIN_LED1);
		} else {
			gpio_clear(PORT_LED1_3, PIN_LED1);
		}
	}

	return true;
};

int main(void) {
	pin_setup();
	enable_1v8_power();
	cpu_clock_init();

	CGU_BASE_PERIPH_CLK = CGU_BASE_PERIPH_CLK_AUTOBLOCK
			| CGU_BASE_PERIPH_CLK_CLK_SEL(CGU_SRC_PLL1);

	CGU_BASE_APB1_CLK = CGU_BASE_APB1_CLK_AUTOBLOCK
			| CGU_BASE_APB1_CLK_CLK_SEL(CGU_SRC_PLL1);
	
	usb_peripheral_reset();
	
	usb_device_init(0, &usb_device);
	
	usb_endpoint_init(&usb_endpoint_control_out);
	usb_endpoint_init(&usb_endpoint_control_in);
	
	usb_run(&usb_device);
	
	while (1) {
	}

	return 0;
}
