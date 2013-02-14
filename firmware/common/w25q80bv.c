/*
 * This is a rudimentary driver for the W25Q80BV SPI Flash IC using the
 * LPC43xx's SSP0 peripheral (not quad SPIFI).  The only goal here is to allow
 * programming the flash.
 */

#include <stdint.h>
#include <string.h>
#include "w25q80bv.h"
#include "hackrf_core.h"
#include <libopencm3/lpc43xx/ssp.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/gpio.h>

/*
 * Set up pins for GPIO and SPI control, configure SSP0 peripheral for SPI.
 * SSP0_SSEL is controlled by GPIO in order to handle various transfer lengths.
 */

void w25q80bv_setup(void)
{
	/* FIXME speed up once everything is working reliably */
	const uint8_t serial_clock_rate = 32;
	const uint8_t clock_prescale_rate = 128;

	/* configure SSP pins */
	scu_pinmux(SCU_SSP0_MISO, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP0_MOSI, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP0_SCK,  (SCU_SSP_IO | SCU_CONF_FUNCTION2));

	/* configure GPIO pins */
	scu_pinmux(SCU_FLASH_HOLD, SCU_GPIO_FAST);
	scu_pinmux(SCU_FLASH_WP, SCU_GPIO_FAST);
	scu_pinmux(SCU_SSP0_SSEL, (SCU_GPIO_FAST | SCU_CONF_FUNCTION4));

	/* drive SSEL, HOLD, and WP pins high */
	gpio_set(PORT_FLASH, (PIN_FLASH_HOLD | PIN_FLASH_WP));
	gpio_set(PORT_SSP0_SSEL, PIN_SSP0_SSEL);

	/* Set GPIO pins as outputs. */
	GPIO1_DIR |= (PIN_FLASH_HOLD | PIN_FLASH_WP);
	GPIO5_DIR |= PIN_SSP0_SSEL;

	/* initialize SSP0 */
	ssp_init(SSP0_NUM,
			SSP_DATA_8BITS,
			SSP_FRAME_SPI,
			SSP_CPOL_0_CPHA_0,
			serial_clock_rate,
			clock_prescale_rate,
			SSP_MODE_NORMAL,
			SSP_MASTER,
			SSP_SLAVE_OUT_ENABLE);
}

uint8_t w25q80bv_get_status(void) {
	uint8_t value;

	gpio_clear(PORT_SSP0_SSEL, PIN_SSP0_SSEL);
	ssp_transfer(SSP0_NUM, W25Q80BV_READ_STATUS1);
	value = ssp_transfer(SSP0_NUM, 0xFF);
	gpio_set(PORT_SSP0_SSEL, PIN_SSP0_SSEL);

	return value;
}

void w25q80bv_wait_while_busy(void) {
	while (w25q80bv_get_status() & W25Q80BV_STATUS_BUSY);
}

void w25q80bv_chip_erase(void) {
	w25q80bv_wait_while_busy();
	gpio_clear(PORT_SSP0_SSEL, PIN_SSP0_SSEL);
	ssp_transfer(SSP0_NUM, W25Q80BV_CHIP_ERASE);
	gpio_set(PORT_SSP0_SSEL, PIN_SSP0_SSEL);
}

void w25q80bv_write_enable(void) {
	w25q80bv_wait_while_busy();
	gpio_clear(PORT_SSP0_SSEL, PIN_SSP0_SSEL);
	ssp_transfer(SSP0_NUM, W25Q80BV_WRITE_ENABLE);
	gpio_set(PORT_SSP0_SSEL, PIN_SSP0_SSEL);
}

/* write a 256 byte page */
void w25q80bv_page_program(void) {
	int i;

	w25q80bv_write_enable();
	w25q80bv_wait_while_busy();

	gpio_clear(PORT_SSP0_SSEL, PIN_SSP0_SSEL);
	ssp_transfer(SSP0_NUM, W25Q80BV_PAGE_PROGRAM);
	ssp_transfer(SSP0_NUM, 0x00); //FIXME real address high byte
	ssp_transfer(SSP0_NUM, 0x00); //FIXME real address middle byte
	ssp_transfer(SSP0_NUM, 0x00); //FIXME real address low byte
	for (i = 0; i < 256; i++)
		ssp_transfer(SSP0_NUM, i); //FIXME real data
	gpio_set(PORT_SSP0_SSEL, PIN_SSP0_SSEL);
}
