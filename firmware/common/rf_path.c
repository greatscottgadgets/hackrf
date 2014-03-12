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

#include "rf_path.h"

#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/scu.h>

#include <hackrf_core.h>

#include <rffc5071.h>
#include <max2837.h>
#include <max5864.h>
#include <sgpio.h>

#if (defined JAWBREAKER || defined HACKRF_ONE)
/*
 * RF switches on Jawbreaker are controlled by General Purpose Outputs (GPO) on
 * the RFFC5072.
 *
 * On HackRF One, the same signals are controlled by GPIO on the LPC.
 * SWITCHCTRL_NO_TX_AMP_PWR and SWITCHCTRL_NO_RX_AMP_PWR are not normally used
 * on HackRF One as the amplifier power is instead controlled only by
 * SWITCHCTRL_AMP_BYPASS.
 */
#define SWITCHCTRL_NO_TX_AMP_PWR (1 << 0) /* GPO1 turn off TX amp power */
#define SWITCHCTRL_AMP_BYPASS    (1 << 1) /* GPO2 bypass amp section */
#define SWITCHCTRL_TX            (1 << 2) /* GPO3 1 for TX mode, 0 for RX mode */
#define SWITCHCTRL_MIX_BYPASS    (1 << 3) /* GPO4 bypass RFFC5072 mixer section */
#define SWITCHCTRL_HP            (1 << 4) /* GPO5 1 for high-pass, 0 for low-pass */
#define SWITCHCTRL_NO_RX_AMP_PWR (1 << 5) /* GPO6 turn off RX amp power */

/*
 GPO6  GPO5  GPO4 GPO3  GPO2  GPO1
!RXAMP  HP  MIXBP  TX  AMPBP !TXAMP  Mix mode   Amp mode
   1    X     1    1     1      1    TX bypass  Bypass
   1    X     1    1     0      0    TX bypass  TX amplified
   1    1     0    1     1      1    TX high    Bypass
   1    1     0    1     0      0    TX high    TX amplified
   1    0     0    1     1      1    TX low     Bypass
   1    0     0    1     0      0    TX low     TX amplified
   1    X     1    0     1      1    RX bypass  Bypass
   0    X     1    0     0      1    RX bypass  RX amplified
   1    1     0    0     1      1    RX high    Bypass
   0    1     0    0     0      1    RX high    RX amplified
   1    0     0    0     1      1    RX low     Bypass
   0    0     0    0     0      1    RX low     RX amplified
*/

/*
 * Safe (initial) switch settings turn off both amplifiers and enable both amp
 * bypass and mixer bypass.
 */
#define SWITCHCTRL_SAFE (SWITCHCTRL_NO_TX_AMP_PWR | SWITCHCTRL_AMP_BYPASS | SWITCHCTRL_TX | SWITCHCTRL_MIX_BYPASS | SWITCHCTRL_HP | SWITCHCTRL_NO_RX_AMP_PWR)
#endif

uint8_t switchctrl = SWITCHCTRL_SAFE;

/*
 * Antenna port power on HackRF One is controlled by GPO1 on the RFFC5072.
 * This is the only thing we use RFFC5072 GPO for on HackRF One.  The value of
 * SWITCHCTRL_NO_ANT_PWR does not correspond to the GPO1 bit in the gpo
 * register.
 */
#define SWITCHCTRL_ANT_PWR (1 << 6) /* turn on antenna port power */

