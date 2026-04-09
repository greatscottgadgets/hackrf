/*
 * Copyright 2012-2026 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2012 Jared Boone
 * Copyright 2013 Benjamin Vernoux
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

#pragma once

#include "fixed_point.h"
#include "rf_path.h"

/**
 * Select fixed or graduated IF for a given RF and image reject filter.
 */
fp_40_24_t select_graduated_if(fp_40_24_t freq_rf, rf_path_filter_t img_reject);

/**
 * Select image reject filter appropriate for a given RF.
 */
rf_path_filter_t select_img_reject(fp_40_24_t freq_rf);

/**
 * Restrict RF to range appropriate for a given image reject filter.
 */
fp_40_24_t restrict_rf(fp_40_24_t freq_rf, rf_path_filter_t img_reject);
