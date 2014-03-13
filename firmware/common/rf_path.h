/*
 * Copyright 2012 Jared Boone
 * Copyright 2013 Benjamin Vernoux
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

#ifndef __RFPATH_H__
#define __RFPATH_H__

#include <stdint.h>

void rf_path_pin_setup(void);
void rf_path_init(void);

typedef enum {
	RF_PATH_DIRECTION_OFF,
	RF_PATH_DIRECTION_RX,
	RF_PATH_DIRECTION_TX,
} rf_path_direction_t;

void rf_path_set_direction(const rf_path_direction_t direction);

typedef enum {
	RF_PATH_FILTER_BYPASS = 0,
	RF_PATH_FILTER_LOW_PASS = 1,
	RF_PATH_FILTER_HIGH_PASS = 2,
} rf_path_filter_t;

void rf_path_set_filter(const rf_path_filter_t filter);

void rf_path_set_lna(const uint_fast8_t enable);
void rf_path_set_antenna(const uint_fast8_t enable);

#endif/*__RFPATH_H__*/
