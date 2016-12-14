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

#ifndef __OPERACAKE_H
#define __OPERACAKE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "i2c_bus.h"

uint8_t operacake_init(void);
uint8_t operacake_set_ports(uint8_t PA, uint8_t PB);

#ifdef __cplusplus
}
#endif

#endif /* __OPERACAKE_H */
