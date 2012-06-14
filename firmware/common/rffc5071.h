/*
 * Copyright 2012 Michael Ossmann
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

#include <stdint.h>

/* register names */
#define RFFC5071_LF       0x00
#define RFFC5071_XO       0x01
#define RFFC5071_CAL_TIME 0x02
#define RFFC5071_VCO_CTRL 0x03
#define RFFC5071_CT_CAL1  0x04
#define RFFC5071_CT_CAL2  0x05
#define RFFC5071_PLL_CAL1 0x06
#define RFFC5071_PLL_CAL2 0x07
#define RFFC5071_VCO_AUTO 0x08
#define RFFC5071_PLL_CTRL 0x09
#define RFFC5071_PLL_BIAS 0x0A
#define RFFC5071_MIX_CONT 0x0B
#define RFFC5071_P1_FREQ1 0x0C
#define RFFC5071_P1_FREQ2 0x0D
#define RFFC5071_P1_FREQ3 0x0E
#define RFFC5071_P2_FREQ1 0x0F
#define RFFC5071_P2_FREQ2 0x10
#define RFFC5071_P2_FREQ3 0x11
#define RFFC5071_FN_CTRL  0x12
#define RFFC5071_EXT_MOD  0x13
#define RFFC5071_FMOD     0x14
#define RFFC5071_SDI_CTRL 0x15
#define RFFC5071_GPO      0x16
#define RFFC5071_T_VCO    0x17
#define RFFC5071_IQMOD1   0x18
#define RFFC5071_IQMOD2   0x19
#define RFFC5071_IQMOD3   0x1A
#define RFFC5071_IQMOD4   0x1B
#define RFFC5071_T_CTRL   0x1C
#define RFFC5071_DEV_CTRL 0x1D
#define RFFC5071_TEST     0x1E
#define RFFC5071_READBACK 0x1F

void rffc5071_init(void);
void rffc5071_reg_write(uint8_t reg, uint16_t val);
uint16_t rffc5071_reg_read(uint8_t reg);
