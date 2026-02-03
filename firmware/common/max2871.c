/*
 * Copyright 2015-2022 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include "max2871.h"
#include "max2871_regs.h"
#include "selftest.h"

#if (defined DEBUG)
	#include <stdio.h>
	#define LOG printf
#else
	#define LOG(x, ...)
	#include "platform_scu.h"
#endif

#include <stdint.h>
#include <string.h>

static uint32_t max2871_spi_read(max2871_driver_t* const drv);
static void max2871_spi_write(max2871_driver_t* const drv, uint8_t r, uint32_t v);
static void max2871_write_registers(max2871_driver_t* const drv);
static void delay_ms(int ms);

void max2871_setup(max2871_driver_t* const drv)
{
	const platform_scu_t* scu = platform_scu();

	/* Configure GPIO pins. */
	scu_pinmux(scu->VCO_CE, SCU_GPIO_FAST);
	scu_pinmux(scu->VCO_SCLK, SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
	/* Only used for the debug pin config: scu_pinmux(scu->VCO_SCLK, SCU_GPIO_FAST); */
	scu_pinmux(scu->VCO_SDATA, SCU_GPIO_FAST);
	scu_pinmux(scu->VCO_LE, SCU_GPIO_FAST);
	scu_pinmux(scu->VCO_MUX, SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
	scu_pinmux(scu->SYNT_RFOUT_EN, SCU_GPIO_FAST);

	/* Set GPIO pins as outputs. */
	gpio_output(drv->gpio_vco_ce);
	gpio_output(drv->gpio_vco_sclk);
	gpio_output(drv->gpio_vco_sdata);
	gpio_output(drv->gpio_vco_le);
	gpio_output(drv->gpio_synt_rfout_en);

	/* MUX is an input */
	gpio_input(drv->gpio_vco_mux);

	/* set to known state */
	gpio_set(drv->gpio_vco_ce); /* active high */
	gpio_clear(drv->gpio_vco_sclk);
	gpio_clear(drv->gpio_vco_sdata);
	gpio_set(drv->gpio_vco_le);        /* active low */
	gpio_set(drv->gpio_synt_rfout_en); /* active high */

	selftest.mixer_id = max2871_spi_read(drv) >> MAX2871_DIE_SHIFT;
	if (selftest.mixer_id != 7) {
		selftest.report.pass = false;
	}

	max2871_regs_init();
	int i;
	for (i = 5; i >= 0; i--) {
		max2871_spi_write(drv, i, max2871_get_register(i));
		delay_ms(20);
	}

	max2871_set_INT(1);
	max2871_set_N(4500);
	max2871_set_FRAC(0);
	max2871_set_CPL(0);
	max2871_set_CPT(0);
	max2871_set_P(1);
	max2871_set_M(0);
	max2871_set_LDS(0);
	max2871_set_SDN(0);
	max2871_set_MUX(0x0C); /* Register 6 readback */
	max2871_set_DBR(0);
	max2871_set_RDIV2(0);
	max2871_set_R(1); /* 40 MHz f_PFD */
	max2871_set_REG4DB(1);
	max2871_set_CP(15); /* ?: CP charge pump current 0-15 */
	max2871_set_LDF(1); /* INT-N */
	max2871_set_LDP(0); /* ?: Lock-Detect Precision */
	max2871_set_PDP(1);
	max2871_set_SHDN(0);
	max2871_set_TRI(0);
	max2871_set_RST(0);
	max2871_set_VCO(0);
	max2871_set_VAS_SHDN(0);
	max2871_set_VAS_TEMP(1);
	max2871_set_CSM(0);
	max2871_set_MUTEDEL(1);
	max2871_set_CDM(0);
	max2871_set_CDIV(0);
	max2871_set_SDLDO(0);
	max2871_set_SDDIV(0);
	max2871_set_SDREF(0);
	max2871_set_BS(20 * 40); /* For 40 MHz f_PFD */
	max2871_set_FB(1);       /* Do not put DIVA into the feedback loop */
	max2871_set_DIVA(0);
	max2871_set_SDVCO(0);
	max2871_set_MTLD(1);
	max2871_set_BDIV(0);
	max2871_set_RFB_EN(0);
	max2871_set_BPWR(0);
	max2871_set_RFA_EN(0);
	max2871_set_APWR(3);
	max2871_set_SDPLL(0);
	max2871_set_F01(1);
	max2871_set_LD(1);
	max2871_set_ADCS(0);
	max2871_set_ADCM(0);

	max2871_write_registers(drv);

	max2871_set_frequency(drv, 3500);
}

static void delay_ms(int ms)
{
	uint32_t i;
	while (ms--) {
		for (i = 0; i < 20000; i++) {
			__asm__("nop");
		}
	}
}

static void serial_delay(void)
{
	uint32_t i;

	for (i = 0; i < 2; i++)
		__asm__("nop");
}

/* SPI register write
 *
 * Send 32 bits:
 *  First 29 bits are data
 *  Last 3 bits are register number */
static void max2871_spi_write(max2871_driver_t* const drv, uint8_t r, uint32_t v)
{
#if DEBUG
	LOG("0x%04x -> reg%d\n", v, r);
#else

	uint32_t bits = 32;
	uint32_t msb = 1 << (bits - 1);
	uint32_t data = v | r;

	/* make sure everything is starting in the correct state */
	gpio_set(drv->gpio_vco_le);
	gpio_clear(drv->gpio_vco_sclk);
	gpio_clear(drv->gpio_vco_sdata);

	/* start transaction by bringing LE low */
	gpio_clear(drv->gpio_vco_le);

	while (bits--) {
		if (data & msb)
			gpio_set(drv->gpio_vco_sdata);
		else
			gpio_clear(drv->gpio_vco_sdata);
		data <<= 1;

		serial_delay();
		gpio_set(drv->gpio_vco_sclk);

		serial_delay();
		gpio_clear(drv->gpio_vco_sclk);
	}

	gpio_set(drv->gpio_vco_le);
#endif
}

static uint32_t max2871_spi_read(max2871_driver_t* const drv)
{
	uint32_t bits = 32;
	uint32_t data = 0;

	max2871_spi_write(drv, 0x06, 0x0);

	serial_delay();
	gpio_set(drv->gpio_vco_sclk);
	serial_delay();
	gpio_clear(drv->gpio_vco_sclk);
	serial_delay();

	while (bits--) {
		gpio_set(drv->gpio_vco_sclk);
		serial_delay();

		gpio_clear(drv->gpio_vco_sclk);
		serial_delay();

		data <<= 1;
		data |= gpio_read(drv->gpio_vco_mux) ? 1 : 0;
	}
	return data;
}

static void max2871_write_registers(max2871_driver_t* const drv)
{
	int i;
	for (i = 5; i >= 0; i--) {
		max2871_spi_write(drv, i, max2871_get_register(i));
	}
}

/* Set frequency (MHz). */
uint64_t max2871_set_frequency(max2871_driver_t* const drv, uint16_t mhz)
{
	int n = mhz / 40;
	int diva = 0;

	while (n * 40 < 3000) {
		n *= 2;
		diva += 1;
	}

	max2871_set_RFA_EN(0);
	max2871_write_registers(drv);

	max2871_set_N(n);
	max2871_set_DIVA(diva);
	max2871_write_registers(drv);

	while (max2871_spi_read(drv) & MAX2871_VASA) {}

	max2871_set_RFA_EN(1);
	max2871_write_registers(drv);

	return (mhz / 40) * 40 * 1000000;
}

void max2871_enable(max2871_driver_t* const drv)
{
	gpio_set(drv->gpio_vco_ce);
}

void max2871_disable(max2871_driver_t* const drv)
{
	gpio_clear(drv->gpio_vco_ce);
}
