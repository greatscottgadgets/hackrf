/*
 * Copyright 2013 Michael Ossmann
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

/*
 * This is a rudimentary driver for the W25Q80BV SPI Flash IC using the
 * LPC43xx's SSP0 peripheral (not quad SPIFI).  The only goal here is to allow
 * programming the flash.
 */

#include <stdint.h>
#include "w25q80bv.h"
#include "hackrf_core.h"
#include <libopencm3/lpc43xx/ssp.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/gpio.h>

/*
 * Set up pins for GPIO and SPI control, configure SSP0 peripheral for SPI.
 * SSP0_SSEL is controlled by GPIO in order to handle various transfer lengths.
 */

void w25q80bv_setup(void)
{
	/* FIXME speed up once everything is working reliably */
	const uint8_t serial_clock_rate = 32;
	const uint8_t clock_prescale_rate = 128;

	/* configure SSP pins */
	scu_pinmux(SCU_SSP0_MISO, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP0_MOSI, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP0_SCK,  (SCU_SSP_IO | SCU_CONF_FUNCTION2));

	/* configure GPIO pins */
	scu_pinmux(SCU_FLASH_HOLD, SCU_GPIO_FAST);
	scu_pinmux(SCU_FLASH_WP, SCU_GPIO_FAST);
	scu_pinmux(SCU_SSP0_SSEL, (SCU_GPIO_FAST | SCU_CONF_FUNCTION4));

	/* drive SSEL, HOLD, and WP pins high */
	gpio_set(PORT_FLASH, (PIN_FLASH_HOLD | PIN_FLASH_WP));
	gpio_set(PORT_SSP0_SSEL, PIN_SSP0_SSEL);

	/* Set GPIO pins as outputs. */
	GPIO1_DIR |= (PIN_FLASH_HOLD | PIN_FLASH_WP);
	GPIO5_DIR |= PIN_SSP0_SSEL;

	/* initialize SSP0 */
	ssp_init(SSP0_NUM,
			SSP_DATA_8BITS,
			SSP_FRAME_SPI,
			SSP_CPOL_0_CPHA_0,
			serial_clock_rate,
			clock_prescale_rate,
			SSP_MODE_NORMAL,
			SSP_MASTER,
			SSP_SLAVE_OUT_ENABLE);
}

uint8_t w25q80bv_get_status(void)
{
	uint8_t value;

	gpio_clear(PORT_SSP0_SSEL, PIN_SSP0_SSEL);
	ssp_transfer(SSP0_NUM, W25Q80BV_READ_STATUS1);
	value = ssp_transfer(SSP0_NUM, 0xFF);
	gpio_set(PORT_SSP0_SSEL, PIN_SSP0_SSEL);

	return value;
}

void w25q80bv_wait_while_busy(void)
{
	while (w25q80bv_get_status() & W25Q80BV_STATUS_BUSY);
}

void w25q80bv_write_enable(void)
{
	w25q80bv_wait_while_busy();
	gpio_clear(PORT_SSP0_SSEL, PIN_SSP0_SSEL);
	ssp_transfer(SSP0_NUM, W25Q80BV_WRITE_ENABLE);
	gpio_set(PORT_SSP0_SSEL, PIN_SSP0_SSEL);
}

void w25q80bv_chip_erase(void)
{
	w25q80bv_write_enable();
	w25q80bv_wait_while_busy();
	gpio_clear(PORT_SSP0_SSEL, PIN_SSP0_SSEL);
	ssp_transfer(SSP0_NUM, W25Q80BV_CHIP_ERASE);
	gpio_set(PORT_SSP0_SSEL, PIN_SSP0_SSEL);
}

/* write up a 256 byte page or partial page */
void w25q80bv_page_program(const uint32_t addr, const uint16_t len, const uint8_t* data)
{
	int i;

	/* do nothing if asked to write beyond a page boundary */
	if (((addr & 0xFF) + len) > W25Q80BV_PAGE_LEN)
		return;

	/* do nothing if we would overflow the flash */
	if (addr > (W25Q80BV_NUM_BYTES - len))
		return;

	w25q80bv_write_enable();
	w25q80bv_wait_while_busy();

	gpio_clear(PORT_SSP0_SSEL, PIN_SSP0_SSEL);
	ssp_transfer(SSP0_NUM, W25Q80BV_PAGE_PROGRAM);
	ssp_transfer(SSP0_NUM, (addr & 0xFF0000) >> 16);
	ssp_transfer(SSP0_NUM, (addr & 0xFF00) >> 8);
	ssp_transfer(SSP0_NUM, addr & 0xFF);
	for (i = 0; i < len; i++)
		ssp_transfer(SSP0_NUM, data[i]);
	gpio_set(PORT_SSP0_SSEL, PIN_SSP0_SSEL);
}

/* write an arbitrary number of bytes */
void w25q80bv_program(uint32_t addr, uint32_t len, const uint8_t* data)
{
	uint16_t first_block_len;

	/* do nothing if we would overflow the flash */
	if ((len > W25Q80BV_NUM_BYTES) || (addr > W25Q80BV_NUM_BYTES)
			|| ((addr + len) > W25Q80BV_NUM_BYTES))
		return;

	/* handle start not at page boundary */
	first_block_len = W25Q80BV_PAGE_LEN - (addr % W25Q80BV_PAGE_LEN);
	if (len < first_block_len)
		first_block_len = len;
	if (first_block_len) {
		w25q80bv_page_program(addr, first_block_len, data);
		addr += first_block_len;
		data += first_block_len;
		len -= first_block_len;
	}

	/* one page at a time on boundaries */
	while (len >= W25Q80BV_PAGE_LEN) {
		w25q80bv_page_program(addr, W25Q80BV_PAGE_LEN, data);
		addr += W25Q80BV_PAGE_LEN;
		data += W25Q80BV_PAGE_LEN;
		len -= W25Q80BV_PAGE_LEN;
	}

	/* handle end not at page boundary */
	if (len) {
		w25q80bv_page_program(addr, len, data);
	}
}
