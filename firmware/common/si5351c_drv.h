/*
 * Copyright 2012 Michael Ossmann <mike@ossmann.com>
 * Copyright 2012 Jared Boone <jared@sharebrained.com>
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

#ifndef __SI5351C_DRV_H
#define __SI5351C_DRV_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

typedef struct {
	uint8_t i2c_address;
} si5351c_driver_t;

void si5351c_write_single(si5351c_driver_t* const drv, uint8_t reg, uint8_t val);
uint8_t si5351c_read_single(si5351c_driver_t* const drv, uint8_t reg);
void si5351c_write(si5351c_driver_t* const drv, uint8_t* const data, const uint_fast8_t data_count);

#ifdef __cplusplus
}
#endif

#endif /* __SI5351C_DRV_H */
