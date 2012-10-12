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
		SGPIO_OUT_MUX_CFG(i) =
			  SGPIO_OUT_MUX_CFG_P_OE_CFG(0)
			| SGPIO_OUT_MUX_CFG_P_OUT_CFG(4);
	}

	// Set SGPIO output values.
	SGPIO_GPIO_OUTREG =
		  (1L << host_direction_sgpio_pin)
		| (1L << host_disable_sgpio_pin);

	// Enable SGPIO pin outputs.
	SGPIO_GPIO_OENREG =
		  (1L << host_direction_sgpio_pin)
		| (1L << host_disable_sgpio_pin)
		| (0L << host_capture_sgpio_pin)
		| (0L << host_clock_sgpio_pin)
		| (0xFF << 0);

	// Configure SGPIO slices.

	// Enable codec data stream.
	SGPIO_GPIO_OUTREG &= ~(1L << host_disable_sgpio_pin);

	while (1) {
		for (uint_fast8_t i = 0; i < 8; i++) {
			SGPIO_GPIO_OUTREG ^= (1L << i);
		}
	}
}

void sgpio_configure(
	const transceiver_mode_t transceiver_mode,
	const bool multi_slice
) {
	// Disable all counters during configuration
	SGPIO_CTRL_ENABLE = 0;

    sgpio_configure_pin_functions();

    // Set SGPIO output values.
	const uint_fast8_t cpld_direction =
		(transceiver_mode == TRANSCEIVER_MODE_TX) ? 1 : 0;
    SGPIO_GPIO_OUTREG =
          (cpld_direction << 11)
        | (1L << 10)	// disable
		;

	// Enable SGPIO pin outputs.
	const uint_fast16_t sgpio_gpio_data_direction =
		(transceiver_mode == TRANSCEIVER_MODE_TX)
		? (0xFF << 0)
		: (0x00 << 0);
	SGPIO_GPIO_OENREG =
		  (1L << 11)	// direction
	    | (1L << 10)	// disable
	    | (0L <<  9)	// capture
	    | (0L <<  8)	// clock
        | sgpio_gpio_data_direction
		;

	SGPIO_OUT_MUX_CFG( 8) =		// SGPIO: Input: clock
		  SGPIO_OUT_MUX_CFG_P_OE_CFG(0)
		| SGPIO_OUT_MUX_CFG_P_OUT_CFG(0)
		;
	SGPIO_OUT_MUX_CFG( 9) =		// SGPIO: Input: qualifier
		  SGPIO_OUT_MUX_CFG_P_OE_CFG(0)
		| SGPIO_OUT_MUX_CFG_P_OUT_CFG(0)
		;
    SGPIO_OUT_MUX_CFG(10) =		// GPIO: Output: disable
		  SGPIO_OUT_MUX_CFG_P_OE_CFG(0)
		| SGPIO_OUT_MUX_CFG_P_OUT_CFG(4)
		;
    SGPIO_OUT_MUX_CFG(11) =		// GPIO: Output: direction
		  SGPIO_OUT_MUX_CFG_P_OE_CFG(0)
		| SGPIO_OUT_MUX_CFG_P_OUT_CFG(4)
		;

	const uint_fast8_t output_multiplexing_mode =
		multi_slice ? 11 : 9;
	for(uint_fast8_t i=0; i<8; i++) {
		// SGPIO pin 0 outputs slice A bit "i".
		SGPIO_OUT_MUX_CFG(i) =
		      SGPIO_OUT_MUX_CFG_P_OE_CFG(0)
		    | SGPIO_OUT_MUX_CFG_P_OUT_CFG(output_multiplexing_mode)
			;
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
	
	const bool single_slice = !multi_slice;
	const uint_fast8_t slice_count = multi_slice ? 8 : 1;
	
	uint32_t slice_enable_mask = 0;
	for(uint_fast8_t i=0; i<slice_count; i++) {
		const uint_fast8_t slice_index = slice_indices[i];
		const bool input_slice = (i == 0) && (transceiver_mode == TRANSCEIVER_MODE_RX);
		const uint_fast8_t concat_order = (input_slice || single_slice) ? 0 : 3;
		const uint_fast8_t concat_enable = (input_slice || single_slice) ? 0 : 1;
		const uint_fast8_t pos = multi_slice ? 0x1f : 0x03;
		
		SGPIO_MUX_CFG(slice_index) =
		      SGPIO_MUX_CFG_CONCAT_ORDER(concat_order)
		    | SGPIO_MUX_CFG_CONCAT_ENABLE(concat_enable)
		    | SGPIO_MUX_CFG_QUALIFIER_SLICE_MODE(0)
		    | SGPIO_MUX_CFG_QUALIFIER_PIN_MODE(1)
		    | SGPIO_MUX_CFG_QUALIFIER_MODE(3)
		    | SGPIO_MUX_CFG_CLK_SOURCE_SLICE_MODE(0)
		    | SGPIO_MUX_CFG_CLK_SOURCE_PIN_MODE(0)
			| SGPIO_MUX_CFG_EXT_CLK_ENABLE(1)
			;

		SGPIO_SLICE_MUX_CFG(slice_index) =
		      SGPIO_SLICE_MUX_CFG_INV_QUALIFIER(0)
		    | SGPIO_SLICE_MUX_CFG_PARALLEL_MODE(3)
		    | SGPIO_SLICE_MUX_CFG_DATA_CAPTURE_MODE(0)
		    | SGPIO_SLICE_MUX_CFG_INV_OUT_CLK(0)
		    | SGPIO_SLICE_MUX_CFG_CLKGEN_MODE(1)
		    | SGPIO_SLICE_MUX_CFG_CLK_CAPTURE_MODE(1)
		    | SGPIO_SLICE_MUX_CFG_MATCH_MODE(0)
			;

		SGPIO_PRESET(slice_index) = 0;			// External clock, don't care
		SGPIO_COUNT(slice_index) = 0;				// External clock, don't care
		SGPIO_POS(slice_index) =
			  SGPIO_POS_POS_RESET(pos)
			| SGPIO_POS_POS(pos)
			;
		SGPIO_REG(slice_index) = 0x80808080;     // Primary output data register
		SGPIO_REG_SS(slice_index) = 0x80808080;  // Shadow output data register
		
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
