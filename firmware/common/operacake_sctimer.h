/*
 * Copyright 2016 Dominic Spill <dominicgs@gmail.com>
 * Copyright 2018 Schuyler St. Leger
 * Copyright 2021 Great Scott Gadgets
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

#ifndef __OPERACAKE_SCTIMER_H
#define __OPERACAKE_SCTIMER_H

#include <stdbool.h>
#include <stdint.h>

struct operacake_dwell_times {
	uint32_t dwell;
	uint8_t port;
};

void operacake_sctimer_init();
void operacake_sctimer_enable(bool enable);
void operacake_sctimer_set_dwell_times(struct operacake_dwell_times* times, int n);
void operacake_sctimer_stop();
void operacake_sctimer_reset_state();

#endif /* __OPERACAKE_SCTIMER_H */
