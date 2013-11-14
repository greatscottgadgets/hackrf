/*
 * Copyright 2010 - 2012 Michael Ossmann
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
#include <libopencm3/lpc43xx/i2c.h>
#include <libopencm3/lpc43xx/m4/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/scs.h>

#include "hackrf_core.h"

/* Global counter incremented by SysTick Interrupt each millisecond */
extern volatile uint32_t g_ulSysTickCount;
extern uint32_t g_NbCyclePerSecond;

extern uint32_t sys_tick_get_time_ms(void);
extern uint32_t sys_tick_delta_time_ms(uint32_t start, uint32_t end);
extern void sys_tick_wait_time_ms(uint32_t wait_ms);


uint32_t test_nb_instruction_per_sec_100_nop_asm(void)
{
	register uint32_t val __asm__("r0");

	__asm__(" ldr	r1, =g_ulSysTickCount");
	__asm__(" ldr	r2, [r1]"); /* g_ulSysTickCount */
	__asm__(" push	{r4}");
	__asm__(" movs	r0, #0");	/* nb_instructions_per_sec = 0; */
	__asm__(" movw	r4, #999");	/*  wait_ms = 1000; */

	__asm__("test_nb_instruction_per_sec_loop_100_nop:");
/* Start 100 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 100 nop */

	__asm__(" ldr	r3, [r1]"); /* tickms = g_ulSysTickCount */
	__asm__(" adds	r0, #108"); /* nb_instructions_per_sec += 108 */
	__asm__(" cmp	r2, r3");
	__asm__(" it	CS"); /* IF THEN Higher or same */
	__asm__(" addcs	r3, #34");
	__asm__(" subs	r3, r3,	r2");
	__asm__(" cmp	r3, r4");
	__asm__(" bls	test_nb_instruction_per_sec_loop_100_nop"); /* tickms < wait_ms ? */

	__asm__(" POP	{r4}");

	return val;
};

uint32_t test_nb_instruction_per_sec_105_nop_asm(void)
{
	register uint32_t val __asm__("r0");

	__asm__(" ldr	r1, =g_ulSysTickCount");
	__asm__(" ldr	r2, [r1]"); /* g_ulSysTickCount */
	__asm__(" push	{r4}");
	__asm__(" movs	r0, #0");	/* nb_instructions_per_sec = 0; */
	__asm__(" movw	r4, #999");	/*  wait_ms = 1000; */

	__asm__("test_nb_instruction_per_sec_loop_105_nop:");
/* Start 100 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 100 nop */

/* Start 5 nop */
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 5 nop */
/* Total 105 nop */

	__asm__(" ldr	r3, [r1]"); /* tickms = g_ulSysTickCount */
	__asm__(" adds	r0, #113"); /* nb_instructions_per_sec += 105+8=113; */
	__asm__(" cmp	r2, r3");
	__asm__(" it	CS"); /* IF THEN Higher or same */
	__asm__(" addcs	r3, #34");
	__asm__(" subs	r3, r3,	r2");
	__asm__(" cmp	r3, r4");
	__asm__(" bls	test_nb_instruction_per_sec_loop_105_nop"); /* tickms < wait_ms ? */

	__asm__(" POP	{r4}");

	return val;
};

uint32_t test_nb_instruction_per_sec_110_nop_asm(void)
{
	register uint32_t val __asm__("r0");

	__asm__(" ldr	r1, =g_ulSysTickCount");
	__asm__(" ldr	r2, [r1]"); /* g_ulSysTickCount */
	__asm__(" push	{r4}");
	__asm__(" movs	r0, #0");	/* nb_instructions_per_sec = 0; */
	__asm__(" movw	r4, #999");	/*  wait_ms = 1000; */

	__asm__("test_nb_instruction_per_sec_loop_110_nop:");
/* Start 100 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 100 nop */

/* Start 10 nop */
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
/* End 10 nop */
/* Total 110 nop */

	__asm__(" ldr	r3, [r1]"); /* tickms = g_ulSysTickCount */
	__asm__(" adds	r0, #118"); /* nb_instructions_per_sec += 118; */
	__asm__(" cmp	r2, r3");
	__asm__(" it	CS"); /* IF THEN Higher or same */
	__asm__(" addcs	r3, #34");
	__asm__(" subs	r3, r3,	r2");
	__asm__(" cmp	r3, r4");
	__asm__(" bls	test_nb_instruction_per_sec_loop_110_nop"); /* tickms < wait_ms ? */

	__asm__(" POP	{r4}");

	return val;
};

uint32_t test_nb_instruction_per_sec_115_nop_asm(void)
{
	register uint32_t val __asm__("r0");

	__asm__(" ldr	r1, =g_ulSysTickCount");
	__asm__(" ldr	r2, [r1]"); /* g_ulSysTickCount */
	__asm__(" push	{r4}");
	__asm__(" movs	r0, #0");	/* nb_instructions_per_sec = 0; */
	__asm__(" movw	r4, #999");	/*  wait_ms = 1000; */

	__asm__("test_nb_instruction_per_sec_loop_115_nop:");
/* Start 100 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 100 nop */

/* Start 15 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 15 nop */
/* Total 115 nop */

	__asm__(" ldr	r3, [r1]"); /* tickms = g_ulSysTickCount */
	__asm__(" adds	r0, #123"); /* nb_instructions_per_sec += 115+8 = 123; */
	__asm__(" cmp	r2, r3");
	__asm__(" it	CS"); /* IF THEN Higher or same */
	__asm__(" addcs	r3, #34");
	__asm__(" subs	r3, r3,	r2");
	__asm__(" cmp	r3, r4");
	__asm__(" bls	test_nb_instruction_per_sec_loop_115_nop"); /* tickms < wait_ms ? */

	__asm__(" POP	{r4}");

	return val;
};

