/*
 * Copyright 2012-2022 Great Scott Gadgets
 * Copyright 2014 Jared Boone <jared@sharebrained.com>
 * Copyright 2012 Will Code <willcode4@gmail.com>
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

#ifndef __MAX2839_TARGET_H
#define __MAX2839_TARGET_H

#include "max2839.h"

void max2839_target_init(max2839_driver_t* const drv);
void max2839_target_set_mode(max2839_driver_t* const drv, const max2839_mode_t new_mode);

#endif // __MAX2839_TARGET_H
