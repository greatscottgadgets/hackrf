#include "rf_path.h"

#include <rffc5071.h>

#ifdef JAWBREAKER
/*
 * RF switches on Jawbreaker are controlled by General Purpose Outputs (GPO) on
 * the RFFC5072.
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

void rf_path_init(void) {
	rffc5071_set_gpo(switchctrl);
}

void rf_path_set_direction(const rf_path_direction_t direction) {
	/* Turn off TX and RX amplifiers, then enable based on direction and bypass state. */
	switchctrl |= SWITCHCTRL_NO_TX_AMP_PWR | SWITCHCTRL_NO_RX_AMP_PWR;
	if( direction == RF_PATH_DIRECTION_TX ) {
		switchctrl |= SWITCHCTRL_TX;
		if( (switchctrl & SWITCHCTRL_AMP_BYPASS) == 0 ) {
			/* TX amplifier is in path, be sure to enable TX amplifier. */
			switchctrl &= ~SWITCHCTRL_NO_TX_AMP_PWR;
		}
		rffc5071_tx();
	} else {
		switchctrl &= ~SWITCHCTRL_TX;
		if( (switchctrl & SWITCHCTRL_AMP_BYPASS) == 0 ) {
			/* RX amplifier is in path, be sure to enable RX amplifier. */
			switchctrl &= ~SWITCHCTRL_NO_RX_AMP_PWR;
		}
		rffc5071_rx();
	}
	
	rffc5071_set_gpo(switchctrl);
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
	
	rffc5071_set_gpo(switchctrl);
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
	
	rffc5071_set_gpo(switchctrl);
}
