#ifndef __RFPATH_H__
#define __RFPATH_H__

#include <stdint.h>

void rf_path_init(void);

typedef enum {
	RF_PATH_DIRECTION_RX,
	RF_PATH_DIRECTION_TX,
} rf_path_direction_t;

void rf_path_set_direction(const rf_path_direction_t direction);

typedef enum {
	RF_PATH_FILTER_BYPASS,
	RF_PATH_FILTER_LOW_PASS,
	RF_PATH_FILTER_HIGH_PASS,
} rf_path_filter_t;

void rf_path_set_filter(const rf_path_filter_t filter);

void rf_path_set_lna(const uint_fast8_t enable);

#endif/*__RFPATH_H__*/
