/*
 * Copyright 2012-2022 Great Scott Gadgets <info@greatscottgadgets.com>
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

#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "rf_path.h"

typedef enum {
	RF_DIRECTION_USER_OPT_NOP,	// No OPeration / Ignore the thing
	RF_DIRECTION_USER_OPT_RESERVED, // Currently a NOP
	RF_DIRECTION_USER_OPT_CLEAR,	// Clear/Disable the thing
	RF_DIRECTION_USER_OPT_SET,	// Set/Enable the thing
} user_config_user_opt_t;

void user_config_set_bias_t_opt(const rf_path_direction_t direction, const user_config_user_opt_t action);
void user_config_set_bias_t_opts(uint16_t value);

void user_config_on_rf_path_direction_change(rf_path_t* const rf_path, const rf_path_direction_t direction);

#endif