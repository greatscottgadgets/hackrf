/*
 * Copyright 2025 Great Scott Gadgets <info@greatscottgadgets.com>
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

#ifndef __M0_STATE_H__
#define __M0_STATE_H__

#include <stdint.h>

#define M0_REQUEST_FLAG (1 << 16)

struct m0_state {
	uint32_t requested_mode;
	uint32_t active_mode;
	uint32_t m0_count;
	uint32_t m4_count;
	uint32_t num_shortfalls;
	uint32_t longest_shortfall;
	uint32_t shortfall_limit;
	uint32_t threshold;
	uint32_t next_mode;
	uint32_t error;
};

enum m0_mode {
	M0_MODE_IDLE = 0,
	M0_MODE_WAIT = 1,
	M0_MODE_RX = 2,
	M0_MODE_TX_START = 3,
	M0_MODE_TX_RUN = 4,
};

enum m0_error {
	M0_ERROR_NONE = 0,
	M0_ERROR_RX_TIMEOUT = 1,
	M0_ERROR_TX_TIMEOUT = 2,
};

/* Address of m0_state is set in ldscripts. If you change the name of this
 * variable, it won't be where it needs to be in the processor's address space,
 * unless you also adjust the ldscripts.
 */
extern volatile struct m0_state m0_state;

void m0_set_mode(enum m0_mode mode);

#endif /*__M0_STATE_H__*/
