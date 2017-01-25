#ifndef MAX2871_H
#define MAX2871_H

#include <stdint.h>

typedef struct {
	//spi_bus_t* const bus;
	//gpio_t gpio_reset;
	uint16_t regs[1];
	uint32_t regs_dirty;
} max2871_driver_t;

extern void max2871_setup(max2871_driver_t* const drv);
extern uint64_t max2871_set_frequency(max2871_driver_t* const drv, uint16_t mhz);
extern void max2871_tx(max2871_driver_t* const drv);
extern void max2871_rx(max2871_driver_t* const drv);
extern void max2871_rxtx(max2871_driver_t* const drv);
extern void max2871_enable(max2871_driver_t* const drv);
extern void max2871_disable(max2871_driver_t* const drv);
extern void max2871_set_gpo(max2871_driver_t* const drv, uint8_t gpo);
#endif
