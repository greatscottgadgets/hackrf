/*
 * Copyright 2012 Michael Ossmann
 * Copyright (C) 2012 Jared Boone
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

#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/sgpio.h>
#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/cm3/scs.h>

#include <hackrf_core.h>

void pin_setup(void) {
	/* Configure SCU Pin Mux as GPIO */
	scu_pinmux(SCU_PINMUX_LED1, SCU_GPIO_FAST);
	scu_pinmux(SCU_PINMUX_LED2, SCU_GPIO_FAST);
	scu_pinmux(SCU_PINMUX_LED3, SCU_GPIO_FAST);

	scu_pinmux(SCU_PINMUX_EN1V8, SCU_GPIO_FAST);

	/* Configure all GPIO as Input (safe state) */GPIO0_DIR = 0;
	GPIO1_DIR = 0;
	GPIO2_DIR = 0;
	GPIO3_DIR = 0;
	GPIO4_DIR = 0;
	GPIO5_DIR = 0;
	GPIO6_DIR = 0;
	GPIO7_DIR = 0;

	/* Configure GPIO2[1/2/8] (P4_1/2 P6_12) as output. */GPIO2_DIR |= (PIN_LED1
			| PIN_LED2 | PIN_LED3);

	/* GPIO3[6] on P6_10  as output. */GPIO3_DIR |= PIN_EN1V8;

	/* Configure SSP1 Peripheral (to be moved later in SSP driver) */
	scu_pinmux(SCU_SSP1_MISO, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP1_MOSI, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP1_SCK, (SCU_SSP_IO | SCU_CONF_FUNCTION1));
	scu_pinmux(SCU_SSP1_SSEL, (SCU_SSP_IO | SCU_CONF_FUNCTION1));
}

void enable_1v8_power() {
	gpio_set(PORT_EN1V8, PIN_EN1V8);
}

void release_cpld_jtag_pins() {
	scu_pinmux(SCU_PINMUX_CPLD_TDO, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION4);
	scu_pinmux(SCU_PINMUX_CPLD_TCK, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_CPLD_TMS, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_CPLD_TDI, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);

	GPIO_CLR(PORT_CPLD_TDO) = PIN_CPLD_TDO;
	GPIO_CLR(PORT_CPLD_TCK) = PIN_CPLD_TCK;
	GPIO_CLR(PORT_CPLD_TMS) = PIN_CPLD_TMS;
	GPIO_CLR(PORT_CPLD_TDI) = PIN_CPLD_TDI;
}

void configure_sgpio_pin_functions() {
	scu_pinmux(SCU_PINMUX_SGPIO0, SCU_GPIO_FAST | SCU_CONF_FUNCTION3);
	scu_pinmux(SCU_PINMUX_SGPIO1, SCU_GPIO_FAST | SCU_CONF_FUNCTION3);
	scu_pinmux(SCU_PINMUX_SGPIO2, SCU_GPIO_FAST | SCU_CONF_FUNCTION2);
	scu_pinmux(SCU_PINMUX_SGPIO3, SCU_GPIO_FAST | SCU_CONF_FUNCTION2);
	scu_pinmux(SCU_PINMUX_SGPIO4, SCU_GPIO_FAST | SCU_CONF_FUNCTION2);
	scu_pinmux(SCU_PINMUX_SGPIO5, SCU_GPIO_FAST | SCU_CONF_FUNCTION2);
	scu_pinmux(SCU_PINMUX_SGPIO6, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_SGPIO7, SCU_GPIO_FAST | SCU_CONF_FUNCTION6);
	scu_pinmux(SCU_PINMUX_SGPIO8, SCU_GPIO_FAST | SCU_CONF_FUNCTION6);
	scu_pinmux(SCU_PINMUX_SGPIO9, SCU_GPIO_FAST | SCU_CONF_FUNCTION7);
	scu_pinmux(SCU_PINMUX_SGPIO10, SCU_GPIO_FAST | SCU_CONF_FUNCTION6);
	scu_pinmux(SCU_PINMUX_SGPIO11, SCU_GPIO_FAST | SCU_CONF_FUNCTION6);
	scu_pinmux(SCU_PINMUX_SGPIO12, SCU_GPIO_FAST | SCU_CONF_FUNCTION6);
	scu_pinmux(SCU_PINMUX_SGPIO13, SCU_GPIO_FAST | SCU_CONF_FUNCTION7);
	scu_pinmux(SCU_PINMUX_SGPIO14, SCU_GPIO_FAST | SCU_CONF_FUNCTION7);
	scu_pinmux(SCU_PINMUX_SGPIO15, SCU_GPIO_FAST | SCU_CONF_FUNCTION7);
}

