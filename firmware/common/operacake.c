/*
 * Copyright 2016 Dominic Spill <dominicgs@gmail.com>
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

#include "operacake.h"
#include "operacake_sctimer.h"
#include "hackrf_core.h"
#include "gpio.h"
#include "gpio_lpc.h"
#include "i2c_bus.h"
#include <libopencm3/lpc43xx/scu.h>

/*
 * I2C Mode
 */
#define OPERACAKE_PIN_OE(x)      (x<<7)
#define OPERACAKE_PIN_U2CTRL1(x) (x<<6)
#define OPERACAKE_PIN_U2CTRL0(x) (x<<5)
#define OPERACAKE_PIN_U3CTRL1(x) (x<<4)
#define OPERACAKE_PIN_U3CTRL0(x) (x<<3)
#define OPERACAKE_PIN_U1CTRL(x)  (x<<2)
#define OPERACAKE_PIN_LEDEN2(x)  (x<<1)
#define OPERACAKE_PIN_LEDEN(x)   (x<<0)

#define OPERACAKE_PORT_A1 (OPERACAKE_PIN_U2CTRL0(0) | OPERACAKE_PIN_U2CTRL1(0))
#define OPERACAKE_PORT_A2 (OPERACAKE_PIN_U2CTRL0(1) | OPERACAKE_PIN_U2CTRL1(0))
#define OPERACAKE_PORT_A3 (OPERACAKE_PIN_U2CTRL0(0) | OPERACAKE_PIN_U2CTRL1(1))
#define OPERACAKE_PORT_A4 (OPERACAKE_PIN_U2CTRL0(1) | OPERACAKE_PIN_U2CTRL1(1))

#define OPERACAKE_PORT_B1 (OPERACAKE_PIN_U3CTRL0(0) | OPERACAKE_PIN_U3CTRL1(0))
#define OPERACAKE_PORT_B2 (OPERACAKE_PIN_U3CTRL0(1) | OPERACAKE_PIN_U3CTRL1(0))
#define OPERACAKE_PORT_B3 (OPERACAKE_PIN_U3CTRL0(0) | OPERACAKE_PIN_U3CTRL1(1))
#define OPERACAKE_PORT_B4 (OPERACAKE_PIN_U3CTRL0(1) | OPERACAKE_PIN_U3CTRL1(1))

#define OPERACAKE_SAMESIDE  OPERACAKE_PIN_U1CTRL(1)
#define OPERACAKE_CROSSOVER OPERACAKE_PIN_U1CTRL(0)
#define OPERACAKE_EN_LEDS (OPERACAKE_PIN_LEDEN2(1) | OPERACAKE_PIN_LEDEN2(0))
#define OPERACAKE_GPIO_ENABLE OPERACAKE_PIN_OE(0)
#define OPERACAKE_GPIO_DISABLE OPERACAKE_PIN_OE(1)

#define OPERACAKE_REG_INPUT    0x00
#define OPERACAKE_REG_OUTPUT   0x01
#define OPERACAKE_REG_POLARITY 0x02
#define OPERACAKE_REG_CONFIG   0x03

#define OPERACAKE_DEFAULT_OUTPUT (OPERACAKE_GPIO_DISABLE | OPERACAKE_SAMESIDE \
								  | OPERACAKE_PORT_A1 | OPERACAKE_PORT_B1 \
								  | OPERACAKE_EN_LEDS)
#define OPERACAKE_CONFIG_ALL_OUTPUT (0x00)
// Leave LED bits as outputs
#define OPERACAKE_CONFIG_GPIO_INPUTS (0x7C)

#define OPERACAKE_POLARITY_NORMAL (0x00)

#define OPERACAKE_ADDRESS_DEFAULT 0x18
#define OPERACAKE_ADDRESS_INVALID 0xFF

#define OPERACAKE_MAX_BOARDS 8

i2c_bus_t* const oc_bus = &i2c0;

enum operacake_switching_mode {
	MODE_MANUAL    = 0,
	MODE_FREQUENCY = 1,
	MODE_TIME      = 2,
};

struct operacake_state {
	bool present;
	enum operacake_switching_mode mode;
	uint8_t PA;
	uint8_t PB;
};

struct operacake_state operacake_boards[OPERACAKE_MAX_BOARDS];
bool allow_gpio_mode = true;

/* read single register */
uint8_t operacake_read_reg(i2c_bus_t* const bus, uint8_t address, uint8_t reg) {
	// Convert from Opera Cake address (0-7) to I2C address
	address += OPERACAKE_ADDRESS_DEFAULT;

	const uint8_t data_tx[] = { reg };
	uint8_t data_rx[] = { 0x00 };
	i2c_bus_transfer(bus, address, data_tx, 1, data_rx, 1);
	return data_rx[0];
}

/* Write to one of the PCA9557 registers */
void operacake_write_reg(i2c_bus_t* const bus, uint8_t address, uint8_t reg, uint8_t value) {
	// Convert from Opera Cake address (0-7) to I2C address
	address += OPERACAKE_ADDRESS_DEFAULT;

	const uint8_t data[] = {reg, value};
	i2c_bus_transfer(bus, address, data, 2, NULL, 0);
}

