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

#include "u128.h"

/* From https://stackoverflow.com/a/58381061 */

u128 u128_multiply(uint64_t lhs, uint64_t rhs)
{
	uint64_t lo_lo = (lhs & 0xFFFFFFFF) * (rhs & 0xFFFFFFFF);
	uint64_t hi_lo = (lhs >> 32) * (rhs & 0xFFFFFFFF);
	uint64_t lo_hi = (lhs & 0xFFFFFFFF) * (rhs >> 32);
	uint64_t hi_hi = (lhs >> 32) * (rhs >> 32);
	uint64_t cross = ((lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + lo_hi);
	return (u128){
		.hi = ((hi_lo >> 32) + (cross >> 32) + hi_hi),
		.lo = ((cross << 32) | (lo_lo & 0xFFFFFFFF)),
	};
}

/* From https://stackoverflow.com/a/70052545 */

/* clang-format off */

#define SUBCcc(a,b,cy,t0,t1,t2) \
  (t0=(b)+cy, t1=(a), cy=t0<cy, t2=t1<t0, cy=cy+t2, t1-t0)

#define SUBcc(a,b,cy,t0,t1) \
  (t0=(b), t1=(a), cy=t1<t0, t1-t0)

#define SUBC(a,b,cy,t0,t1) \
  (t0=(b)+cy, t1=(a), t1-t0)

#define ADDCcc(a,b,cy,t0,t1) \
  (t0=(b)+cy, t1=(a), cy=t0<cy, t0=t0+t1, t1=t0<t1, cy=cy+t1, t0=t0)

#define ADDcc(a,b,cy,t0,t1) \
  (t0=(b), t1=(a), t0=t0+t1, cy=t0<t1, t0=t0)

#define ADDC(a,b,cy,t0,t1) \
  (t0=(b)+cy, t1=(a), t0+t1)

/* clang-format on */

u128 u128_divide(u128 dvnd, u128 dvsr)
{
	u128 quot, rem, tmp;
	uint64_t cy, t0, t1, t2;

	quot.hi = dvnd.hi;
	quot.lo = dvnd.lo;
	rem.hi = 0;
	rem.lo = 0;

	for (int i = 0; i < 128; i++) {
		quot.lo = ADDcc(quot.lo, quot.lo, cy, t0, t1);
		quot.hi = ADDCcc(quot.hi, quot.hi, cy, t0, t1);
		rem.lo = ADDCcc(rem.lo, rem.lo, cy, t0, t1);
		rem.hi = ADDC(rem.hi, rem.hi, cy, t0, t1);
		tmp.lo = SUBcc(rem.lo, dvsr.lo, cy, t0, t1);
		tmp.hi = SUBCcc(rem.hi, dvsr.hi, cy, t0, t1, t2);
		if (!cy) { // remainder >= divisor
			rem.lo = tmp.lo;
			rem.hi = tmp.hi;
			quot.lo |= 1;
		}
	}

	return quot;
}
