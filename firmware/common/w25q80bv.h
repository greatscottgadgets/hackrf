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

#ifndef __W25Q80BV_H__
#define __W25Q80BV_H__

#define W25Q80BV_PAGE_LEN     256U
#define W25Q80BV_NUM_PAGES    4096U
#define W25Q80BV_NUM_BYTES    1048576U

#define W25Q80BV_WRITE_ENABLE 0x06
#define W25Q80BV_CHIP_ERASE   0xC7
#define W25Q80BV_READ_STATUS1 0x05
#define W25Q80BV_PAGE_PROGRAM 0x02

#define W25Q80BV_STATUS_BUSY  0x01

void w25q80bv_setup(void);
void w25q80bv_chip_erase(void);
void w25q80bv_program(uint32_t addr, uint32_t len, const uint8_t* data);

#endif//__W25Q80BV_H__
