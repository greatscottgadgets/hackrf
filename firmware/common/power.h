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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef IS_PRALINE
void enable_1v2_power(void);
void disable_1v2_power(void);
void enable_3v3aux_power(void);
void disable_3v3aux_power(void);
#endif
void enable_1v8_power(void);
void disable_1v8_power(void);

#if defined(IS_RAD1O) || defined(IS_HACKRF_ONE) || defined(IS_PRALINE)
void enable_rf_power(void);
void disable_rf_power(void);
#endif

#ifdef __cplusplus
}
#endif
