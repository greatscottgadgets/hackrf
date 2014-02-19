/*
 * 'gcc -DTEST -DDEBUG -O2 -o test max2837.c' prints out what test
 * program would do if it had a real spi library
 *
 * 'gcc -DTEST -DBUS_PIRATE -O2 -o test max2837.c' prints out bus
 * pirate commands to do the same thing.
 */

#include <stdint.h>
#include <string.h>
#include "max2837.h"
#include "max2837_regs.def" // private register def macros

#if (defined DEBUG || defined BUS_PIRATE)
#include <stdio.h>
#define LOG printf
#else
#define LOG(x,...)
#include <libopencm3/lpc43xx/ssp.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/gpio.h>
#include "hackrf_core.h"
#endif

/* Default register values. */
static uint16_t max2837_regs_default[MAX2837_NUM_REGS] = { 
	0x150,   /* 0 */
	0x002,   /* 1 */
	0x1f4,   /* 2 */
	0x1b9,   /* 3 */
	0x00a,   /* 4 */
	0x080,   /* 5 */
	0x006,   /* 6 */
	0x000,   /* 7 */
	0x080,   /* 8 */
	0x018,   /* 9 */
	0x058,   /* 10 */
	0x016,   /* 11 */
	0x24f,   /* 12 */
	0x150,   /* 13 */
	0x1c5,   /* 14 */
	0x081,   /* 15 */
	0x01c,   /* 16 */
	0x155,   /* 17 */
	0x155,   /* 18 */
	0x153,   /* 19 */
	0x241,   /* 20 */
	/*
	 * Charge Pump Common Mode Enable bit (0) of register 21 must be set or TX
	 * does not work.  Page 1 of the SPI doc says not to set it (0x02c), but
	 * page 21 says it should be set by default (0x02d).
	 */
	0x02d,   /* 21 */
	0x1a9,   /* 22 */
	0x24f,   /* 23 */
	0x180,   /* 24 */
	0x100,   /* 25 */
	0x3ca,   /* 26 */
	0x3e3,   /* 27 */
	0x0c0,   /* 28 */
	0x3f0,   /* 29 */
	0x080,   /* 30 */
	0x000 }; /* 31 */

uint16_t max2837_regs[MAX2837_NUM_REGS];

/* Mark all regsisters dirty so all will be written at init. */
uint32_t max2837_regs_dirty = 0xffffffff;

/* Set up all registers according to defaults specified in docs. */
void max2837_init(void)
{
	LOG("# max2837_init\n");
	memcpy(max2837_regs, max2837_regs_default, sizeof(max2837_regs));
	max2837_regs_dirty = 0xffffffff;

	/* Write default register values to chip. */
	max2837_regs_commit();
}

/*
 * Set up pins for GPIO and SPI control, configure SSP peripheral for SPI, and
 * set our own default register configuration.
 */
