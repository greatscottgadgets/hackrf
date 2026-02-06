/*
 * Copyright 2024 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include "ice40_spi.h"

#include <libopencm3/lpc43xx/scu.h>
#include "hackrf_core.h"
#include "delay.h"

void ice40_spi_target_init(ice40_spi_driver_t* const drv)
{
	/* Configure SSP1 Peripheral and relevant FPGA pins. */
	scu_pinmux(SCU_SSP1_CIPO, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP1_COPI, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP1_SCK, (SCU_SSP_IO | SCU_CONF_FUNCTION1));
	scu_pinmux(SCU_PINMUX_FPGA_CRESET, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_FPGA_CDONE, SCU_GPIO_PUP | SCU_CONF_FUNCTION4);
	scu_pinmux(SCU_PINMUX_FPGA_SPI_CS, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);

	/* Configure GPIOs as inputs or outputs as needed. */
	gpio_clear(drv->gpio_creset);
	gpio_output(drv->gpio_creset);
	gpio_input(drv->gpio_cdone);
	// select is configured in SSP code
}

uint8_t ice40_spi_read(ice40_spi_driver_t* const drv, uint8_t r)
{
	uint8_t value[3] = {r & 0x7F, 0, 0};
	spi_bus_transfer(drv->bus, value, 3);
	return value[2];
}

void ice40_spi_write(ice40_spi_driver_t* const drv, uint8_t r, uint16_t v)
{
	uint8_t value[3] = {(r & 0x7F) | 0x80, v, 0};
	spi_bus_transfer(drv->bus, value, 3);
}

static void spi_ssp1_wait_for_tx_fifo_not_full(void)
{
	while ((SSP_SR(SSP1_BASE) & SSP_SR_TNF) == 0) {}
}

static void spi_ssp1_wait_for_rx_fifo_not_empty(void)
{
	while ((SSP_SR(SSP1_BASE) & SSP_SR_RNE) == 0) {}
}

static void spi_ssp1_wait_for_not_busy(void)
{
	while (SSP_SR(SSP1_BASE) & SSP_SR_BSY) {}
}

static uint32_t spi_ssp1_transfer_word(const uint32_t data)
{
	spi_ssp1_wait_for_tx_fifo_not_full();
	SSP_DR(SSP1_BASE) = data;
	spi_ssp1_wait_for_not_busy();
	spi_ssp1_wait_for_rx_fifo_not_empty();
	return SSP_DR(SSP1_BASE);
}

bool ice40_spi_syscfg_program(
	ice40_spi_driver_t* const drv,
	size_t (*read_block_cb)(void* ctx, uint8_t* buffer),
	void* read_ctx)
{
	// Drive CRESET_B = 0, SPI_SS = 0, SPI_SCK = 1.
	gpio_clear(drv->gpio_creset);
	gpio_clear(drv->gpio_select);

	// Wait a minimum of 200 ns.
	delay_us_at_mhz(1, 204 / 4); // 250 ns.

	// Release CRESET_B or drive CRESET_B = 1.
	gpio_set(drv->gpio_creset);

	// Wait a minimum of 1200 μs to clear internal configuration memory.
	// Testing showed us that we need to wait longer. Let's wait 1800 μs.
	delay_us_at_mhz(1800, 204);

	// Set SPI_SS = 1, Send 8 dummy clocks.
	gpio_set(drv->gpio_select);
	spi_ssp1_transfer_word(0);

	// Send configuration image serially on SPI_SI to iCE40, most-significant bit
	// first, on falling edge of SPI_SCK.
	uint8_t out_buffer[4096] = {0};
	gpio_clear(drv->gpio_select);
	for (;;) {
		size_t read_sz = read_block_cb(read_ctx, out_buffer);
		if (read_sz == 0)
			break;
		for (size_t j = 0; j < read_sz; j++) {
			spi_ssp1_transfer_word(out_buffer[j]);
		}
	}

	// Wait for 100 clocks cycles for CDONE to go high.
	gpio_set(drv->gpio_select);
	for (size_t j = 0; j < 13; j++) {
		spi_ssp1_transfer_word(0);
	}

	return gpio_read(drv->gpio_cdone);
}
