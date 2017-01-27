#ifndef MAX2871_H
#define MAX2871_H

#include "gpio.h"

#include <stdint.h>

typedef struct {
	gpio_t gpio_vco_ce;
	gpio_t gpio_vco_sclk;
	gpio_t gpio_vco_sdata;
	gpio_t gpio_vco_le;
	gpio_t gpio_synt_rfout_en;
	gpio_t gpio_vco_mux;
} max2871_driver_t;

extern void max2871_setup(max2871_driver_t* const drv);
extern uint64_t max2871_set_frequency(max2871_driver_t* const drv, uint16_t mhz);
extern void max2871_enable(max2871_driver_t* const drv);
extern void max2871_disable(max2871_driver_t* const drv);
#endif