void max2837_setup(void)
{
	LOG("# max2837_setup\n");
#if !defined TEST
	/* Configure XCVR_CTL GPIO pins. */
#ifdef JELLYBEAN
	scu_pinmux(SCU_XCVR_RXHP, SCU_GPIO_FAST);
	scu_pinmux(SCU_XCVR_B1, SCU_GPIO_FAST);
	scu_pinmux(SCU_XCVR_B2, SCU_GPIO_FAST);
	scu_pinmux(SCU_XCVR_B3, SCU_GPIO_FAST);
	scu_pinmux(SCU_XCVR_B4, SCU_GPIO_FAST);
	scu_pinmux(SCU_XCVR_B5, SCU_GPIO_FAST);
	scu_pinmux(SCU_XCVR_B6, SCU_GPIO_FAST);
	scu_pinmux(SCU_XCVR_B7, SCU_GPIO_FAST);
#endif
	scu_pinmux(SCU_XCVR_ENABLE, SCU_GPIO_FAST);
	scu_pinmux(SCU_XCVR_RXENABLE, SCU_GPIO_FAST);
	scu_pinmux(SCU_XCVR_TXENABLE, SCU_GPIO_FAST);

	/* Set GPIO pins as outputs. */
	GPIO2_DIR |= (PIN_XCVR_ENABLE | PIN_XCVR_RXENABLE | PIN_XCVR_TXENABLE);
#ifdef JELLYBEAN
	GPIO_DIR(PORT_XCVR_RXHP) |= PIN_XCVR_RXHP;
	GPIO_DIR(PORT_XCVR_B) |=
		  PIN_XCVR_B1
		| PIN_XCVR_B2
		| PIN_XCVR_B3
		| PIN_XCVR_B4
		| PIN_XCVR_B5
		| PIN_XCVR_B6
		| PIN_XCVR_B7
		;
#endif

	max2837_mode_shutdown();
#ifdef JELLYBEAN
	gpio_set(PORT_XCVR_RXHP, PIN_XCVR_RXHP);
	gpio_set(PORT_XCVR_B,
		  PIN_XCVR_B1
		| PIN_XCVR_B2
		| PIN_XCVR_B3
		| PIN_XCVR_B4
		| PIN_XCVR_B5
		| PIN_XCVR_B6
		| PIN_XCVR_B7
	);
#endif
#endif

	max2837_init();
	LOG("# max2837_init done\n");

	/* Use SPI control instead of B1-B7 pins for gain settings. */
	set_MAX2837_TXVGA_GAIN_SPI_EN(1);
	set_MAX2837_TXVGA_GAIN_MSB_SPI_EN(1);
	//set_MAX2837_TXVGA_GAIN(0x3f); /* maximum attenuation */
	set_MAX2837_TXVGA_GAIN(0x00); /* minimum attenuation */
	set_MAX2837_VGAMUX_enable(1);
	set_MAX2837_VGA_EN(1);
	set_MAX2837_HPC_RXGAIN_EN(0);
	set_MAX2837_HPC_STOP(MAX2837_STOP_1K);
	set_MAX2837_LNAgain_SPI_EN(1);
	set_MAX2837_LNAgain(MAX2837_LNAgain_MAX); /* maximum gain */
	set_MAX2837_VGAgain_SPI_EN(1);
	set_MAX2837_VGA(0x18); /* reasonable gain for noisy 2.4GHz environment */

	/* maximum rx output common-mode voltage */
	set_MAX2837_BUFF_VCM(MAX2837_BUFF_VCM_1_25);

	/* configure baseband filter for 8 MHz TX */
	set_MAX2837_LPF_EN(1);
	set_MAX2837_ModeCtrl(MAX2837_ModeCtrl_RxLPF);
	set_MAX2837_FT(MAX2837_FT_5M);

	max2837_regs_commit();
}

/* SPI register read. */
uint16_t max2837_spi_read(uint8_t r) {
	gpio_clear(PORT_XCVR_CS, PIN_XCVR_CS);
	const uint16_t value = ssp_transfer(SSP1_NUM, (uint16_t)((1 << 15) | (r << 10)));
	gpio_set(PORT_XCVR_CS, PIN_XCVR_CS);
	return value & 0x3ff;
}

/* SPI register write */
void max2837_spi_write(uint8_t r, uint16_t v) {

#ifdef BUS_PIRATE
	LOG("{0x%02x 0x%02x]\n", 0x00 | ((uint16_t)r<<2) | ((v>>8) & 0x3),
	    v & 0xff);
#elif DEBUG
	LOG("0x%03x -> reg%d\n", v, r);
#else
	gpio_clear(PORT_XCVR_CS, PIN_XCVR_CS);
	ssp_transfer(SSP1_NUM, (uint16_t)((r << 10) | (v & 0x3ff)));
	gpio_set(PORT_XCVR_CS, PIN_XCVR_CS);
#endif
}

uint16_t max2837_reg_read(uint8_t r)
{
	if ((max2837_regs_dirty >> r) & 0x1) {
		max2837_spi_read(r);
	};
	return max2837_regs[r];
}

void max2837_reg_write(uint8_t r, uint16_t v)
{
	max2837_regs[r] = v;
	max2837_spi_write(r, v);
	MAX2837_REG_SET_CLEAN(r);
}

/* This functions should not be needed, and might be confusing. DELETE. */
void max2837_regs_read(void)
{
	;
}

static inline void max2837_reg_commit(uint8_t r)
{
	max2837_reg_write(r,max2837_regs[r]);
}

void max2837_regs_commit(void)
{
	int r;
	for(r = 0; r < MAX2837_NUM_REGS; r++) {
		if ((max2837_regs_dirty >> r) & 0x1) {
			max2837_reg_commit(r);
		}
	}
}

void max2837_mode_shutdown(void) {
	/* All circuit blocks are powered down, except the 4-wire serial bus
	 * and its internal programmable registers.
	 */
	gpio_clear(PORT_XCVR_ENABLE,
			(PIN_XCVR_ENABLE | PIN_XCVR_RXENABLE | PIN_XCVR_TXENABLE));
}

