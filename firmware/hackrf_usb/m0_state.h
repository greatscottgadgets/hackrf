/*
 * Copyright 2022 Great Scott Gadgets
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
#include <usb_request.h>

struct m0_state {
	uint32_t mode;
	uint32_t m0_count;
	uint32_t m4_count;
};

enum m0_mode {
	M0_MODE_IDLE = 0,
	M0_MODE_RX = 1,
	M0_MODE_TX = 2,
};

/* Address of m0_state is set in ldscripts. If you change the name of this
 * variable, it won't be where it needs to be in the processor's address space,
 * unless you also adjust the ldscripts.
 */
extern volatile struct m0_state m0_state;

usb_request_status_t usb_vendor_request_get_m0_state(
	usb_endpoint_t* const endpoint,	const usb_transfer_stage_t stage);

#endif/*__M0_STATE_H__*/