#ifdef HACKRF_ONE
static void switchctrl_set_hackrf_one(uint8_t ctrl) {
	if (ctrl & SWITCHCTRL_TX) {
		gpio_set(PORT_TX, PIN_TX);
		gpio_clear(PORT_RX, PIN_RX);
	} else {
		gpio_clear(PORT_TX, PIN_TX);
		gpio_set(PORT_RX, PIN_RX);
	}

	if (ctrl & SWITCHCTRL_MIX_BYPASS) {
		gpio_set(PORT_MIX_BYPASS, PIN_MIX_BYPASS);
		gpio_clear(PORT_NO_MIX_BYPASS, PIN_NO_MIX_BYPASS);
		if (ctrl & SWITCHCTRL_TX) {
			gpio_set(PORT_TX_MIX_BP, PIN_TX_MIX_BP);
			gpio_clear(PORT_RX_MIX_BP, PIN_RX_MIX_BP);
		} else {
			gpio_clear(PORT_TX_MIX_BP, PIN_TX_MIX_BP);
			gpio_set(PORT_RX_MIX_BP, PIN_RX_MIX_BP);
		}
	} else {
		gpio_clear(PORT_MIX_BYPASS, PIN_MIX_BYPASS);
		gpio_set(PORT_NO_MIX_BYPASS, PIN_NO_MIX_BYPASS);
		gpio_clear(PORT_TX_MIX_BP, PIN_TX_MIX_BP);
		gpio_clear(PORT_RX_MIX_BP, PIN_RX_MIX_BP);
	}

	if (ctrl & SWITCHCTRL_HP) {
		gpio_set(PORT_HP, PIN_HP);
		gpio_clear(PORT_LP, PIN_LP);
	} else {
		gpio_clear(PORT_HP, PIN_HP);
		gpio_set(PORT_LP, PIN_LP);
	}

	if (ctrl & SWITCHCTRL_AMP_BYPASS) {
		gpio_set(PORT_AMP_BYPASS, PIN_AMP_BYPASS);
		gpio_clear(PORT_TX_AMP, PIN_TX_AMP);
		gpio_set(PORT_NO_TX_AMP_PWR, PIN_NO_TX_AMP_PWR);
		gpio_clear(PORT_RX_AMP, PIN_RX_AMP);
		gpio_set(PORT_NO_RX_AMP_PWR, PIN_NO_RX_AMP_PWR);
	} else if (ctrl & SWITCHCTRL_TX) {
		gpio_clear(PORT_AMP_BYPASS, PIN_AMP_BYPASS);
		gpio_set(PORT_TX_AMP, PIN_TX_AMP);
		gpio_clear(PORT_NO_TX_AMP_PWR, PIN_NO_TX_AMP_PWR);
		gpio_clear(PORT_RX_AMP, PIN_RX_AMP);
		gpio_set(PORT_NO_RX_AMP_PWR, PIN_NO_RX_AMP_PWR);
	} else {
		gpio_clear(PORT_AMP_BYPASS, PIN_AMP_BYPASS);
		gpio_clear(PORT_TX_AMP, PIN_TX_AMP);
		gpio_set(PORT_NO_TX_AMP_PWR, PIN_NO_TX_AMP_PWR);
		gpio_set(PORT_RX_AMP, PIN_RX_AMP);
		gpio_clear(PORT_NO_RX_AMP_PWR, PIN_NO_RX_AMP_PWR);
	}

	/*
	 * These normally shouldn't be set post-Jawbreaker, but they can be
	 * used to explicitly turn off power to the amplifiers while AMP_BYPASS
	 * is unset:
	 */
	if (ctrl & SWITCHCTRL_NO_TX_AMP_PWR)
		gpio_set(PORT_NO_TX_AMP_PWR, PIN_NO_TX_AMP_PWR);
	if (ctrl & SWITCHCTRL_NO_RX_AMP_PWR)
		gpio_set(PORT_NO_RX_AMP_PWR, PIN_NO_RX_AMP_PWR);

	if (ctrl & SWITCHCTRL_ANT_PWR) {
		rffc5071_set_gpo(0x00); /* turn on antenna power by clearing GPO1 */
	} else {
		rffc5071_set_gpo(0x01); /* turn off antenna power by setting GPO1 */
	}
}
#endif

static void switchctrl_set(const uint8_t gpo) {
#ifdef JAWBREAKER
	rffc5071_set_gpo(gpo);
#elif HACKRF_ONE
	switchctrl_set_hackrf_one(gpo);
#else
	(void)gpo;
#endif
}

