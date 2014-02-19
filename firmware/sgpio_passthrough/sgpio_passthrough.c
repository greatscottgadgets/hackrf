/*
 * Copyright 2012 Michael Ossmann
 * Copyright (C) 2012 Jared Boone
 * Copyright (C) 2012 Benjamin Vernoux
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

void test_sgpio_sliceA_D(void)
{
	SGPIO_GPIO_OENREG = 0; // All inputs for the moment.

	// Disable all counters during configuration
	SGPIO_CTRL_ENABLE = 0;

	// Configure pin functions.
	configure_sgpio_pin_functions();

	/****************************************************/
	/* Enable SGPIO pin outputs. */
	/****************************************************/
	SGPIO_GPIO_OENREG =
        0xFFFF;           // data: output for SGPIO0 to SGPIO15

	/*******************************************************************************/
	/* SGPIO pin 0 outputs slice A bit 0. (see Table 212. Output pin multiplexing) */
	/*******************************************************************************/
	SGPIO_OUT_MUX_CFG(0) =
		(0L <<  4) |    // P_OE_CFG = X
		(0L <<  0);     // P_OUT_CFG = 0, dout_doutm1 (1-bit mode)

	// SGPIO pin 12 outputs slice D bit 0. (see Table 212. Output pin multiplexing)
	SGPIO_OUT_MUX_CFG(12) =
		(0L <<  4) |    // P_OE_CFG = X
		(0L <<  0);     // P_OUT_CFG = 0, dout_doutm1 (1-bit mode)

	/****************************************************/
	/* Slice A */
	/****************************************************/
	SGPIO_MUX_CFG(SGPIO_SLICE_A) = 
		(0L << 12) |    // CONCAT_ORDER = 0 (self-loop)
		(1L << 11) |    // CONCAT_ENABLE = 1 (concatenate data)
		(0L <<  9) |    // QUALIFIER_SLICE_MODE = X
		(0L <<  7) |    // QUALIFIER_PIN_MODE = X
		(0L <<  5) |    // QUALIFIER_MODE = 0 (enable)
		(0L <<  3) |    // CLK_SOURCE_SLICE_MODE = 0, slice D
		(0L <<  1) |    // CLK_SOURCE_PIN_MODE = X
		(0L <<  0);     // EXT_CLK_ENABLE = 0, internal clock signal (slice)

	SGPIO_SLICE_MUX_CFG(SGPIO_SLICE_A) =
		(0L <<  8) |    // INV_QUALIFIER = 0 (use normal qualifier)
		(0L <<  6) |    // PARALLEL_MODE = 0 (shift 1 bit per clock)
		(0L <<  4) |    // DATA_CAPTURE_MODE = 0 (detect rising edge)
		(0L <<  3) |    // INV_OUT_CLK = 0 (normal clock)
		(0L <<  2) |    // CLKGEN_MODE = 0 (use clock from COUNTER)
		(0L <<  1) |    // CLK_CAPTURE_MODE = 0 (use rising clock edge)
		(0L <<  0);     // MATCH_MODE = 0 (do not match data)

	SGPIO_PRESET(SGPIO_SLICE_A) = 1;
	SGPIO_COUNT(SGPIO_SLICE_A) = 0;
	SGPIO_POS(SGPIO_SLICE_A) = (0x1FL << 8) | (0x1FL << 0);
	SGPIO_REG(SGPIO_SLICE_A) = 0xAAAAAAAA;     // Primary output data register
	SGPIO_REG_SS(SGPIO_SLICE_A) = 0xAAAAAAAA;  // Shadow output data register

	/****************************************************/
	/* Slice D (clock for Slice A) */
	/****************************************************/
	SGPIO_MUX_CFG(SGPIO_SLICE_D) = 
		(0L << 12) |    // CONCAT_ORDER = 0 (self-loop)
		(1L << 11) |    // CONCAT_ENABLE = 1 (concatenate data)
		(0L <<  9) |    // QUALIFIER_SLICE_MODE = X
		(0L <<  7) |    // QUALIFIER_PIN_MODE = X
		(0L <<  5) |    // QUALIFIER_MODE = 0 (enable)
		(0L <<  3) |    // CLK_SOURCE_SLICE_MODE = 0, slice D
		(0L <<  1) |    // CLK_SOURCE_PIN_MODE = X
		(0L <<  0);     // EXT_CLK_ENABLE = 0, internal clock signal (slice)

	SGPIO_SLICE_MUX_CFG(SGPIO_SLICE_D) =
		(0L <<  8) |    // INV_QUALIFIER = 0 (use normal qualifier)
		(0L <<  6) |    // PARALLEL_MODE = 0 (shift 1 bit per clock)
		(0L <<  4) |    // DATA_CAPTURE_MODE = 0 (detect rising edge)
		(0L <<  3) |    // INV_OUT_CLK = 0 (normal clock)
		(0L <<  2) |    // CLKGEN_MODE = 0 (use clock from COUNTER)
		(0L <<  1) |    // CLK_CAPTURE_MODE = 0 (use rising clock edge)
		(0L <<  0);     // MATCH_MODE = 0 (do not match data)

	SGPIO_PRESET(SGPIO_SLICE_D) = 0;
	SGPIO_COUNT(SGPIO_SLICE_D) = 0;
	SGPIO_POS(SGPIO_SLICE_D) = (0x1FL << 8) | (0x1FL << 0);
	SGPIO_REG(SGPIO_SLICE_D) = 0xAAAAAAAA;     // Primary output data register
	SGPIO_REG_SS(SGPIO_SLICE_D) = 0xAAAAAAAA;  // Shadow output data register


	/****************************************************/
	/* Start SGPIO operation by enabling slice clocks. */
	/****************************************************/
	SGPIO_CTRL_ENABLE =
		(1L <<  SGPIO_SLICE_D) |    // Slice D
		(1L <<  SGPIO_SLICE_A);     // Slice A
	// Start SGPIO operation by enabling slice clocks.

	/* 
		Expected: 
		SGPIO12 = MCU Freq/2
		SGPIO0  = SGPIO12/2 MHz= 51MHz (SliceD/2)
	*/

}