void test_sgpio_interface() {
	const uint_fast8_t host_clock_sgpio_pin = 8; // Input
	const uint_fast8_t host_capture_sgpio_pin = 9; // Input
	const uint_fast8_t host_disable_sgpio_pin = 10; // Output
	const uint_fast8_t host_direction_sgpio_pin = 11; // Output

	SGPIO_GPIO_OENREG = 0; // All inputs for the moment.

	// Disable all counters during configuration
	SGPIO_CTRL_ENABLE = 0;

	configure_sgpio_pin_functions();

	// Make all SGPIO controlled by SGPIO's "GPIO" registers
	for (uint_fast8_t i = 0; i < 16; i++) {
		SGPIO_OUT_MUX_CFG(i) = (0L << 4) | (4L << 0);
	}

	// Set SGPIO output values.
	SGPIO_GPIO_OUTREG = (1L << host_direction_sgpio_pin)
			| (1L << host_disable_sgpio_pin);

	// Enable SGPIO pin outputs.
	SGPIO_GPIO_OENREG = (1L << host_direction_sgpio_pin)
			| (1L << host_disable_sgpio_pin) | (0L << host_capture_sgpio_pin)
			| (0L << host_clock_sgpio_pin) | (0xFF << 0);

	// Configure SGPIO slices.

	// Enable codec data stream.
	SGPIO_GPIO_OUTREG &= ~(1L << host_disable_sgpio_pin);

	while (1) {
		for (uint_fast8_t i = 0; i < 8; i++) {
			SGPIO_GPIO_OUTREG ^= (1L << i);
		}
	}
}

