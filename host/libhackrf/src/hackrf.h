/*
Copyright (c) 2012, Jared Boone <jared@sharebrained.com>
Copyright (c) 2013, Benjamin Vernoux <titanmkd@gmail.com>
Copyright (c) 2013, Michael Ossmann <mike@ossmann.com>

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the 
	documentation and/or other materials provided with the distribution.
    Neither the name of Great Scott Gadgets nor the names of its contributors may be used to endorse or promote products derived from this software
	without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __HACKRF_H__
#define __HACKRF_H__

#include <stdint.h>

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

/* external amp, bool on/off */
extern ADDAPI int ADDCALL hackrf_set_amp_enable(hackrf_device* device, const uint8_t value);

extern ADDAPI int ADDCALL hackrf_board_partid_serialno_read(hackrf_device* device, read_partid_serialno_t* read_partid_serialno);

/* range 0-40 step 8db */
extern ADDAPI int ADDCALL hackrf_set_lna_gain(hackrf_device* device, uint32_t value);

/* range 0-62 step 2db */
extern ADDAPI int ADDCALL hackrf_set_vga_gain(hackrf_device* device, uint32_t value);

/* range 0-47 step 1db */
extern ADDAPI int ADDCALL hackrf_set_txvga_gain(hackrf_device* device, uint32_t value);

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
