/*
 * Copyright 2026 Great Scott Gadgets <info@greatscottgadgets.com>
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

#ifndef __LZ4_BUF_H
#define __LZ4_BUF_H

#include <stdint.h>

#define LZ4_BUFFER_SIZE 0x1000

/* Addresses of these buffers are set in linker script. If you change the name
 * of these buffers, you must also adjust the linker script. */

extern uint8_t lz4_in_buf[LZ4_BUFFER_SIZE];
extern uint8_t lz4_out_buf[LZ4_BUFFER_SIZE];

#endif /*__LZ4_BUF_H */
