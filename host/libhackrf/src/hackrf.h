/*
 * Copyright 2012 Jared Boone <jared@sharebrained.com>
 * Copyright 2013 Benjamin Vernoux <titanmkd@gmail.com>
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

#ifndef __HACKRF_H__
#define __HACKRF_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
   #define ADD_EXPORTS
   
  /* You should define ADD_EXPORTS *only* when building the DLL. */
  #ifdef ADD_EXPORTS
    #define ADDAPI __declspec(dllexport)
  #else
    #define ADDAPI __declspec(dllimport)
  #endif

  /* Define calling convention in one place, for convenience. */
  #define ADDCALL __cdecl

#else /* _WIN32 not defined. */

  /* Define with no value on non-Windows OSes. */
  #define ADDAPI
  #define ADDCALL

#endif

enum hackrf_error {
	HACKRF_SUCCESS = 0,
	HACKRF_TRUE = 1,
	HACKRF_ERROR_INVALID_PARAM = -2,
	HACKRF_ERROR_NOT_FOUND = -5,
	HACKRF_ERROR_BUSY = -6,
	HACKRF_ERROR_NO_MEM = -11,
	HACKRF_ERROR_LIBUSB = -1000,
	HACKRF_ERROR_THREAD = -1001,
	HACKRF_ERROR_STREAMING_THREAD_ERR = -1002,
	HACKRF_ERROR_STREAMING_STOPPED = -1003,
	HACKRF_ERROR_STREAMING_EXIT_CALLED = -1004,
	HACKRF_ERROR_OTHER = -9999,
};

enum hackrf_board_id {
	BOARD_ID_JELLYBEAN  = 0,
	BOARD_ID_JAWBREAKER = 1,
	BOARD_ID_INVALID = 0xFF,
};

typedef struct hackrf_device hackrf_device;

typedef struct {
	hackrf_device* device;
	uint8_t* buffer;
	int buffer_length;
	int valid_length;
	void* rx_ctx;
	void* tx_ctx;
} hackrf_transfer;

typedef struct {
	uint32_t part_id[2];
	uint32_t serial_no[4];
} read_partid_serialno_t;

typedef int (*hackrf_sample_block_cb_fn)(hackrf_transfer* transfer);

#ifdef __cplusplus
extern "C"
{
#endif

extern ADDAPI int ADDCALL hackrf_init();
extern ADDAPI int ADDCALL hackrf_exit();
 
extern ADDAPI int ADDCALL hackrf_open(hackrf_device** device);
extern ADDAPI int ADDCALL hackrf_close(hackrf_device* device);
 
extern ADDAPI int ADDCALL hackrf_start_rx(hackrf_device* device, hackrf_sample_block_cb_fn callback, void* rx_ctx);
extern ADDAPI int ADDCALL hackrf_stop_rx(hackrf_device* device);
 
extern ADDAPI int ADDCALL hackrf_start_tx(hackrf_device* device, hackrf_sample_block_cb_fn callback, void* tx_ctx);
extern ADDAPI int ADDCALL hackrf_stop_tx(hackrf_device* device);

/* return HACKRF_TRUE if success */
extern ADDAPI int ADDCALL hackrf_is_streaming(hackrf_device* device);
 
extern ADDAPI int ADDCALL hackrf_max2837_read(hackrf_device* device, uint8_t register_number, uint16_t* value);
extern ADDAPI int ADDCALL hackrf_max2837_write(hackrf_device* device, uint8_t register_number, uint16_t value);
 
extern ADDAPI int ADDCALL hackrf_si5351c_read(hackrf_device* device, uint16_t register_number, uint16_t* value);
extern ADDAPI int ADDCALL hackrf_si5351c_write(hackrf_device* device, uint16_t register_number, uint16_t value);
 
extern ADDAPI int ADDCALL hackrf_sample_rate_set(hackrf_device* device, const uint32_t sampling_rate_hz);
extern ADDAPI int ADDCALL hackrf_baseband_filter_bandwidth_set(hackrf_device* device, const uint32_t bandwidth_hz);
 
extern ADDAPI int ADDCALL hackrf_rffc5071_read(hackrf_device* device, uint8_t register_number, uint16_t* value);
extern ADDAPI int ADDCALL hackrf_rffc5071_write(hackrf_device* device, uint8_t register_number, uint16_t value);
 
extern ADDAPI int ADDCALL hackrf_spiflash_erase(hackrf_device* device);
extern ADDAPI int ADDCALL hackrf_spiflash_write(hackrf_device* device, const uint32_t address, const uint16_t length, unsigned char* const data);
extern ADDAPI int ADDCALL hackrf_spiflash_read(hackrf_device* device, const uint32_t address, const uint16_t length, unsigned char* data);

extern ADDAPI int ADDCALL hackrf_cpld_write(hackrf_device* device, const uint16_t length,
		unsigned char* const data, const uint16_t total_length);
		
extern ADDAPI int ADDCALL hackrf_board_id_read(hackrf_device* device, uint8_t* value);
extern ADDAPI int ADDCALL hackrf_version_string_read(hackrf_device* device, char* version, uint8_t length);

extern ADDAPI int ADDCALL hackrf_set_freq(hackrf_device* device, const uint64_t freq_hz);

extern ADDAPI int ADDCALL hackrf_set_amp_enable(hackrf_device* device, const uint8_t value);

extern ADDAPI int ADDCALL hackrf_board_partid_serialno_read(hackrf_device* device, read_partid_serialno_t* read_partid_serialno);

extern ADDAPI const char* ADDCALL hackrf_error_name(enum hackrf_error errcode);
extern ADDAPI const char* ADDCALL hackrf_board_id_name(enum hackrf_board_id board_id);

/* Compute nearest freq for bw filter (manual filter) */
extern ADDAPI uint32_t ADDCALL hackrf_compute_baseband_filter_bw_round_down_lt(const uint32_t bandwidth_hz);
/* Compute best default value depending on sample rate (auto filter) */
extern ADDAPI uint32_t ADDCALL hackrf_compute_baseband_filter_bw(const uint32_t bandwidth_hz);

#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif//__HACKRF_H__
