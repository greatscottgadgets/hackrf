/*
 * Copyright 2024 Great Scott Gadgets <info@greatscottgadgets.com>
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

#ifndef __LOCKING_H__
#define __LOCKING_H__

#include <stdint.h>

#include <libopencm3/cm3/sync.h>

// Use ldrex and strex directly if available.
// Otherwise, disable interrupts to ensure exclusivity.
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
	#define load_exclusive  __ldrex
	#define store_exclusive __strex
#else
static inline uint32_t load_exclusive(volatile uint32_t* addr)
{
	__disable_irq();
	return *addr;
}

static inline uint32_t store_exclusive(uint32_t val, volatile uint32_t* addr)
{
	*addr = val;
	__enable_irq();
	return 0;
}
#endif

#endif /* __LOCKING_H__ */