uint32_t test_nb_instruction_per_sec_120_nop_asm(void)
{
	register uint32_t val __asm__("r0");

	__asm__(" ldr	r1, =g_ulSysTickCount");
	__asm__(" ldr	r2, [r1]"); /* g_ulSysTickCount */
	__asm__(" push	{r4}");
	__asm__(" movs	r0, #0");	/* nb_instructions_per_sec = 0; */
	__asm__(" movw	r4, #999");	/*  wait_ms = 1000; */

	__asm__("test_nb_instruction_per_sec_loop_120_nop:");
/* Start 100 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 100 nop */

/* Start 20 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 20 nop */
/* Total 120 nop */

	__asm__(" ldr	r3, [r1]"); /* tickms = g_ulSysTickCount */
	__asm__(" adds	r0, #128"); /* nb_instructions_per_sec += 128; */
	__asm__(" cmp	r2, r3");
	__asm__(" it	CS"); /* IF THEN Higher or same */
	__asm__(" addcs	r3, #34");
	__asm__(" subs	r3, r3,	r2");
	__asm__(" cmp	r3, r4");
	__asm__(" bls	test_nb_instruction_per_sec_loop_120_nop"); /* tickms < wait_ms ? */

	__asm__(" POP	{r4}");

	return val;
};

uint32_t test_nb_instruction_per_sec_150_nop_asm(void)
{
	register uint32_t val __asm__("r0");

	__asm__(" ldr	r1, =g_ulSysTickCount");
	__asm__(" ldr	r2, [r1]"); /* g_ulSysTickCount */
	__asm__(" push	{r4}");
	__asm__(" movs	r0, #0");	/* nb_instructions_per_sec = 0; */
	__asm__(" movw	r4, #999");	/*  wait_ms = 1000; */

	__asm__("test_nb_instruction_per_sec_loop_150_nop:");
/* Start 100 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 100 nop */

/* Start 50 nop */
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
	__asm__(" nop");
	__asm__(" nop");
/* End 50 nop */
/* Total 150 nop */

	__asm__(" ldr	r3, [r1]"); /* tickms = g_ulSysTickCount */
	__asm__(" adds	r0, #158"); /* nb_instructions_per_sec += 158; */
	__asm__(" cmp	r2, r3");
	__asm__(" it	CS"); /* IF THEN Higher or same */
	__asm__(" addcs	r3, #34");
	__asm__(" subs	r3, r3,	r2");
	__asm__(" cmp	r3, r4");
	__asm__(" bls	test_nb_instruction_per_sec_loop_150_nop"); /* tickms < wait_ms ? */

	__asm__(" POP	{r4}");

	return val;
};

uint32_t test_nb_instruction_per_sec_200_nop_asm(void)
{
	register uint32_t val __asm__("r0");

	__asm__(" ldr	r1, =g_ulSysTickCount");
	__asm__(" ldr	r2, [r1]"); /* g_ulSysTickCount */
	__asm__(" push	{r4}");
	__asm__(" movs	r0, #0");	/* nb_instructions_per_sec = 0; */
	__asm__(" movw	r4, #999");	/*  wait_ms = 1000; */

	__asm__("test_nb_instruction_per_sec_loop_200_nop:");
/* Start 100 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 100 nop */

/* Start 100 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 100 nop */
/* Total 200 nop */

	__asm__(" ldr	r3, [r1]"); /* tickms = g_ulSysTickCount */
	__asm__(" adds	r0, #208"); /* nb_instructions_per_sec += 208; */
	__asm__(" cmp	r2, r3");
	__asm__(" it	CS"); /* IF THEN Higher or same */
	__asm__(" addcs	r3, #34");
	__asm__(" subs	r3, r3,	r2");
	__asm__(" cmp	r3, r4");
	__asm__(" bls	test_nb_instruction_per_sec_loop_200_nop"); /* tickms < wait_ms ? */

	__asm__(" POP	{r4}");

	return val;
};

uint32_t test_nb_instruction_per_sec_1000_nop_asm(void)
{
	register uint32_t val __asm__("r0");

	__asm__(" ldr	r1, =g_ulSysTickCount");
	__asm__(" ldr	r2, [r1]"); /* g_ulSysTickCount */
	__asm__(" push	{r4}");
	__asm__(" movs	r0, #0");	/* nb_instructions_per_sec = 0; */
	__asm__(" movw	r4, #999");	/*  wait_ms = 1000; */

	__asm__("test_nb_instruction_per_sec_loop_1000nop:");
/* Start 100 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 100 nop */

/* Start 100 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 100 nop */

/* Start 100 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 100 nop */

/* Start 100 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 100 nop */

/* Start 100 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 100 nop */

/* Start 100 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 100 nop */

/* Start 100 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 100 nop */

/* Start 100 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 100 nop */

/* Start 100 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 100 nop */

/* Start 100 nop */
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
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
	__asm__(" nop");
/* End 100 nop */
/* Total 200 nop */

	__asm__(" ldr	r3, [r1]"); /* tickms = g_ulSysTickCount */
	__asm__(" adds	r0, #1008"); /* nb_instructions_per_sec += 1008; */
	__asm__(" cmp	r2, r3");
	__asm__(" it	CS"); /* IF THEN Higher or same */
	__asm__(" addcs	r3, #34");
	__asm__(" subs	r3, r3,	r2");
	__asm__(" cmp	r3, r4");
	__asm__(" bls	test_nb_instruction_per_sec_loop_1000nop"); /* tickms < wait_ms ? */

	__asm__(" POP	{r4}");

	return val;
};
