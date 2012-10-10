/*
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

#ifndef __FAULT_HANDLER__
#define __FAULT_HANDLER__

#include <stdint.h>

#include <libopencm3/cm3/memorymap.h>

// TODO: Move all this to a Cortex-M(?) include file, since these
// structures are supposedly the same between processors (to an
// undetermined extent).
typedef struct armv7m_scb_t armv7m_scb_t;
struct armv7m_scb_t {
	volatile const uint32_t CPUID;
	volatile uint32_t ICSR;
	volatile uint32_t VTOR;
	volatile uint32_t AIRCR;
	volatile uint32_t SCR;
	volatile uint32_t CCR;
	volatile uint32_t SHPR1;
	volatile uint32_t SHPR2;
	volatile uint32_t SHPR3;
	volatile uint32_t SHCSR;
	volatile uint32_t CFSR;
	volatile uint32_t HFSR;
	volatile uint32_t DFSR;
	volatile uint32_t MMFAR;
	volatile uint32_t BFAR;
	volatile uint32_t AFSR;
	volatile const uint32_t ID_PFR0;
	volatile const uint32_t ID_PFR1;
	volatile const uint32_t ID_DFR0;
	volatile const uint32_t ID_AFR0;
	volatile const uint32_t ID_MMFR0;
	volatile const uint32_t ID_MMFR1;
	volatile const uint32_t ID_MMFR2;
	volatile const uint32_t ID_MMFR3;
	volatile const uint32_t ID_ISAR0;
	volatile const uint32_t ID_ISAR1;
	volatile const uint32_t ID_ISAR2;
	volatile const uint32_t ID_ISAR3;
	volatile const uint32_t ID_ISAR4;
	volatile const uint32_t __reserved_0x74_0x87[5];
	volatile uint32_t CPACR;
} __attribute__((packed));

static armv7m_scb_t* const SCB = (armv7m_scb_t*)SCB_BASE;

#define SCB_HFSR_DEBUGEVT (1 << 31)
#define SCB_HFSR_FORCED (1 << 30)
#define SCB_HFSR_VECTTBL (1 << 1)

#endif//__FAULT_HANDLER__
