/*
 * Copyright 2012 Jared Boone <jared@sharebrained.com>
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

#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/sgpio.h>

#include <hackrf_core.h>

void sgpio_configure_pin_functions() {
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


void sgpio_test_interface() {
	const uint_fast8_t host_clock_sgpio_pin = 8; // Input
	const uint_fast8_t host_capture_sgpio_pin = 9; // Input
	const uint_fast8_t host_disable_sgpio_pin = 10; // Output
	const uint_fast8_t host_direction_sgpio_pin = 11; // Output

	SGPIO_GPIO_OENREG = 0; // All inputs for the moment.

	// Disable all counters during configuration
	SGPIO_CTRL_ENABLE = 0;

	sgpio_configure_pin_functions();

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

void sgpio_configure_for_tx() {
	// Disable all counters during configuration
	SGPIO_CTRL_ENABLE = 0;

    sgpio_configure_pin_functions();

    // Set SGPIO output values.
    SGPIO_GPIO_OUTREG =
        (1L << 11) |    // direction
        (1L << 10);     // disable

	// Enable SGPIO pin outputs.
	SGPIO_GPIO_OENREG =
		(1L << 11) |    // direction: TX: data to CPLD
	    (1L << 10) |    // disable
	    (0L <<  9) |    // capture
	    (0L <<  8) |    // clock
        0xFF;           // data: output

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
	SGPIO_REG(SGPIO_SLICE_A) = 0x80808080;     // Primary output data register
	SGPIO_REG_SS(SGPIO_SLICE_A) = 0x80808080;  // Shadow output data register

	// Start SGPIO operation by enabling slice clocks.
	SGPIO_CTRL_ENABLE =
	    (1L << SGPIO_SLICE_A)
    ;
}

void sgpio_configure_for_rx() {
	// Disable all counters during configuration
	SGPIO_CTRL_ENABLE = 0;

    sgpio_configure_pin_functions();

    // Set SGPIO output values.
    SGPIO_GPIO_OUTREG =
        (0L << 11) |    // direction
        (1L << 10);     // disable

	// Enable SGPIO pin outputs.
	SGPIO_GPIO_OENREG =
		(1L << 11) |    // direction: RX: data from CPLD
	    (1L << 10) |    // disable
	    (0L <<  9) |    // capture
	    (0L <<  8) |    // clock
        0x00;           // data: input

	SGPIO_OUT_MUX_CFG( 8) = 0;   // SGPIO: Input: clock
	SGPIO_OUT_MUX_CFG( 9) = 0;   // SGPIO: Input: qualifier
    SGPIO_OUT_MUX_CFG(10) = (0L << 4) | (4L << 0);   // GPIO: Output: disable
    SGPIO_OUT_MUX_CFG(11) = (0L << 4) | (4L << 0);   // GPIO: Output: direction

	for(uint_fast8_t i=0; i<8; i++) {
		SGPIO_OUT_MUX_CFG(i) =
		    (0L <<  4) |    // P_OE_CFG = 0
		    (9L <<  0);     // P_OUT_CFG = 9, dout_doutm8a (8-bit mode 8a)
	}

	// Slice A
	SGPIO_MUX_CFG(SGPIO_SLICE_A) =
	    (0L << 12) |    // CONCAT_ORDER = X
	    (0L << 11) |    // CONCAT_ENABLE = 0 (concatenate data)
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
	    (0L <<  3) |    // INV_OUT_CLK = X
	    (1L <<  2) |    // CLKGEN_MODE = 1 (use external pin clock)
	    (1L <<  1) |    // CLK_CAPTURE_MODE = 1 (use falling clock edge)
	    (0L <<  0);     // MATCH_MODE = 0 (do not match data)

	SGPIO_PRESET(SGPIO_SLICE_A) = 0;
	SGPIO_COUNT(SGPIO_SLICE_A) = 0;
	SGPIO_POS(SGPIO_SLICE_A) = (0 << 8) | (0 << 0);
	SGPIO_REG(SGPIO_SLICE_A) = 0xCAFEBABE;     // Primary output data register
	SGPIO_REG_SS(SGPIO_SLICE_A) = 0xDEADBEEF;  // Shadow output data register

	// Start SGPIO operation by enabling slice clocks.
	SGPIO_CTRL_ENABLE =
	    (1L << SGPIO_SLICE_A)
    ;
}

void sgpio_configure_for_rx_deep() {
	// Disable all counters during configuration
	SGPIO_CTRL_ENABLE = 0;

    sgpio_configure_pin_functions();

    // Set SGPIO output values.
    SGPIO_GPIO_OUTREG =
        (0L << 11) |    // direction
        (1L << 10);     // disable

	// Enable SGPIO pin outputs.
	SGPIO_GPIO_OENREG =
		(1L << 11) |    // direction: RX: data from CPLD
	    (1L << 10) |    // disable
	    (0L <<  9) |    // capture
	    (0L <<  8) |    // clock
        0x00;           // data: input

	SGPIO_OUT_MUX_CFG( 8) = 0;   // SGPIO: Input: clock
	SGPIO_OUT_MUX_CFG( 9) = 0;   // SGPIO: Input: qualifier
    SGPIO_OUT_MUX_CFG(10) = (0L << 4) | (4L << 0);   // GPIO: Output: disable
    SGPIO_OUT_MUX_CFG(11) = (0L << 4) | (4L << 0);   // GPIO: Output: direction

	for(uint_fast8_t i=0; i<8; i++) {
		SGPIO_OUT_MUX_CFG(i) =
		    (0L <<  4) |    // P_OE_CFG = 0
		    (9L <<  0);     // P_OUT_CFG = 9, dout_doutm8a (8-bit mode 8a)
	}

	const uint_fast8_t slice_indices[] = {
		SGPIO_SLICE_A,
		SGPIO_SLICE_I,
		SGPIO_SLICE_E,
		SGPIO_SLICE_J,
		SGPIO_SLICE_C,
		SGPIO_SLICE_K,
		SGPIO_SLICE_F,
		SGPIO_SLICE_L,
	};
	
	uint32_t slice_enable_mask = 0;
	for(uint_fast8_t i=0; i<8; i++) {
		uint_fast8_t slice_index = slice_indices[i];
		const uint_fast8_t concat_order = (slice_index == SGPIO_SLICE_A) ? 0 : 3;
		const uint_fast8_t concat_enable = (slice_index == SGPIO_SLICE_A) ? 0 : 1;
		SGPIO_MUX_CFG(slice_index) =
		    (concat_order << 12) |	// CONCAT_ORDER = 3 (eight slices)
		    (concat_enable << 11) |	// CONCAT_ENABLE = 1 (concatenate data)
		    (0L <<  9) |    // QUALIFIER_SLICE_MODE = X
		    (1L <<  7) |    // QUALIFIER_PIN_MODE = 1 (SGPIO9)
		    (3L <<  5) |    // QUALIFIER_MODE = 3 (external SGPIO pin)
		    (0L <<  3) |    // CLK_SOURCE_SLICE_MODE = X
		    (0L <<  1) |    // CLK_SOURCE_PIN_MODE = 0 (SGPIO8)
		    (1L <<  0);     // EXT_CLK_ENABLE = 1, external clock signal (slice)

		SGPIO_SLICE_MUX_CFG(slice_index) =
		    (0L <<  8) |    // INV_QUALIFIER = 0 (use normal qualifier)
		    (3L <<  6) |    // PARALLEL_MODE = 3 (shift 8 bits per clock)
		    (0L <<  4) |    // DATA_CAPTURE_MODE = 0 (detect rising edge)
		    (0L <<  3) |    // INV_OUT_CLK = X
		    (1L <<  2) |    // CLKGEN_MODE = 1 (use external pin clock)
		    (1L <<  1) |    // CLK_CAPTURE_MODE = 1 (use falling clock edge)
		    (0L <<  0);     // MATCH_MODE = 0 (do not match data)

		SGPIO_PRESET(slice_index) = 0;			// External clock, don't care
		SGPIO_COUNT(slice_index) = 0;				// External clock, don't care
		SGPIO_POS(slice_index) = (0x1f << 8) | (0x1f << 0);
		SGPIO_REG(slice_index) = 0xFFFFFFFF;     // Primary output data register
		SGPIO_REG_SS(slice_index) = 0xFFFFFFFF;  // Shadow output data register
		
		slice_enable_mask |= (1 << slice_index);
	}
	
	// Start SGPIO operation by enabling slice clocks.
	SGPIO_CTRL_ENABLE = slice_enable_mask;
}

void sgpio_cpld_stream_enable() {
	// Enable codec data stream.
	SGPIO_GPIO_OUTREG &= ~(1L << 10);
}

void sgpio_cpld_stream_disable() {
	// Disable codec data stream.
	SGPIO_GPIO_OUTREG |= (1L << 10);
}

bool sgpio_cpld_stream_is_enabled() {
	return (SGPIO_GPIO_OUTREG & (1L << 10)) == 0;
}