/*******************************************************************************/
/* Output 1bit table (see Table 212. Output pin multiplexing) */
/* SGPIO pin 00 outputs slice A bit 0. */
/* SGPIO pin 01 outputs slice I bit 0. */
/* SGPIO pin 02 outputs slice E bit 0. */
/* SGPIO pin 03 outputs slice J bit 0. */
/* SGPIO pin 04 outputs slice C bit 0. */
/* SGPIO pin 05 outputs slice K bit 0. */
/* SGPIO pin 06 outputs slice F bit 0. */
/* SGPIO pin 07 outputs slice L bit 0. */
/* SGPIO pin 08 outputs slice B bit 0. */
/* SGPIO pin 09 outputs slice M bit 0. */
/* SGPIO pin 10 outputs slice G bit 0. */
/* SGPIO pin 11 outputs slice N bit 0. */
/* SGPIO pin 12 outputs slice D bit 0. */
/* SGPIO pin 13 outputs slice O bit 0. */
/* SGPIO pin 14 outputs slice H bit 0. */
/* SGPIO pin 15 outputs slice P bit 0. */
/*******************************************************************************/
const uint8_t slice_preset_tab[16] =
{
	0,  /* Idx00 = Slice A => SGPIO0  Freq Div by 1=0 */
	8,  /* Idx01 = Slice B => SGPIO8  Freq Div by 9=8 */
	4,  /* Idx02 = Slice C => SGPIO4  Freq Div by 5=4 */
	12, /* Idx03 = Slice D => SGPIO12 Freq Div by 13=12 */
	2,  /* Idx04 = Slice E => SGPIO2 Freq Div by 3=2 */
	6,  /* Idx05 = Slice F => SGPIO6 Freq Div by 7=6 */
	10, /* Idx06 = Slice G => SGPIO10 Freq Div by 11=10 */
	14, /* Idx07 = Slice H => SGPIO14 Freq Div by 15=14 */
	1,  /* Idx08 = Slice I => SGPIO1 Freq Div by 2=1 */
	3,  /* Idx09 = Slice J => SGPIO3 Freq Div by 4=3 */
	5,  /* Idx10 = Slice K => SGPIO5 Freq Div by 6=5 */
	7,  /* Idx11 = Slice L => SGPIO7 Freq Div by 8=7 */
	9,  /* Idx12 = Slice M => SGPIO9 Freq Div by 10=9 */
	11, /* Idx13 = Slice N => SGPIO11 Freq Div by 12=11 */
	13, /* Idx14 = Slice O => SGPIO13 Freq Div by 14=13 */
	15  /* Idx15 = Slice P => SGPIO15 Freq Div by 16=15 */
};

