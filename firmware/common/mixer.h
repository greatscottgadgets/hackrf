/*
 * Copyright 2012 Michael Ossmann
 * Copyright 2014 Jared Boone <jared@sharebrained.com>
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

#ifndef __MIXER_H
#define __MIXER_H

#include <stdint.h>

/* Initialize chip. Call _setup() externally, as it calls _init(). */
extern void mixer_init(void);
extern void mixer_setup(void);

/* Set frequency (MHz). */
extern uint64_t mixer_set_frequency(uint16_t mhz);

/* Set up rx only, tx only, or full duplex. Chip should be disabled
 * before _tx, _rx, or _rxtx are called. */
extern void mixer_tx(void);
extern void mixer_rx(void);
extern void mixer_rxtx(void);
extern void mixer_enable(void);
extern void mixer_disable(void);

extern void mixer_set_gpo(uint8_t);

#endif // __MIXER_H
