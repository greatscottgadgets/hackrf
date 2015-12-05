#include "mixer.h"
//#include "max2871.h"
// TODO: put max2871_regs.c into the build system
#include "max2871_regs.c"

#if (defined DEBUG)
#include <stdio.h>
#define LOG printf
#else
#define LOG(x,...)
#include <libopencm3/lpc43xx/ssp.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/gpio.h>
#include "hackrf_core.h"
#endif

#include <stdint.h>
#include <string.h>

static void max2871_spi_write(uint8_t r, uint32_t v);
static void max2871_write_registers(void);
static void delay_ms(int ms);

void mixer_setup(void)
{
	/* Configure GPIO pins. */
	scu_pinmux(SCU_VCO_CE, SCU_GPIO_FAST);
	scu_pinmux(SCU_VCO_SCLK, SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
	//Only used for the debug pin config: scu_pinmux(SCU_VCO_SCLK, SCU_GPIO_FAST);
	scu_pinmux(SCU_VCO_SDATA, SCU_GPIO_FAST);
	scu_pinmux(SCU_VCO_LE, SCU_GPIO_FAST);
	scu_pinmux(SCU_VCO_MUX, SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
    scu_pinmux(SCU_SYNT_RFOUT_EN, SCU_GPIO_FAST);

	/* Set GPIO pins as outputs. */
	GPIO_DIR(PORT_VCO_CE) |= PIN_VCO_CE;
	GPIO_DIR(PORT_VCO_SCLK) |= PIN_VCO_SCLK;
	GPIO_DIR(PORT_VCO_SDATA) |= PIN_VCO_SDATA;
	GPIO_DIR(PORT_VCO_LE) |= PIN_VCO_LE;
	GPIO_DIR(PORT_SYNT_RFOUT_EN) |= PIN_SYNT_RFOUT_EN;

    /* MUX is an input */
	GPIO_DIR(PORT_VCO_MUX) &= ~(PIN_VCO_MUX);

	/* set to known state */
	gpio_set(PORT_VCO_CE, PIN_VCO_CE); /* active high */
	gpio_clear(PORT_VCO_SCLK, PIN_VCO_SCLK);
	gpio_clear(PORT_VCO_SDATA, PIN_VCO_SDATA);
	gpio_set(PORT_VCO_LE, PIN_VCO_LE); /* active low */
	gpio_set(PORT_SYNT_RFOUT_EN, PIN_SYNT_RFOUT_EN); /* active high */

    max2871_regs_init();
    int i;
    for(i = 5; i >= 0; i--) {
        max2871_spi_write(i, max2871_get_register(i));
        delay_ms(20);
    }

    max2871_set_INT(1);
    max2871_set_N(4500);
    max2871_set_FRAC(0);
    max2871_set_CPL(0);
    max2871_set_CPT(0);
    max2871_set_P(1);
    max2871_set_M(0);
    max2871_set_LDS(0);
    max2871_set_SDN(0);
    max2871_set_MUX(0x0C);  // Register 6 readback
    max2871_set_DBR(0);
    max2871_set_RDIV2(0);
    max2871_set_R(1); // 40 MHz f_PFD
    max2871_set_REG4DB(1);
    max2871_set_CP(15); // ?: CP charge pump current 0-15
    max2871_set_LDF(1); // INT-N
    max2871_set_LDP(0); // ?: Lock-Detect Precision
    max2871_set_PDP(1);
    max2871_set_SHDN(0);
    max2871_set_TRI(0);
    max2871_set_RST(0);
    max2871_set_VCO(0);
    max2871_set_VAS_SHDN(0);
    max2871_set_VAS_TEMP(1);
    max2871_set_CSM(0);
    max2871_set_MUTEDEL(1);
    max2871_set_CDM(0);
    max2871_set_CDIV(0);
    max2871_set_SDLDO(0);
    max2871_set_SDDIV(0);
    max2871_set_SDREF(0);
    max2871_set_BS(20*40); // For 40 MHz f_PFD
    max2871_set_FB(1); // Do not put DIVA into the feedback loop
    max2871_set_DIVA(0);
    max2871_set_SDVCO(0);
    max2871_set_MTLD(1);
    max2871_set_BDIV(0);
    max2871_set_RFB_EN(0);
    max2871_set_BPWR(0);
    max2871_set_RFA_EN(0);
    max2871_set_APWR(3);
    max2871_set_SDPLL(0);
    max2871_set_F01(1);
    max2871_set_LD(1);
    max2871_set_ADCS(0);
    max2871_set_ADCM(0);

    max2871_write_registers();

    mixer_set_frequency(3500);
}

static void delay_ms(int ms)
{
	uint32_t i;
    while(ms--) {
        for (i = 0; i < 20000; i++) {
            __asm__("nop");
        }
    }
}


static void serial_delay(void)
{
	uint32_t i;

	for (i = 0; i < 2; i++)
		__asm__("nop");
}


/* SPI register write
 *
 * Send 32 bits:
 *  First 29 bits are data
 *  Last 3 bits are register number
 */
static void max2871_spi_write(uint8_t r, uint32_t v) {

#if DEBUG
	LOG("0x%04x -> reg%d\n", v, r);
#else

	uint32_t bits = 32;
	uint32_t msb = 1 << (bits -1);
	uint32_t data = v | r;

	/* make sure everything is starting in the correct state */
	gpio_set(PORT_VCO_LE, PIN_VCO_LE);
	gpio_clear(PORT_VCO_SCLK, PIN_VCO_SCLK);
	gpio_clear(PORT_VCO_SDATA, PIN_VCO_SDATA);

	/* start transaction by bringing LE low */
	gpio_clear(PORT_VCO_LE, PIN_VCO_LE);

	while (bits--) {
		if (data & msb)
			gpio_set(PORT_VCO_SDATA, PIN_VCO_SDATA);
		else
			gpio_clear(PORT_VCO_SDATA, PIN_VCO_SDATA);
		data <<= 1;

		serial_delay();
		gpio_set(PORT_VCO_SCLK, PIN_VCO_SCLK);

		serial_delay();
		gpio_clear(PORT_VCO_SCLK, PIN_VCO_SCLK);
	}

	gpio_set(PORT_VCO_LE, PIN_VCO_LE);
#endif
}

static uint32_t max2871_spi_read(void)
{
	uint32_t bits = 32;
	uint32_t data = 0;

    max2871_spi_write(0x06, 0x0);

    serial_delay();
    gpio_set(PORT_VCO_SCLK, PIN_VCO_SCLK);
    serial_delay();
    gpio_clear(PORT_VCO_SCLK, PIN_VCO_SCLK);
    serial_delay();

	while (bits--) {
		gpio_set(PORT_VCO_SCLK, PIN_VCO_SCLK);
		serial_delay();

		gpio_clear(PORT_VCO_SCLK, PIN_VCO_SCLK);
		serial_delay();

		data <<= 1;
        data |= GPIO_STATE(PORT_VCO_MUX, PIN_VCO_MUX) ? 1 : 0;
	}
    return data;
}

static void max2871_write_registers(void)
{
    int i;
    for(i = 5; i >= 0; i--) {
        max2871_spi_write(i, max2871_get_register(i));
    }
}

/* Set frequency (MHz). */
uint64_t mixer_set_frequency(uint16_t mhz)
{
    int n = mhz / 40;
    int diva = 0;

    while(n * 40 < 3000) {
        n *= 2;
        diva += 1;
    }

    max2871_set_RFA_EN(0);
    max2871_write_registers();

    max2871_set_N(n);
    max2871_set_DIVA(diva);
    max2871_write_registers();

    while(max2871_spi_read() & MAX2871_VASA);

    max2871_set_RFA_EN(1);
    max2871_write_registers();

    return (mhz/40)*40 * 1000000;
}

void mixer_tx(void)
{}
void mixer_rx(void)
{}
void mixer_rxtx(void)
{}
void mixer_enable(void)
{}
void mixer_disable(void)
{}
void mixer_set_gpo(uint8_t gpo)
{
    (void) gpo;
}