void configure_sgpio() {
	// Disable all counters during configuration
	SGPIO_CTRL_ENABLE = 0;

    configure_sgpio_pin_functions();

    // Set SGPIO output values.
    SGPIO_GPIO_OUTREG =
        (1L << 11) |    // direction
        (1L << 10);     // disable

	// Enable SGPIO pin outputs.
	SGPIO_GPIO_OENREG =
		(1L << 11) |    // direction
	    (1L << 10) |    // disable
        0xFF;

	SGPIO_OUT_MUX_CFG( 8) = 0;   // SGPIO: Input: clock
	SGPIO_OUT_MUX_CFG( 9) = 0;   // SGPIO: Input: qualifier
    SGPIO_OUT_MUX_CFG(10) = (0L << 4) | (4L << 0);   // GPIO: Output: disable
    SGPIO_OUT_MUX_CFG(11) = (0L << 4) | (4L << 0);   // GPIO: Output: direction

	for(uint_fast8_t i=0; i<8; i++) {
		// SGPIO pin 0 outputs slice A bit "i".
		SGPIO_OUT_MUX_CFG(i) =
		    (0L <<  4) |    // P_OE_CFG = 0
		    (9L <<  0);     // P_OUT_CFG = 9, dout_doutm8a (8-bit mode 8a)
	}

	// Slice A
	SGPIO_MUX_CFG(SGPIO_SLICE_A) =
	    (0L << 12) |    // CONCAT_ORDER = 0 (self-loop)
	    (1L << 11) |    // CONCAT_ENABLE = 1 (concatenate data)
	    (0L <<  9) |    // QUALIFIER_SLICE_MODE = X
	    (1L <<  7) |    // QUALIFIER_PIN_MODE = 1 (SGPIO9)
	    (3L <<  5) |    // QUALIFIER_MODE = 3 (external SGPIO pin)
	    (0L <<  3) |    // CLK_SOURCE_SLICE_MODE = X
	    (0L <<  1) |    // CLK_SOURCE_PIN_MODE = 0 (SGPIO8)
	    (1L <<  0);     // EXT_CLK_ENABLE = 1, external clock signal (slice)

	SGPIO_SLICE_MUX_CFG(SGPIO_SLICE_A) =
	    (0L <<  8) |    // INV_QUALIFIER = 0 (use normal qualifier)
	    (3L <<  6) |    // PARALLEL_MODE = 3 (shift 8 bits per clock)
	    (0L <<  4) |    // DATA_CAPTURE_MODE = 0 (detect rising edge)
	    (0L <<  3) |    // INV_OUT_CLK = 0 (normal clock)
	    (1L <<  2) |    // CLKGEN_MODE = 1 (use external pin clock)
	    (0L <<  1) |    // CLK_CAPTURE_MODE = 0 (use rising clock edge)
	    (0L <<  0);     // MATCH_MODE = 0 (do not match data)

	SGPIO_PRESET(SGPIO_SLICE_A) = 0;
	SGPIO_COUNT(SGPIO_SLICE_A) = 0;
	SGPIO_POS(SGPIO_SLICE_A) = (0x3L << 8) | (0x3L << 0);
	SGPIO_REG(SGPIO_SLICE_A) = 0xFF00FF00;     // Primary output data register
	SGPIO_REG_SS(SGPIO_SLICE_A) = 0xFF00FF00;  // Shadow output data register
    /*
	// Slice D (clock for Slice A)
	LPC_SGPIO->SGPIO_MUX_CFG[3] =
	    (0L << 12) |    // CONCAT_ORDER = 0 (self-loop)
	    (1L << 11) |    // CONCAT_ENABLE = 1 (concatenate data)
	    (0L <<  9) |    // QUALIFIER_SLICE_MODE = X
	    (0L <<  7) |    // QUALIFIER_PIN_MODE = X
	    (0L <<  5) |    // QUALIFIER_MODE = 0 (enable)
	    (0L <<  3) |    // CLK_SOURCE_SLICE_MODE = 0, slice D
	    (0L <<  1) |    // CLK_SOURCE_PIN_MODE = X
	    (0L <<  0);     // EXT_CLK_ENABLE = 0, internal clock signal (slice)

	LPC_SGPIO->SLICE_MUX_CFG[3] =
	    (0L <<  8) |    // INV_QUALIFIER = 0 (use normal qualifier)
	    (0L <<  6) |    // PARALLEL_MODE = 0 (shift 1 bit per clock)
	    (0L <<  4) |    // DATA_CAPTURE_MODE = 0 (detect rising edge)
	    (0L <<  3) |    // INV_OUT_CLK = 0 (normal clock)
	    (0L <<  2) |    // CLKGEN_MODE = 0 (use clock from COUNTER)
	    (0L <<  1) |    // CLK_CAPTURE_MODE = 0 (use rising clock edge)
	    (0L <<  0);     // MATCH_MODE = 0 (do not match data)

	LPC_SGPIO->PRESET[3] = 0;
	LPC_SGPIO->COUNT[3] = 0;
	LPC_SGPIO->POS[3] = (0x1FL << 8) | (0x1FL << 0);
	LPC_SGPIO->REG[0] = 0xAAAAAAAA;     // Primary output data register
	LPC_SGPIO->REG_SS[0] = 0xAAAAAAAA;  // Shadow output data register
    */
	// Start SGPIO operation by enabling slice clocks.
	SGPIO_CTRL_ENABLE =
	    (1L <<  0)      // Slice A
    ;

    // Enable codec data stream.
    SGPIO_GPIO_OUTREG &= ~(1L << 10);
}

int main(void) {
	pin_setup();
	enable_1v8_power();
	cpu_clock_init();

	CGU_BASE_PERIPH_CLK = (CGU_BASE_CLK_AUTOBLOCK
			| (CGU_SRC_PLL1 << CGU_BASE_CLK_SEL_SHIFT));

	CGU_BASE_APB1_CLK = (CGU_BASE_CLK_AUTOBLOCK
			| (CGU_SRC_PLL1 << CGU_BASE_CLK_SEL_SHIFT));

	gpio_set(PORT_LED1_3, (PIN_LED1 | PIN_LED2 | PIN_LED3)); /* LEDs on */

	//test_sgpio_interface();
	configure_sgpio();

	while (1) {

	}

	return 0;
}
