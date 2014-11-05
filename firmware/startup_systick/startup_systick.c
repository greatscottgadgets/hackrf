/*
 * Copyright 2012 Benjamin Vernoux
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
volatile uint32_t g_ulSysTickCount;
uint32_t g_NbCyclePerSecond;

void systick_setup(void)
{
	uint32_t systick_reload_val;
	g_ulSysTickCount = 0;

	/* Disable IRQ globally */
	__asm__("cpsid i");

	/* Set processor Clock as Source Clock */
	systick_set_clocksource(STK_CTRL_CLKSOURCE);

	/* Get SysTick calibration value to obtain by default 1 tick = 10ms */
	systick_reload_val = systick_get_calib();
	/*
     * Calibration seems wrong on LPC43xx(TBC) for default Freq it assume System Clock is 12MHz but it is 12*17=204MHz
	 * Fix the Calibration value bu multiplication by 17
     */
	systick_reload_val = (systick_reload_val*17);

	/* To obtain 1ms per tick just divide by 10 the 10ms base tick and set the reload */
	systick_reload_val = systick_reload_val/10;
	systick_set_reload(systick_reload_val);

	systick_interrupt_enable();

	/* Start counting. */
	systick_counter_enable();

	/* Set SysTick Priority to maximum */
	nvic_set_priority(NVIC_SYSTICK_IRQ, 0xFF);

	/* Enable IRQ globally */
	__asm__("cpsie i");
}

void scs_dwt_cycle_counter_enabled(void)
{
	SCS_DEMCR |= SCS_DEMCR_TRCENA;
	SCS_DWT_CTRL  |= SCS_DWT_CTRL_CYCCNTENA;
}

uint32_t sys_tick_get_time_ms(void)
{
    return g_ulSysTickCount;
}

uint32_t sys_tick_delta_time_ms(uint32_t start, uint32_t end)
{
	#define MAX_T_U32 ((2^32)-1)
    uint32_t diff;

    if(end > start)
    {
        diff=end-start;
    }else
    {
        diff=MAX_T_U32-(start-end)+1;
    }
    
    return diff;
}

void sys_tick_wait_time_ms(uint32_t wait_ms)
{
    uint32_t start, end;
    uint32_t tickms;

    start = sys_tick_get_time_ms();

    do
    {
        end = sys_tick_get_time_ms();
        tickms = sys_tick_delta_time_ms(start, end);
    }while(tickms < wait_ms);
}

/* Called each 1ms/1000Hz by interrupt
 1) Count the number of cycle per second.
 2) Increment g_ulSysTickCount counter.
*/
void sys_tick_handler(void)
{
	if(g_ulSysTickCount==0)
	{
		/* Clear Cycle Counter*/
		SCS_DWT_CYCCNT = 0;
	}else if(g_ulSysTickCount==1000)
	{
		/* Capture number of cycle elapsed during 1 second */
		g_NbCyclePerSecond = SCS_DWT_CYCCNT;
	}

	g_ulSysTickCount++;
}

int main(void)
{
	pin_setup();

	enable_1v8_power();

	cpu_clock_init();

	scs_dwt_cycle_counter_enabled();

	systick_setup();

	gpio_set(PORT_LED1_3, (PIN_LED1|PIN_LED2|PIN_LED3)); /* LEDs on */

	while (1) 
	{
		gpio_set(PORT_LED1_3, (PIN_LED1)); /* LED1 on */
		gpio_set(PORT_LED1_3, (PIN_LED2)); /* LED2 on */
		gpio_set(PORT_LED1_3, (PIN_LED3)); /* LED3 on */

		sys_tick_wait_time_ms(500);

		gpio_clear(PORT_LED1_3, (PIN_LED3)); /* LED3 off */
		gpio_clear(PORT_LED1_3, (PIN_LED2)); /* LED2 off */
		gpio_clear(PORT_LED1_3, (PIN_LED1)); /* LED1 off  */

		sys_tick_wait_time_ms(500);
	}

	return 0;
}
