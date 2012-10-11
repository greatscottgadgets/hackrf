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

#ifndef __SGPIO_H__
#define __SGPIO_H__

#include <hackrf_core.h>

void sgpio_configure_pin_functions();
void sgpio_test_interface();
void sgpio_configure_for_tx();
void sgpio_configure_for_rx();
void sgpio_configure_deep(const transceiver_mode_t transceiver_mode);
void sgpio_cpld_stream_enable();
void sgpio_cpld_stream_disable();
bool sgpio_cpld_stream_is_enabled();

#endif//__SGPIO_H__
