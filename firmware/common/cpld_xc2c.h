/*
 * Copyright 2019 Jared Boone <jared@sharebrained.com>
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

#ifndef __CPLD_XC2C_H__
#define __CPLD_XC2C_H__

#include <stdint.h>
#include <stdbool.h>

#include "cpld_jtag.h"

bool cpld_xc2c64a_jtag_checksum(const jtag_t* const jtag, uint32_t* const crc_value);

#endif/*__CPLD_XC2C_H__*/
