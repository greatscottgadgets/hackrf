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

#ifndef __FIXED_POINT_H__
#define __FIXED_POINT_H__

/* 40.24 fixed-point */
typedef uint64_t fp_40_24_t;

/* one million in 40.24 fixed-point */
#define FP_ONE_MHZ ((1000ULL * 1000ULL) << 24)

/* one in 40.24 fixed-point */
#define FP_ONE_HZ (1 << 24)

#endif /*__FIXED_POINT_H__*/
