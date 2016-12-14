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
#include "hackrf_core.h"

#define OPERACAKE_PIN_OE      (1<<7)
#define OPERACAKE_PIN_U2CTRL1 (1<<6)
#define OPERACAKE_PIN_U2CTRL0 (1<<5)
#define OPERACAKE_PIN_U3CTRL1 (1<<4)
#define OPERACAKE_PIN_U3CTRL0 (1<<3)
#define OPERACAKE_PIN_U1CTRL  (1<<2)
#define OPERACAKE_PIN_LEDEN2  (1<<1)
#define OPERACAKE_PIN_LEDEN   (1<<0)

#define OPERACAKE_PA1 (!OPERACAKE_PIN_U2CTRL0 | !OPERACAKE_PIN_U2CTRL1)
#define OPERACAKE_PA2 (OPERACAKE_PIN_U2CTRL0 | !OPERACAKE_PIN_U2CTRL1)
#define OPERACAKE_PA3 (!OPERACAKE_PIN_U2CTRL0 | OPERACAKE_PIN_U2CTRL1)
#define OPERACAKE_PA4 (OPERACAKE_PIN_U2CTRL0 | OPERACAKE_PIN_U2CTRL1)

#define OPERACAKE_PB1 (!OPERACAKE_PIN_U3CTRL0 | !OPERACAKE_PIN_U3CTRL1)
#define OPERACAKE_PB2 (OPERACAKE_PIN_U3CTRL0 | !OPERACAKE_PIN_U3CTRL1)
#define OPERACAKE_PB3 (!OPERACAKE_PIN_U3CTRL0 | OPERACAKE_PIN_U3CTRL1)
#define OPERACAKE_PB4 (OPERACAKE_PIN_U3CTRL0 | OPERACAKE_PIN_U3CTRL1)

#define OPERACAKE_CROSSOVER (!OPERACAKE_PIN_U1CTRL)
#define OPERACAKE_EN_LEDS (OPERACAKE_PIN_LEDEN2 | !OPERACAKE_PIN_LEDEN)

#define OPERACAKE_REG_INPUT    0x00
#define OPERACAKE_REG_OUTPUT   0x01
#define OPERACAKE_REG_POLARITY 0x02
#define OPERACAKE_REG_CONFIG   0x03

#define OPERACAKE_DEFAULT_OUTPUT (!OPERACAKE_PIN_OE | OPERACAKE_PA1 | OPERACAKE_PB1 | OPERACAKE_EN_LEDS)
#define OPERACAKE_DEFAULT_POLARITY (0x00)//(OPERACAKE_PIN_OE | OPERACAKE_PIN_LEDEN)
#define OPERACAKE_CONFIG_ALL_OUTPUT (0x00)

typedef struct {
	i2c_bus_t* const bus;
	uint8_t i2c_address;
} operacake_driver_t;

operacake_driver_t operacake_driver = {
	.bus = &i2c0,
	.i2c_address = 0x18,
};

uint8_t operacake_read_single(operacake_driver_t* drv, uint8_t reg);
void operacake_write(operacake_driver_t* drv, const uint8_t* const data, const size_t data_count);


uint8_t operacake_init(void) {
	/* TODO: detect Operacake */
	uint8_t polarity_data[] = {OPERACAKE_REG_POLARITY,
							   OPERACAKE_DEFAULT_POLARITY};
	operacake_write(&operacake_driver, polarity_data, 2);
	uint8_t output_data[] = {OPERACAKE_REG_OUTPUT,
							 OPERACAKE_DEFAULT_OUTPUT};
	operacake_write(&operacake_driver, output_data, 2);
	const uint8_t config_data[] = {OPERACAKE_REG_CONFIG,
								   OPERACAKE_CONFIG_ALL_OUTPUT};
	operacake_write(&operacake_driver, config_data, 2);
	return 0;
}

uint8_t operacake_set_ports(uint8_t PA, uint8_t PB) {
	return PA | PB;
}

/* read single register */
uint8_t operacake_read_single(operacake_driver_t* drv, uint8_t reg) {
	const uint8_t data_tx[] = { reg };
	uint8_t data_rx[] = { 0x00 };
	i2c_bus_transfer(drv->bus, drv->i2c_address, data_tx, 1, data_rx, 1);
	return data_rx[0];
}

/* Write to one of the PCA9557 registers */
void operacake_write(operacake_driver_t* drv, const uint8_t* const data, const size_t data_count) {
	i2c_bus_transfer(drv->bus, drv->i2c_address, data, data_count, NULL, 0);
}
