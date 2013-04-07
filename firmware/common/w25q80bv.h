/*
 * Copyright 2013 Michael Ossmann
 * Copyright 2013 Benjamin Vernoux
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
#define W25Q80BV_DEVICE_ID    0xAB
#define W25Q80BV_UNIQUE_ID    0x4B

#define W25Q80BV_STATUS_BUSY  0x01


#define W25Q80BV_DEVICE_ID_RES  0x13 /* Expected device_id for W25Q80BV */

typedef union
{
	uint64_t id_64b;
	uint32_t id_32b[2]; /* 2*32bits 64bits Unique ID */
	uint8_t id_8b[8]; /* 8*8bits 64bits Unique ID */
} w25q80bv_unique_id_t;

void w25q80bv_setup(void);
void w25q80bv_chip_erase(void);
void w25q80bv_program(uint32_t addr, uint32_t len, const uint8_t* data);
uint8_t w25q80bv_get_device_id(void);
void w25q80bv_get_unique_id(w25q80bv_unique_id_t* unique_id);

#endif//__W25Q80BV_H__
