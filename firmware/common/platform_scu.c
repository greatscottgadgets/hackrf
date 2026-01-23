/*
 * Copyright 2012-2026 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include <stddef.h>

#include "platform_scu.h"

// clang-format off
const platform_scu_t* platform_scu()
{
	static const platform_scu_t* _platform_scu = NULL;
	if (_platform_scu != NULL) {
		return _platform_scu;
	}

	board_id_t board_id = detected_platform();
	board_rev_t board_rev = detected_revision();
	static platform_scu_t scu;
	(void) board_rev; // TODO silence warning until we use this

	/* MAX283x GPIO (XCVR_CTL) PinMux */
	switch (board_id) {
	case BOARD_ID_RAD1O:
		scu.XCVR_RXHP = P8_1; /* GPIO[] on P8_1 */
		scu.XCVR_B6   = P8_2; /* GPIO[] on P8_2 */
		scu.XCVR_B7   = P9_3; /* GPIO[] on P9_3 */
		break;
	case BOARD_ID_PRALINE:
		scu.XCVR_ENABLE   = PE_1;  /* GPIO7[1]  on PE_1 */
		scu.XCVR_RXENABLE = PE_2;  /* GPIO7[2]  on PE_2 */
		scu.XCVR_CS       = PD_14; /* GPIO6[28] on PD_14 */
		scu.XCVR_RXHP     = PD_15; /* GPIO6[29] on PD_15 */
		scu.XCVR_LD       = P9_6;  /* GPIO4[11] on P9_6 */

		scu.XCVR_ENABLE_PINCFG   = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.XCVR_RXENABLE_PINCFG = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.XCVR_CS_PINCFG       = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.XCVR_RXHP_PINCFG     = (SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
		scu.XCVR_LD_PINCFG       = (SCU_GPIO_FAST | SCU_CONF_FUNCTION0 |
									SCU_CONF_EPD_EN_PULLDOWN | SCU_CONF_EPUN_DIS_PULLUP);
		break;
	case BOARD_ID_JELLYBEAN:
	case BOARD_ID_JAWBREAKER:
	case BOARD_ID_HACKRF1_OG:
	case BOARD_ID_HACKRF1_R9:
		scu.XCVR_ENABLE   = P4_6;  /* GPIO2[6]  on P4_6 */
		scu.XCVR_RXENABLE = P4_5;  /* GPIO2[5]  on P4_5 */
		scu.XCVR_TXENABLE = P4_4;  /* GPIO2[4]  on P4_4 */
		scu.XCVR_CS       = P1_20; /* GPIO0[15] on P1_20 */

		scu.XCVR_ENABLE_PINCFG   = (SCU_GPIO_FAST);
		scu.XCVR_RXENABLE_PINCFG = (SCU_GPIO_FAST);
		scu.XCVR_TXENABLE_PINCFG = (SCU_GPIO_FAST);
		scu.XCVR_CS_PINCFG       = (SCU_GPIO_FAST);
		break;
	default:
		// TODO handle UNRECOGNIZED & UNDETECTED
		break;
	}

	_platform_scu = &scu;

	return _platform_scu;
}

// clang-format on
