/*
 * Copyright 2023 Jonathan Suite (GitHub: @ai6aj)
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

#include "user_config.h"

static user_config_user_opt_t user_direction_rx_bias_t_opts = RF_DIRECTION_USER_OPT_NOP;
static user_config_user_opt_t user_direction_tx_bias_t_opts = RF_DIRECTION_USER_OPT_NOP;
static user_config_user_opt_t user_direction_off_bias_t_opts =
	RF_DIRECTION_USER_OPT_CLEAR;

//	Perform user-specified actions to Bias T power when transitioning modes
static void _rf_path_handle_user_bias_t_action(rf_path_t* const rf_path, int action)
{
	switch (action) {
	case RF_DIRECTION_USER_OPT_SET:
		rf_path_set_antenna(rf_path, 1);
		break;

	case RF_DIRECTION_USER_OPT_CLEAR:
		rf_path_set_antenna(rf_path, 0);
		break;

	case RF_DIRECTION_USER_OPT_NOP:
	default:
		break;
	}
}

void user_config_on_rf_path_direction_change(
	rf_path_t* const rf_path,
	const rf_path_direction_t direction)
{
	switch (direction) {
	case RF_PATH_DIRECTION_RX:
		_rf_path_handle_user_bias_t_action(rf_path, user_direction_rx_bias_t_opts);
		break;

	case RF_PATH_DIRECTION_TX:
		_rf_path_handle_user_bias_t_action(rf_path, user_direction_tx_bias_t_opts);
		break;

	case RF_PATH_DIRECTION_OFF:
	default:
		_rf_path_handle_user_bias_t_action(
			rf_path,
			user_direction_off_bias_t_opts);
		break;
	}
}

void user_config_set_bias_t_opt(
	const rf_path_direction_t direction,
	const user_config_user_opt_t option)
{
	switch (direction) {
	case RF_PATH_DIRECTION_RX:
		user_direction_rx_bias_t_opts = option;
		break;

	case RF_PATH_DIRECTION_TX:
		user_direction_tx_bias_t_opts = option;
		break;

	case RF_PATH_DIRECTION_OFF:
		user_direction_off_bias_t_opts = option;
		break;

	default:
		break;
	}
}

/*
 Bias T options are set as follows:
	Bits 0,1:	One of NOP (0), CLEAR (0b10), or SET (0b11)
	Bit 2:		1=Set OFF behavior according to bits 0,1  0=Don't change
	Bits 3,4:	One of NOP (0), CLEAR (0b10), or SET (0b11)
	Bit 5:		1=Set RX behavior according to bits 0,1  0=Don't change
	Bits 6,7:	One of NOP (0), CLEAR (0b10), or SET (0b11)
	Bit 8:		1=Set TX behavior according to bits 0,1  0=Don't change
	Bits 9-15:	Ignored; set to 0
*/
void user_config_set_bias_t_opts(uint16_t value)
{
	if (value & 0x4) {
		user_config_set_bias_t_opt(RF_PATH_DIRECTION_OFF, value & 0x3);
	}
	if (value & 0x20) {
		user_config_set_bias_t_opt(RF_PATH_DIRECTION_RX, (value & 0x18) >> 3);
	}
	if (value & 0x100) {
		user_config_set_bias_t_opt(RF_PATH_DIRECTION_TX, (value & 0xC0) >> 6);
	}
}
