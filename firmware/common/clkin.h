/*
 * Copyright 2022 Great Scott Gadgets
 *
 * This file is part of HackRF.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

/* Compat shim: clkin.h was renamed to clock_io.h upstream. Kept so
 * out-of-tree consumers (portapack-mayhem) keep building. */

#pragma once

#include "clock_io.h"