void test_sgpio_all_slices(void)
{

	SGPIO_GPIO_OENREG = 0; // All inputs for the moment.

	// Disable all counters during configuration
	SGPIO_CTRL_ENABLE = 0;

	// Configure pin functions.
	configure_sgpio_pin_functions();

	/****************************************************/
	/* Enable SGPIO pin outputs. */
	/****************************************************/
	SGPIO_GPIO_OENREG =
        0xFFFF;           // data: output for SGPIO0 to SGPIO15

	for(uint_fast8_t i=0; i<16; i++) 
	{
		SGPIO_OUT_MUX_CFG(i) =
			(0L <<  4) |    // P_OE_CFG = X
			(0L <<  0);     // P_OUT_CFG = 0, dout_doutm1 (1-bit mode)
	}

	/****************************************************/
	/* Slice A to P */
	/****************************************************/
	for(uint_fast8_t i=0; i<16; i++) 
	{
		SGPIO_MUX_CFG(i) = 
			(0L << 12) |    // CONCAT_ORDER = 0 (self-loop)
			(1L << 11) |    // CONCAT_ENABLE = 1 (concatenate data)
			(0L <<  9) |    // QUALIFIER_SLICE_MODE = X
			(0L <<  7) |    // QUALIFIER_PIN_MODE = X
			(0L <<  5) |    // QUALIFIER_MODE = 0 (enable)
			(0L <<  3) |    // CLK_SOURCE_SLICE_MODE = 0, slice D
			(0L <<  1) |    // CLK_SOURCE_PIN_MODE = X
			(0L <<  0);     // EXT_CLK_ENABLE = 0, internal clock signal (slice)

		SGPIO_SLICE_MUX_CFG(i) =
			(0L <<  8) |    // INV_QUALIFIER = 0 (use normal qualifier)
			(0L <<  6) |    // PARALLEL_MODE = 0 (shift 1 bit per clock)
			(0L <<  4) |    // DATA_CAPTURE_MODE = 0 (detect rising edge)
			(0L <<  3) |    // INV_OUT_CLK = 0 (normal clock)
			(0L <<  2) |    // CLKGEN_MODE = 0 (use clock from COUNTER)
			(0L <<  1) |    // CLK_CAPTURE_MODE = 0 (use rising clock edge)
			(0L <<  0);     // MATCH_MODE = 0 (do not match data)

		SGPIO_PRESET(i) = slice_preset_tab[i];
		SGPIO_COUNT(i) = 0;
		SGPIO_POS(i) = (0x1FL << 8) | (0x1FL << 0);
		SGPIO_REG(i) = 0xAAAAAAAA;     // Primary output data register
		SGPIO_REG_SS(i) = 0xAAAAAAAA;  // Shadow output data register
	}

	/****************************************************/
	/* Start SGPIO operation by enabling slice clocks. */
	/****************************************************/
	SGPIO_CTRL_ENABLE = 0xFFFF; /* Start all slices A to P */
/*
		(1L <<  SGPIO_SLICE_D) |    // Slice D
		(1L <<  SGPIO_SLICE_A);     // Slice A
	// Start SGPIO operation by enabling slice clocks.
*/
	/* 
		Expected: 
		MCU Freq MHz = 204
		SGPIO Theorical Freq MHz
		SGPIO00	= 102,00000
		SGPIO01	= 51,00000
		SGPIO02	= 34,00000
		SGPIO03	= 25,50000
		SGPIO04	= 20,40000
		SGPIO05	= 17,00000
		SGPIO06	= 14,57143
		SGPIO07	= 12,75000
		SGPIO08	= 11,33333
		SGPIO09	= 10,20000
		SGPIO10	= 9,27273
		SGPIO11	= 8,50000
		SGPIO12	= 7,84615
		SGPIO13	= 7,28571
		SGPIO14	= 6,80000
		SGPIO15	= 6,37500
		TitanMKD: I have problems with my boards and this test see document Test_SGPIO0_to15.ods / Test_SGPIO0_to15.pdf
	*/
}

void test_sgpio_interface(void) 
{
	SGPIO_GPIO_OENREG = 0; // All inputs for the moment.

	// Disable all counters during configuration
	SGPIO_CTRL_ENABLE = 0;

	configure_sgpio_pin_functions();

	// Make all SGPIO controlled by SGPIO's "GPIO" registers
	for (uint_fast8_t i = 0; i < 16; i++) {
		SGPIO_OUT_MUX_CFG(i) = (0L << 4) | (4L << 0);
	}

	// Enable SGPIO pin outputs (SGPIO0 to 15).
	SGPIO_GPIO_OENREG = 0xFFFF;

	/* Set values for SGPIO0 to 15 */
	while (1)
	{
		// 750KHz => 272 cycles
		/*
		for (uint_fast8_t i = 0; i < 8; i++) {
			SGPIO_GPIO_OUTREG ^= (1L << i);

		}
		*/

		// 3.923 MHz => 52 cycles
		__asm__(" nop");
		__asm__(" nop");
		__asm__(" nop");
		__asm__(" nop");
		__asm__(" nop");
		__asm__(" nop");
		__asm__(" nop");
		__asm__(" nop");
		__asm__(" nop");
		__asm__(" nop");
		__asm__(" nop");
		__asm__(" nop");
		SGPIO_GPIO_OUTREG ^= 0x5555;

		// 7.28 MHz => 28 cycles
		/*
		SGPIO_GPIO_OUTREG ^= 0x5555;
		*/

		// 17 MHz => 12 cycles
		/*
		__asm__(" nop");
		__asm__(" nop");
		__asm__(" nop");
		__asm__(" nop");
		SGPIO_GPIO_OUTREG = 0x5555;
		__asm__(" nop");
		__asm__(" nop");
		__asm__(" nop");
		__asm__(" nop");
		SGPIO_GPIO_OUTREG = 0xAAAA;
		*/
		// 25.50 MHz => 8 cycles
		/*
		SGPIO_GPIO_OUTREG = 0x5555;
		SGPIO_GPIO_OUTREG = 0xAAAA;
		*/
	}

	/* TitanMKD: I have problems with my board with this test (see test_sgpio_all_slices()) */
}

int main(void) 
{
	pin_setup();
	enable_1v8_power();
	cpu_clock_init();
	ssp1_init();
	gpio_set(PORT_LED1_3, PIN_LED1);

	//test_sgpio_sliceA_D();
	test_sgpio_interface();
	//test_sgpio_all_slices();

	while(1);

	return 0;
}
