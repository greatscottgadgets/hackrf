/*
 * Copyright 2012-2026 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2012 Benjamin Vernoux <titanmkd@gmail.com>
 * Copyright 2012 Jared Boone <jared@sharebrained.com>
 *
 * This file is part of HackRF.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

/*
 * Compat shim for out-of-tree consumers (portapack-mayhem) that still
 * `#include "hackrf_core.h"`. Upstream split the original header into
 * topic-specific headers; this re-exports the same public surface.
 * New code should include the specific header directly.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "pins.h"
#include "leds.h"
#include "power.h"
#include "clock_gen.h"
#include "cpu_clock.h"
#include "clock_io.h"

#include "cpld_jtag.h"
#include "fixed_point.h"
#include "i2c_bus.h"
#include "max283x.h"
#include "max5864.h"
#include "mixer.h"
#include "radio.h"
#include "rf_path.h"
#include "sgpio.h"
#include "si5351c.h"
#include "spi_ssp.h"
#include "w25q80bv.h"
#include "fpga.h"
#include "ice40_spi.h"

/* Old names kept for source compatibility (renamed upstream in pins.h). */
#define pin_setup    pins_setup
#define pin_shutdown pins_shutdown

#ifdef __cplusplus
}
#endif
