#include "mixer.h"
#include "rffc5071.h"
#include "rffc5071_spi.h"
#include "max2871.h"
#include "gpio_lpc.h"

/* RFFC5071 GPIO serial interface PinMux */
#if (defined JAWBREAKER || defined HACKRF_ONE)
static struct gpio_t gpio_rffc5072_select	= GPIO(2, 13);
static struct gpio_t gpio_rffc5072_clock	= GPIO(5,  6);
static struct gpio_t gpio_rffc5072_data		= GPIO(3,  3);
static struct gpio_t gpio_rffc5072_reset	= GPIO(2, 14);
#endif
#ifdef RAD1O
static struct gpio_t gpio_vco_ce			= GPIO(2, 13);
static struct gpio_t gpio_vco_sclk			= GPIO(5,  6);
static struct gpio_t gpio_vco_sdata			= GPIO(3,  3);
static struct gpio_t gpio_vco_le			= GPIO(2, 14);
static struct gpio_t gpio_vco_mux			= GPIO(5, 25);
static struct gpio_t gpio_synt_rfout_en		= GPIO(3,  5);
#endif

#if (defined JAWBREAKER || defined HACKRF_ONE)
const rffc5071_spi_config_t rffc5071_spi_config = {
	.gpio_select = &gpio_rffc5072_select,
	.gpio_clock = &gpio_rffc5072_clock,
	.gpio_data = &gpio_rffc5072_data,
};

spi_bus_t spi_bus_rffc5071 = {
	.config = &rffc5071_spi_config,
	.start = rffc5071_spi_start,
	.stop = rffc5071_spi_stop,
	.transfer = rffc5071_spi_transfer,
	.transfer_gather = rffc5071_spi_transfer_gather,
};

mixer_driver_t mixer = {
	.bus = &spi_bus_rffc5071,
	.gpio_reset = &gpio_rffc5072_reset,
};
#endif
#ifdef RAD1O
mixer_driver_t mixer = {
	.gpio_vco_ce = &gpio_vco_ce,
	.gpio_vco_sclk = &gpio_vco_sclk,
	.gpio_vco_sdata = &gpio_vco_sdata,
	.gpio_vco_le = &gpio_vco_le,
	.gpio_synt_rfout_en = &gpio_synt_rfout_en,
	.gpio_vco_mux = &gpio_vco_mux,
};
#endif

void mixer_bus_setup(mixer_driver_t* const mixer)
{
#if (defined JAWBREAKER || defined HACKRF_ONE)
	(void) mixer;
	spi_bus_start(&spi_bus_rffc5071, &rffc5071_spi_config);
#endif
#ifdef RAD1O
	(void) mixer;
#endif
}

void mixer_setup(mixer_driver_t* const mixer)
{
#if (defined JAWBREAKER || defined HACKRF_ONE)
	rffc5071_setup(mixer);
#endif
#ifdef RAD1O
	max2871_setup(mixer);
#endif
}

uint64_t mixer_set_frequency(mixer_driver_t* const mixer, uint16_t mhz)
{
#if (defined JAWBREAKER || defined HACKRF_ONE)
	return rffc5071_set_frequency(mixer, mhz);
#endif
#ifdef RAD1O
	return max2871_set_frequency(mixer, mhz);
#endif
}

void mixer_tx(mixer_driver_t* const mixer)
{
#if (defined JAWBREAKER || defined HACKRF_ONE)
	rffc5071_tx(mixer);
#endif
#ifdef RAD1O
	(void) mixer;
#endif
}

void mixer_rx(mixer_driver_t* const mixer)
{
#if (defined JAWBREAKER || defined HACKRF_ONE)
	rffc5071_rx(mixer);
#endif
#ifdef RAD1O
	(void) mixer;
#endif
}

void mixer_rxtx(mixer_driver_t* const mixer)
{
#if (defined JAWBREAKER || defined HACKRF_ONE)
	rffc5071_rxtx(mixer);
#endif
#ifdef RAD1O
	(void) mixer;
#endif
}

void mixer_enable(mixer_driver_t* const mixer)
{
#if (defined JAWBREAKER || defined HACKRF_ONE)
	rffc5071_enable(mixer);
#endif
#ifdef RAD1O
	max2871_enable(mixer);
#endif
}

void mixer_disable(mixer_driver_t* const mixer)
{
#if (defined JAWBREAKER || defined HACKRF_ONE)
	rffc5071_disable(mixer);
#endif
#ifdef RAD1O
	max2871_disable(mixer);
#endif
}

void mixer_set_gpo(mixer_driver_t* const mixer, uint8_t gpo)
{
#if (defined JAWBREAKER || defined HACKRF_ONE)
	rffc5071_set_gpo(mixer, gpo);
#endif
#ifdef RAD1O
	(void) mixer;
	(void) gpo;
#endif
}