uint8_t operacake_init(bool allow_gpio) {
	/* Find connected operacakes */
	for (int addr = 0; addr < 8; addr++) {
		operacake_write_reg(oc_bus, addr, OPERACAKE_REG_OUTPUT,
		                    OPERACAKE_DEFAULT_OUTPUT);
		operacake_write_reg(oc_bus, addr, OPERACAKE_REG_CONFIG,
		                    OPERACAKE_CONFIG_ALL_OUTPUT);
		uint8_t reg = operacake_read_reg(oc_bus, addr, OPERACAKE_REG_CONFIG);

		operacake_boards[addr].present = (reg == OPERACAKE_CONFIG_ALL_OUTPUT);
		operacake_boards[addr].mode    = MODE_MANUAL;
		operacake_boards[addr].PA      = OPERACAKE_PORT_A1;
		operacake_boards[addr].PB      = OPERACAKE_PORT_B1;
	}
	allow_gpio_mode = allow_gpio;
	if (allow_gpio) {
		operacake_sctimer_init();
	}
	return 0;
}

bool operacake_is_board_present(uint8_t address) {
	if (address >= OPERACAKE_MAX_BOARDS)
		return false;

	return operacake_boards[address].present;
}

void operacake_get_boards(uint8_t *addresses) {
	int count = 0;
	for (int i = 0; i < OPERACAKE_MAX_BOARDS; i++) {
		addresses[i] = OPERACAKE_ADDRESS_INVALID;
	}
	for (int i = 0; i < OPERACAKE_MAX_BOARDS; i++) {
		if (operacake_is_board_present(i))
			addresses[count++] = i;
	}
}

void operacake_set_mode(uint8_t address, uint8_t mode) {
	if (address >= OPERACAKE_MAX_BOARDS)
		return;

	operacake_boards[address].mode = mode;

	if (mode == MODE_TIME) {
		// Switch Opera Cake to pin-control mode
		uint8_t config_pins = (uint8_t)~(OPERACAKE_PIN_OE(1) | OPERACAKE_PIN_LEDEN(1) | OPERACAKE_PIN_LEDEN2(1));
		operacake_write_reg(oc_bus, address, OPERACAKE_REG_CONFIG, config_pins);
		operacake_write_reg(oc_bus, address, OPERACAKE_REG_OUTPUT, OPERACAKE_GPIO_ENABLE | OPERACAKE_EN_LEDS);
	} else {
		operacake_write_reg(oc_bus, address, OPERACAKE_REG_CONFIG, OPERACAKE_CONFIG_ALL_OUTPUT);
		operacake_set_ports(address, operacake_boards[address].PA, operacake_boards[address].PB);
	}

	// If any boards are in MODE_TIME, enable the sctimer events.
	bool enable_sctimer = false;
	for (int i = 0; i < OPERACAKE_MAX_BOARDS; i++) {
		if (operacake_boards[i].mode == MODE_TIME)
			enable_sctimer = true;
	}
	operacake_sctimer_enable(enable_sctimer);
}

uint8_t operacake_get_mode(uint8_t address) {
	if (address >= OPERACAKE_MAX_BOARDS)
		return 0;

	return operacake_boards[address].mode;
}

uint8_t port_to_pins(uint8_t port) {
	switch(port) {
		case OPERACAKE_PA1:
			return OPERACAKE_PORT_A1;
		case OPERACAKE_PA2:
			return OPERACAKE_PORT_A2;
		case OPERACAKE_PA3:
			return OPERACAKE_PORT_A3;
		case OPERACAKE_PA4:
			return OPERACAKE_PORT_A4;

		case OPERACAKE_PB1:
			return OPERACAKE_PORT_B1;
		case OPERACAKE_PB2:
			return OPERACAKE_PORT_B2;
		case OPERACAKE_PB3:
			return OPERACAKE_PORT_B3;
		case OPERACAKE_PB4:
			return OPERACAKE_PORT_B4;
	}
	return 0xFF;
}

uint8_t operacake_set_ports(uint8_t address, uint8_t PA, uint8_t PB) {
	uint8_t side, pa, pb, reg;
	/* Start with some error checking,
	 * which should have been done either
	 * on the host or elsewhere in firmware
	 */
	if((PA > OPERACAKE_PB4) || (PB > OPERACAKE_PB4)) {
		return 1;
	}
	/* Check which side PA and PB are on */
	if(((PA <= OPERACAKE_PA4) && (PB <= OPERACAKE_PA4))
	    || ((PA > OPERACAKE_PA4) && (PB > OPERACAKE_PA4))) {
		return 1;
	}

	if(PA > OPERACAKE_PA4) {
		side = OPERACAKE_CROSSOVER;
	} else {
		side = OPERACAKE_SAMESIDE;
	}

	// Keep track of manual settings for when we switch in/out of time/frequency modes.
	operacake_boards[address].PA = PA;
	operacake_boards[address].PB = PB;

	// Only apply register settings if the board is in manual mode.
	if (operacake_boards[address].mode != MODE_MANUAL)
		return 0;

	pa = port_to_pins(PA);
	pb = port_to_pins(PB);

	reg = (OPERACAKE_GPIO_DISABLE | side
					| pa | pb | OPERACAKE_EN_LEDS);
	operacake_write_reg(oc_bus, address, OPERACAKE_REG_OUTPUT, reg);
	return 0;
}