void rf_path_pin_setup() {
#ifdef HACKRF_ONE
	/* Configure RF switch control signals */
	scu_pinmux(SCU_HP,             SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_LP,             SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_TX_MIX_BP,      SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_NO_MIX_BYPASS,  SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_RX_MIX_BP,      SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_TX_AMP,         SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_TX,             SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
	scu_pinmux(SCU_MIX_BYPASS,     SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
	scu_pinmux(SCU_RX,             SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
	scu_pinmux(SCU_NO_TX_AMP_PWR,  SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_AMP_BYPASS,     SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_RX_AMP,         SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_NO_RX_AMP_PWR,  SCU_GPIO_FAST | SCU_CONF_FUNCTION0);

	/* Configure RF power supply (VAA) switch */
	scu_pinmux(SCU_NO_VAA_ENABLE,  SCU_GPIO_FAST | SCU_CONF_FUNCTION0);

	/* Configure RF switch control signals as outputs */
	GPIO0_DIR |= PIN_AMP_BYPASS;
	GPIO1_DIR |= (PIN_NO_MIX_BYPASS | PIN_RX_AMP | PIN_NO_RX_AMP_PWR);
	GPIO2_DIR |= (PIN_HP | PIN_LP | PIN_TX_MIX_BP | PIN_RX_MIX_BP | PIN_TX_AMP);
	GPIO3_DIR |= PIN_NO_TX_AMP_PWR;
	GPIO5_DIR |= (PIN_TX | PIN_MIX_BYPASS | PIN_RX);

	/*
	 * Safe (initial) switch settings turn off both amplifiers and antenna port
	 * power and enable both amp bypass and mixer bypass.
	 */
	switchctrl_set(SWITCHCTRL_AMP_BYPASS | SWITCHCTRL_MIX_BYPASS);

	/* Configure RF power supply (VAA) switch control signal as output */
	GPIO_DIR(PORT_NO_VAA_ENABLE) |= PIN_NO_VAA_ENABLE;

	/* Safe state: start with VAA turned off: */
	disable_rf_power();
#endif
}

void rf_path_init(void) {
	ssp1_set_mode_max5864();
	max5864_shutdown();
	
	ssp1_set_mode_max2837();
	max2837_setup();
	max2837_start();
	
	rffc5071_setup();
	switchctrl_set(switchctrl);
}

void rf_path_set_direction(const rf_path_direction_t direction) {
	/* Turn off TX and RX amplifiers, then enable based on direction and bypass state. */
	switchctrl |= SWITCHCTRL_NO_TX_AMP_PWR | SWITCHCTRL_NO_RX_AMP_PWR;
	switch(direction) {
	case RF_PATH_DIRECTION_TX:
		switchctrl |= SWITCHCTRL_TX;
		if( (switchctrl & SWITCHCTRL_AMP_BYPASS) == 0 ) {
			/* TX amplifier is in path, be sure to enable TX amplifier. */
			switchctrl &= ~SWITCHCTRL_NO_TX_AMP_PWR;
		}
		rffc5071_tx();
		if( switchctrl & SWITCHCTRL_MIX_BYPASS ) {
			rffc5071_disable();
		} else {
			rffc5071_enable();
		}
		ssp1_set_mode_max5864();
		max5864_tx();
		ssp1_set_mode_max2837();
		max2837_tx();
		sgpio_configure(SGPIO_DIRECTION_TX);
		break;
	
	case RF_PATH_DIRECTION_RX:
		switchctrl &= ~SWITCHCTRL_TX;
		if( (switchctrl & SWITCHCTRL_AMP_BYPASS) == 0 ) {
			/* RX amplifier is in path, be sure to enable RX amplifier. */
			switchctrl &= ~SWITCHCTRL_NO_RX_AMP_PWR;
		}
		rffc5071_rx();
		if( switchctrl & SWITCHCTRL_MIX_BYPASS ) {
			rffc5071_disable();
		} else {
			rffc5071_enable();
		}
		ssp1_set_mode_max5864();
		max5864_rx();
		ssp1_set_mode_max2837();
		max2837_rx();
		sgpio_configure(SGPIO_DIRECTION_RX);
		break;
		
	case RF_PATH_DIRECTION_OFF:
	default:
#ifdef HACKRF_ONE
		rf_path_set_antenna(0);
#endif
		/* Set RF path to receive direction when "off" */
		switchctrl &= ~SWITCHCTRL_TX;
		rffc5071_disable();
		ssp1_set_mode_max5864();
		max5864_standby();
		ssp1_set_mode_max2837();
		max2837_set_mode(MAX2837_MODE_STANDBY);
		sgpio_configure(SGPIO_DIRECTION_RX);
		break;
	}

	switchctrl_set(switchctrl);
}

void rf_path_set_filter(const rf_path_filter_t filter) {
	switch(filter) {
	default:
	case RF_PATH_FILTER_BYPASS:
		switchctrl |= SWITCHCTRL_MIX_BYPASS;
		rffc5071_disable();
		break;
		
	case RF_PATH_FILTER_LOW_PASS:
		switchctrl &= ~(SWITCHCTRL_HP | SWITCHCTRL_MIX_BYPASS);
		rffc5071_enable();
		break;
		
	case RF_PATH_FILTER_HIGH_PASS:
		switchctrl &= ~SWITCHCTRL_MIX_BYPASS;
		switchctrl |= SWITCHCTRL_HP;
		rffc5071_enable();
		break;
	}

	switchctrl_set(switchctrl);
}

void rf_path_set_lna(const uint_fast8_t enable) {
	if( enable ) {
		if( switchctrl & SWITCHCTRL_TX ) {
			/* AMP_BYPASS=0, NO_RX_AMP_PWR=1, NO_TX_AMP_PWR=0 */
			switchctrl |= SWITCHCTRL_NO_RX_AMP_PWR;
			switchctrl &= ~(SWITCHCTRL_AMP_BYPASS | SWITCHCTRL_NO_TX_AMP_PWR);
		} else {
			/* AMP_BYPASS=0, NO_RX_AMP_PWR=0, NO_TX_AMP_PWR=1 */
			switchctrl |= SWITCHCTRL_NO_TX_AMP_PWR;
			switchctrl &= ~(SWITCHCTRL_AMP_BYPASS | SWITCHCTRL_NO_RX_AMP_PWR);
		}
	} else {
		/* AMP_BYPASS=1, NO_RX_AMP_PWR=1, NO_TX_AMP_PWR=1 */
		switchctrl |= SWITCHCTRL_AMP_BYPASS | SWITCHCTRL_NO_TX_AMP_PWR | SWITCHCTRL_NO_RX_AMP_PWR;
	}
	
	switchctrl_set(switchctrl);
}

/* antenna port power control */
void rf_path_set_antenna(const uint_fast8_t enable) {
	if (enable) {
		switchctrl |= SWITCHCTRL_ANT_PWR;
	} else {
		switchctrl &= ~(SWITCHCTRL_ANT_PWR);
	}

	switchctrl_set(switchctrl);
}
