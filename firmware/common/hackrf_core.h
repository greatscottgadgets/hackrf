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

#ifndef __HACKRF_CORE_H
#define __HACKRF_CORE_H

#include "lpc43.h"

/* hardware identification number */
#define BOARD_ID_JELLYBEAN 0

#ifdef JELLYBEAN
#define BOARD_ID BOARD_ID_JELLYBEAN
#endif

/* GPIO pins */
//#ifdef JELLYBEAN
#define PIN_LED1 (1 << 1) /* GPIO2[1] on P4_1 */
#define PIN_LED2 (1 << 2) /* GPIO2[2] on P4_2 */
#define PIN_LED3 (1 << 8) /* GPIO2[8] on P6_12 */
//#endif

/* indicator LED control */
//#ifdef JELLYBEAN
#define LED1     (GPIO_PIN2 & PIN_LED1)
#define LED1_SET (GPIO_SET2 = PIN_LED1)
#define LED1_CLR (GPIO_CLR2 = PIN_LED1)
#define LED2     (GPIO_PIN2 & PIN_LED2)
#define LED2_SET (GPIO_SET2 = PIN_LED2)
#define LED2_CLR (GPIO_CLR2 = PIN_LED2)
#define LED3     (GPIO_PIN2 & PIN_LED3)
#define LED3_SET (GPIO_SET2 = PIN_LED3)
#define LED3_CLR (GPIO_CLR2 = PIN_LED3)
//#endif

void gpio_init(void);
void all_pins_off(void);

#endif /* __HACKRF_CORE_H */
