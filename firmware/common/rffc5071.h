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

#ifndef __RFFC5071_H
#define __RFFC5071_H

/* 31 registers, each containing 16 bits of data. */
#define RFFC5071_NUM_REGS 31

extern uint16_t rffc5071_regs[RFFC5071_NUM_REGS];
extern uint32_t rffc5071_regs_dirty;

#define RFFC5071_REG_SET_CLEAN(r) rffc5071_regs_dirty &= ~(1UL<<r)
#define RFFC5071_REG_SET_DIRTY(r) rffc5071_regs_dirty |= (1UL<<r)

#ifdef JAWBREAKER
/*
 * RF switches on Jawbreaker are controlled by General Purpose Outputs (GPO) on
 * the RFFC5072.
 */
#define SWITCHCTRL_NO_TX_AMP_PWR (1 << 0) /* GPO1 turn off TX amp power */
#define SWITCHCTRL_AMP_BYPASS    (1 << 1) /* GPO2 bypass amp section */
#define SWITCHCTRL_TX            (1 << 2) /* GPO3 1 for TX mode, 0 for RX mode */
#define SWITCHCTRL_MIX_BYPASS    (1 << 3) /* GPO4 bypass RFFC5072 mixer section */
#define SWITCHCTRL_HP            (1 << 4) /* GPO5 1 for high-pass, 0 for low-pass */
#define SWITCHCTRL_NO_RX_AMP_PWR (1 << 5) /* GPO6 turn off RX amp power */

/*
 * Safe (initial) switch settings turn off both amplifiers and enable both amp
 * bypass and mixer bypass.
 */
#define SWITCHCTRL_SAFE (SWITCHCTRL_NO_TX_AMP_PWR | SWITCHCTRL_AMP_BYPASS | SWITCHCTRL_TX | SWITCHCTRL_MIX_BYPASS | SWITCHCTRL_HP | SWITCHCTRL_NO_RX_AMP_PWR)
#endif

/* Initialize chip. Call _setup() externally, as it calls _init(). */
extern void rffc5071_init(void);
extern void rffc5071_setup(void);

/* Read a register via SPI. Save a copy to memory and return
 * value. Discard any uncommited changes and mark CLEAN. */
extern uint16_t rffc5071_reg_read(uint8_t r);

/* Write value to register via SPI and save a copy to memory. Mark
 * CLEAN. */
extern void rffc5071_reg_write(uint8_t r, uint16_t v);

/* Write all dirty registers via SPI from memory. Mark all clean. Some
 * operations require registers to be written in a certain order. Use
 * provided routines for those operations. */
extern void rffc5071_regs_commit(void);

/* Set frequency (MHz). */
extern uint32_t rffc5071_set_frequency(uint16_t mhz);

/* Set up rx only, tx only, or full duplex. Chip should be disabled
 * before _tx, _rx, or _rxtx are called. */
extern void rffc5071_tx(uint8_t);
extern void rffc5071_rx(uint8_t);
extern void rffc5071_rxtx(void);
extern void rffc5071_enable(void);
extern void rffc5071_disable(void);

extern void rffc5071_set_gpo(uint8_t);

#endif // __RFFC5071_H