void max2837_mode_standby(void) {
	/* Used to enable the frequency synthesizer block while the rest of the
	 * device is powered down. In this mode, PLL, VCO, and LO generator
	 * are on, so that Tx or Rx modes can be quickly enabled from this mode.
	 * These and other blocks can be selectively enabled in this mode.
	 */
	gpio_clear(PORT_XCVR_ENABLE, (PIN_XCVR_RXENABLE | PIN_XCVR_TXENABLE));
	gpio_set(PORT_XCVR_ENABLE, PIN_XCVR_ENABLE);
}

void max2837_mode_tx(void) {
	/* All Tx circuit blocks are powered on. The external PA is powered on
	 * after a programmable delay using the on-chip PA bias DAC. The slow-
	 * charging Rx circuits are in a precharged “idle-off” state for fast
	 * Tx-to-Rx turnaround time.
	 */
	gpio_clear(PORT_XCVR_ENABLE, PIN_XCVR_RXENABLE);
	gpio_set(PORT_XCVR_ENABLE,
			(PIN_XCVR_ENABLE | PIN_XCVR_TXENABLE));
}

void max2837_mode_rx(void) {
	/* All Rx circuit blocks are powered on and active. Antenna signal is
	 * applied; RF is downconverted, filtered, and buffered at Rx BB I and Q
	 * outputs. The slow- charging Tx circuits are in a precharged “idle-off”
	 * state for fast Rx-to-Tx turnaround time.
	 */
	gpio_clear(PORT_XCVR_ENABLE, PIN_XCVR_TXENABLE);
	gpio_set(PORT_XCVR_ENABLE,
			(PIN_XCVR_ENABLE | PIN_XCVR_RXENABLE));
}

max2837_mode_t max2837_mode(void) {
	if( gpio_get(PORT_XCVR_ENABLE, PIN_XCVR_ENABLE) ) {
		if( gpio_get(PORT_XCVR_ENABLE, PIN_XCVR_TXENABLE) ) {
			return MAX2837_MODE_TX;
		} else if( gpio_get(PORT_XCVR_ENABLE, PIN_XCVR_RXENABLE) ) {
			return MAX2837_MODE_RX;
		} else {
			return MAX2837_MODE_STANDBY;
		}
	} else {
		return MAX2837_MODE_SHUTDOWN;
	}
}

void max2837_set_mode(const max2837_mode_t new_mode) {
	switch(new_mode) {
	case MAX2837_MODE_SHUTDOWN:
		max2837_mode_shutdown();
		break;
		
	case MAX2837_MODE_STANDBY:
		max2837_mode_standby();
		break;
		
	case MAX2837_MODE_TX:
		max2837_mode_tx();
		break;

	case MAX2837_MODE_RX:
		max2837_mode_rx();
		break;
		
	default:
		break;
	}
}

void max2837_start(void)
{
	LOG("# max2837_start\n");
	set_MAX2837_EN_SPI(1);
	max2837_regs_commit();
#if !defined TEST
	max2837_mode_standby();
#endif
}

void max2837_tx(void)
{
	LOG("# max2837_tx\n");
#if !defined TEST

	set_MAX2837_ModeCtrl(MAX2837_ModeCtrl_TxLPF);
	max2837_regs_commit();
	max2837_mode_tx();
#endif
}

void max2837_rx(void)
{
	LOG("# max2837_rx\n");

	set_MAX2837_ModeCtrl(MAX2837_ModeCtrl_RxLPF);
	max2837_regs_commit();

#if !defined TEST
	max2837_mode_rx();
#endif
}

void max2837_stop(void)
{
	LOG("# max2837_stop\n");
	set_MAX2837_EN_SPI(0);
	max2837_regs_commit();
#if !defined TEST
	max2837_mode_shutdown();
#endif
}