/*
 * Opera Glasses
 */
typedef struct {
	uint16_t freq_min;
	uint16_t freq_max;
	uint8_t portA;
	uint8_t portB;
} operacake_range;

static operacake_range ranges[MAX_OPERACAKE_RANGES * sizeof(operacake_range)];
static uint8_t range_idx = 0;

uint8_t operacake_add_range(uint16_t freq_min, uint16_t freq_max, uint8_t port) {
	if(range_idx >= MAX_OPERACAKE_RANGES) {
		return 1;
	}
	ranges[range_idx].freq_min = freq_min;
	ranges[range_idx].freq_max = freq_max;
	ranges[range_idx].portA = port;
	ranges[range_idx].portB = 7;
	if(port <= OPERACAKE_PA4) {
		ranges[range_idx].portB = range_idx+4;
	} else {
		ranges[range_idx].portB = OPERACAKE_PA1;
	}
	range_idx++;
	return 0;
}

#define FREQ_ONE_MHZ (1000000ull)
static uint8_t current_range = 0xFF;

uint8_t operacake_set_range(uint32_t freq_mhz) {
	if(range_idx == 0) {
		return 1;
	}

	int range;
	for(range=0; range<range_idx; range++) {
		if((freq_mhz >= ranges[range].freq_min)
		  && (freq_mhz <= ranges[range].freq_max)) {
			break;
		}
	}
	if(range == current_range) {
		return 1;
	}

	for (int i = 0; i < OPERACAKE_MAX_BOARDS; i++) {
		if (operacake_is_board_present(i) && operacake_get_mode(i) == MODE_FREQUENCY) {
			operacake_set_ports(i, ranges[range].portA, ranges[range].portB);
			break;
		}
	}

	current_range = range;
	return 0;
}

/*
 * GPIO
 */
uint16_t gpio_test(uint8_t address)
{
	uint8_t i, reg, bit_mask, gpio_mask = 0x1F;
	uint16_t result = 0;
	if(!allow_gpio_mode)
		return 0xFFFF;

	scu_pinmux(SCU_PINMUX_GPIO3_8, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_GPIO3_12, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_GPIO3_13, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_GPIO3_14, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_GPIO3_15, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);

	static struct gpio_t gpio_pins[] = {
		GPIO(3, 8),  // u1ctrl   IO2
		GPIO(3, 14), // u3ctrl0  IO3
		GPIO(3, 15), // u3ctrl1  IO4
		GPIO(3, 12), // u2ctrl0  IO5
		GPIO(3, 13)  // u2ctrl1  IO6
	};
	// Setup I2C to put it in GPIO mode
	reg = (OPERACAKE_GPIO_ENABLE | OPERACAKE_EN_LEDS);
	operacake_write_reg(oc_bus, address, OPERACAKE_REG_OUTPUT, reg);
	operacake_write_reg(oc_bus, address, OPERACAKE_REG_CONFIG,
		                OPERACAKE_CONFIG_GPIO_INPUTS);
	operacake_write_reg(oc_bus, address, OPERACAKE_REG_POLARITY,
		                OPERACAKE_POLARITY_NORMAL);
	// clear state
	for(i=0; i<5; i++) {
		gpio_output(&gpio_pins[i]);
		gpio_write(&gpio_pins[i], 0);
	}
	// Test each pin separately
	for(i=0; i<5; i++) {
		// Set pin high
		gpio_write(&gpio_pins[i], 1);
		// check input
		reg = operacake_read_reg(oc_bus, address, OPERACAKE_REG_INPUT);
		reg >>= 2;
		reg &= gpio_mask;
		bit_mask = 1 << i;
		result <<= 1;
		if(!(reg & bit_mask)) {
			// Is the correct bit set?
			result |= 1;
		}
		result <<= 1;
		if(reg & ~bit_mask) {
			// Are any other bits set?
			result |= 1;
		}
		result <<= 1;
		// set pin low
		gpio_write(&gpio_pins[i], 0);
		// check input
		reg = operacake_read_reg(oc_bus, address, OPERACAKE_REG_INPUT);
		reg >>= 2;
		reg &= gpio_mask;
		bit_mask = 1 << i;
		if(reg & bit_mask) {
			// Is the correct bit clear?
			result |= 1;
		}
	}

	// clean up
	for(i=0; i<5; i++) {
		gpio_input(&gpio_pins[i]);
	}

	// Put it back in to I2C mode and set default pins
	operacake_write_reg(oc_bus, address, OPERACAKE_REG_CONFIG,
		                    OPERACAKE_CONFIG_ALL_OUTPUT);
	operacake_write_reg(oc_bus, address, OPERACAKE_REG_OUTPUT,
		                OPERACAKE_DEFAULT_OUTPUT);
	return result;
}
