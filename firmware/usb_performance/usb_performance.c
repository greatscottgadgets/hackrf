#include <string.h>

#include <hackrf_core.h>
#include <bitband.h>

#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/lpc43xx/creg.h>
#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/nvic.h>
#include <libopencm3/lpc43xx/rgu.h>

#include <libopencm3/lpc43xx/usb.h>

#include <libopencm3/usb/usbstd.h>

#define ATTR_ALIGNED(x)	__attribute__ ((aligned(x)))
#define __DATA(x) __attribute__ ((section("x")))

void usb_reset_peripheral() {
	RESET_CTRL0 = RESET_CTRL0_USB0_RST;
	RESET_CTRL0 = 0;
	
	while( (RESET_ACTIVE_STATUS0 & RESET_CTRL0_USB0_RST) == 0 );
}

void usb_enable_phy() {
	peripheral_bitband_clear(&CREG_CREG0, CREG_CREG0_USB0PHY_SHIFT);
}

void usb_wait_for_endpoint_priming_to_finish() {
    while( USB0_ENDPTPRIME );
}

void usb_wait_for_endpoint_flushing_to_finish() {
    while( USB0_ENDPTFLUSH );
}

void usb_flush_all_primed_endpoints() {
    usb_wait_for_endpoint_priming_to_finish();
    USB0_ENDPTFLUSH = ~0;
    usb_wait_for_endpoint_flushing_to_finish();
}

void usb_stop_controller() {
    peripheral_bitband_clear(&USB0_USBCMD_D, USB0_USBCMD_D_RS_SHIFT);
}

void usb_run_controller() {
    peripheral_bitband_set(&USB0_USBCMD_D, USB0_USBCMD_D_RS_SHIFT);
}

uint_fast8_t usb_controller_is_resetting() {
    return (USB0_USBCMD_D & USB0_USBCMD_D_RST) != 0;
}

void usb_reset_controller() {
	USB0_USBCMD_D = USB0_USBCMD_D_RST;
	while( usb_controller_is_resetting() );
}
/*
void usb_reset() {
    usb_flush_all_primed_endpoints();
    usb_stop_controller();
	usb_reset_controller();
    //while( usb_controller_is_resetting() );
}
*/
/*
void usb_disable_endpoint(uint_fast8_t logical_endpoint_number) {
	USB0_ENDPTCTRL(logical_endpoint_number) &= ~((1 << ) | (1 << ));
}

void usb_disable_all_endpoints() {
	// Endpoint 0 is always enabled.
	for(uint_fast8_t i=1; i<6; i++) {
		usb_disable_endpoint(i);
	}
}

void usb_clear_all_pending_interrupts() {
	
}

void usb_reset() {
	usb_disable_all_endpoints();
	usb_clear_all_pending_interrupts();
}
*/
/*
void usb_clear_all_interrupts() {
	USB0_USBSTS =
		USB0_USBSTS_UI |
		USB0_USBSTS_UEI |
		USB0_USBSTS_PCI |
		USB0_USBSTS_URI |
		USB0_USBSTS_SRI |
		USB0_USBSTS_SLI;
}
*/
typedef struct {
	uint32_t next_dtd_pointer;
	uint32_t total_bytes;
	uint32_t buffer_pointer_page[5];
} usb0_endpoint_transfer_descriptor_t;

typedef enum {
	queue_head_capabilities_IOS_bit = 15,
	queue_head_capabilities_IOS = 1 << queue_head_capabilities_IOS_bit,
	
	queue_head_capabilities_MPL_base = 16,
	queue_head_capabilities_MPL_length = 11,
	
	queue_head_capabilities_ZLT_bit = 29,
	queue_head_capabilities_ZLT = 1 << queue_head_capabilities_ZLT_bit,
	
	queue_head_capabilities_MULT_base = 30,
	queue_head_capabilities_MULT_length = 2,
} queue_head_capabilities_t;
	
typedef volatile struct {
	volatile uint32_t capabilities;
	volatile uint32_t current_dtd_pointer;
	//volatile usb0_endpoint_transfer_descriptor_t;
	volatile uint32_t next_dtd_pointer;
	volatile uint32_t total_bytes;
	volatile uint32_t buffer_pointer_page[5];
	volatile uint32_t _reserved_0;
	volatile uint8_t setup[8];
	volatile uint32_t _reserved_1[4];
} usb0_queue_head_t;

volatile usb0_queue_head_t queue_head[12] ATTR_ALIGNED(2048) __DATA(USBRAM_SECTION);
volatile usb0_endpoint_transfer_descriptor_t transfer_descriptor[12] ATTR_ALIGNED(64) __DATA(USBRAM_SECTION);

void usb_init() {
	usb_enable_phy();
	usb_reset_controller();
	USB0_USBMODE_D =
		USB0_USBMODE_D_SLOM |
		USB0_USBMODE_D_CM1_0(2);

	nvic_enable_irq(NVIC_M4_USB0_IRQ);

	// Set interrupt threshold interval to 0
	USB0_USBCMD_D &= ~(USB0_USBCMD_D_ITC_MASK);

	USB0_ENDPOINTLISTADDR = (uint32_t)&queue_head;
	for(uint_fast8_t i=0; i<2; i++) {
		queue_head[i].next_dtd_pointer = (uint32_t)&transfer_descriptor[i];
	}
	
	// Enable interrupts
	USB0_USBINTR_D =
		USB0_USBINTR_D_UE |
		USB0_USBINTR_D_UEE |
		USB0_USBINTR_D_PCE |
		USB0_USBINTR_D_URE |
		USB0_USBINTR_D_SRE |
		USB0_USBINTR_D_SLE |
		USB0_USBINTR_D_NAKE;
	
	queue_head[0].capabilities =
		queue_head_capabilities_ZLT |
		(64 << queue_head_capabilities_MPL_base) |
		queue_head_capabilities_IOS;
}

void usb0_irqhandler(void) {
	gpio_clear(PORT_LED1_3, PIN_LED1);
	gpio_clear(PORT_LED1_3, PIN_LED2);
	gpio_clear(PORT_LED1_3, PIN_LED3);
}

int main(void) {
	pin_setup();
	enable_1v8_power();
	cpu_clock_init();

	CGU_BASE_PERIPH_CLK = CGU_BASE_PERIPH_CLK_AUTOBLOCK
			| CGU_BASE_PERIPH_CLK_CLK_SEL(CGU_SRC_PLL1);

	CGU_BASE_APB1_CLK = CGU_BASE_APB1_CLK_AUTOBLOCK
			| CGU_BASE_APB1_CLK_CLK_SEL(CGU_SRC_PLL1);

	usb_reset_peripheral();

	gpio_set(PORT_LED1_3, PIN_LED1);
	gpio_set(PORT_LED1_3, PIN_LED2);
	gpio_set(PORT_LED1_3, PIN_LED3);

	usb_init();
	
	while (1) {
		delay(10000000);
		gpio_clear(PORT_LED1_3, PIN_LED1);
		delay(10000000);
		gpio_set(PORT_LED1_3, PIN_LED1);
	}

	return 0;
}