void max2837_set_frequency(uint32_t freq)
{
	uint8_t band;
	uint8_t lna_band;
	uint32_t div_frac;
	uint32_t div_int;
	uint32_t div_rem;
	uint32_t div_cmp;
	int i;

	/* Select band. Allow tuning outside specified bands. */
	if (freq < 2400000000U) {
		band = MAX2837_LOGEN_BSW_2_3;
		lna_band = MAX2837_LNAband_2_4;
	}
	else if (freq < 2500000000U) {
		band = MAX2837_LOGEN_BSW_2_4;
		lna_band = MAX2837_LNAband_2_4;
	}
	else if (freq < 2600000000U) {
		band = MAX2837_LOGEN_BSW_2_5;
		lna_band = MAX2837_LNAband_2_6;
	}
	else {
		band = MAX2837_LOGEN_BSW_2_6;
		lna_band = MAX2837_LNAband_2_6;
	}

	LOG("# max2837_set_frequency %ld, band %d, lna band %d\n",
	    freq, band, lna_band);

	/* ASSUME 40MHz PLL. Ratio = F*(4/3)/40,000,000 = F/30,000,000 */
	div_int = freq / 30000000;
	div_rem = freq % 30000000;
	div_frac = 0;
	div_cmp = 30000000;
	for( i = 0; i < 20; i++) {
		div_frac <<= 1;
		div_cmp >>= 1;
		if (div_rem > div_cmp) {
			div_frac |= 0x1;
			div_rem -= div_cmp;
		}
	}
	LOG("# int %ld, frac %ld\n", div_int, div_frac);

	/* Band settings */
	set_MAX2837_LOGEN_BSW(band);
	set_MAX2837_LNAband(lna_band);

	/* Write order matters here, so commit INT and FRAC_HI before
	 * committing FRAC_LO, which is the trigger for VCO
	 * auto-select. TODO - it's cleaner this way, but it would be
	 * faster to explicitly commit the registers explicitly so the
	 * dirty bits aren't scanned twice. */
	set_MAX2837_SYN_INT(div_int);
	set_MAX2837_SYN_FRAC_HI((div_frac >> 10) & 0x3ff);
	max2837_regs_commit();
	set_MAX2837_SYN_FRAC_LO(div_frac & 0x3ff);
	max2837_regs_commit();
}

typedef struct {
	uint32_t bandwidth_hz;
	uint32_t ft;
} max2837_ft_t;

static const max2837_ft_t max2837_ft[] = {
	{  1750000, MAX2837_FT_1_75M },
	{  2500000, MAX2837_FT_2_5M },
	{  3500000, MAX2837_FT_3_5M },
	{  5000000, MAX2837_FT_5M },
	{  5500000, MAX2837_FT_5_5M },
	{  6000000, MAX2837_FT_6M },
	{  7000000, MAX2837_FT_7M },
	{  8000000, MAX2837_FT_8M },
	{  9000000, MAX2837_FT_9M },
	{ 10000000, MAX2837_FT_10M },
	{ 12000000, MAX2837_FT_12M },
	{ 14000000, MAX2837_FT_14M },
	{ 15000000, MAX2837_FT_15M },
	{ 20000000, MAX2837_FT_20M },
	{ 24000000, MAX2837_FT_24M },
	{ 28000000, MAX2837_FT_28M },
	{        0, 0 },
};

bool max2837_set_lpf_bandwidth(const uint32_t bandwidth_hz) {
	const max2837_ft_t* p = max2837_ft;
	while( p->bandwidth_hz != 0 ) {
		if( p->bandwidth_hz >= bandwidth_hz ) {
			break;
		}
		p++;
	}
	
	if( p->bandwidth_hz != 0 ) {
		set_MAX2837_FT(p->ft);
		max2837_regs_commit();
		return true;
	} else {
		return false;
	}
}

bool max2837_set_lna_gain(const uint32_t gain_db) {
	uint16_t val;
	switch(gain_db){
		case 40:
			val = MAX2837_LNAgain_MAX;
			break;
		case 32:
			val = MAX2837_LNAgain_M8;
			break;
		case 24:
			val = MAX2837_LNAgain_M16;
			break;
		case 16:
			val = MAX2837_LNAgain_M24;
			break;
		case 8:
			val = MAX2837_LNAgain_M32;
			break;
		case 0:
			val = MAX2837_LNAgain_M40;
			break;
		default:
			return false;
	}
	set_MAX2837_LNAgain(val);
	max2837_reg_commit(1);
	return true;
}

bool max2837_set_vga_gain(const uint32_t gain_db) {
	if( (gain_db & 0x1) || gain_db > 62)/* 0b11111*2 */
		return false;
		
	set_MAX2837_VGA( 31-(gain_db >> 1) );
	max2837_reg_commit(5);
	return true;
}

bool max2837_set_txvga_gain(const uint32_t gain_db) {
	uint16_t val=0;
	if(gain_db <16){
		val = 31-gain_db;
		val |= (1 << 5); // bit6: 16db
	} else{
		val = 31-(gain_db-16);
	}
	
	set_MAX2837_TXVGA_GAIN(val);
	max2837_reg_commit(29);
	return true;
}

#ifdef TEST
int main(int ac, char **av)
{
	max2837_setup();
	max2837_set_frequency(2441000000);
	max2837_start();
	max2837_tx();
	max2837_stop();
}
#endif //TEST
