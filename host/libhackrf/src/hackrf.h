// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

// Copyright (c) 2012, Jared Boone <jared@sharebrained.com>
// Copyright (c) 2013, Benjamin Vernoux <titanmkd@gmail.com>
// Copyright (c) 2013, Michael Ossmann <mike@ossmann.com>
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 3. Neither the name of Great Scott Gadgets nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

#ifndef HACKRF_H
#define HACKRF_H

#include <stdint.h>

#ifdef _WIN32
# define ADD_EXPORTS

// You should define ADD_EXPORTS *only* when building the DLL.
# ifdef ADD_EXPORTS
#  define ADDAPI __declspec(dllexport)
# else
#  define ADDAPI __declspec(dllimport)
# endif

// Define calling convention in one place, for convenience.
# define ADDCALL __cdecl

#else // _WIN32 not defined.

// Define with no value on non-Windows OSes.
# define ADDAPI
# define ADDCALL

#endif

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

/// FIXME: doc
#define SAMPLES_PER_BLOCK 8192

/// FIXME: doc
#define BYTES_PER_BLOCK 16384

/// FIXME: doc
#define MAX_SWEEP_RANGES 10

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

/// FIXME: doc
enum hackrf_error {
    HACKRF_SUCCESS                     = 0,
    HACKRF_TRUE                        = 1,
    HACKRF_ERROR_INVALID_PARAM         = -2,
    HACKRF_ERROR_NOT_FOUND             = -5,
    HACKRF_ERROR_BUSY                  = -6,
    HACKRF_ERROR_NO_MEM                = -11,
    HACKRF_ERROR_LIBUSB                = -1000,
    HACKRF_ERROR_THREAD                = -1001,
    HACKRF_ERROR_STREAMING_THREAD_ERR  = -1002,
    HACKRF_ERROR_STREAMING_STOPPED     = -1003,
    HACKRF_ERROR_STREAMING_EXIT_CALLED = -1004,
    HACKRF_ERROR_USB_API_VERSION       = -1005,
    HACKRF_ERROR_OTHER                 = -9999,
};

/// FIXME: doc
enum hackrf_board_id {
    BOARD_ID_JELLYBEAN  = 0,
    BOARD_ID_JAWBREAKER = 1,
    BOARD_ID_HACKRF_ONE = 2,
    BOARD_ID_RAD1O      = 3,
    BOARD_ID_INVALID    = 0xFF,
};

/// FIXME: doc
enum hackrf_usb_board_id {
    USB_BOARD_ID_JAWBREAKER = 0x604B,
    USB_BOARD_ID_HACKRF_ONE = 0x6089,
    USB_BOARD_ID_RAD1O      = 0xCC15,
    USB_BOARD_ID_INVALID    = 0xFFFF,
};

/// FIXME: doc
enum rf_path_filter {
    /// FIXME: doc
    RF_PATH_FILTER_BYPASS = 0,

    /// FIXME: doc
    RF_PATH_FILTER_LOW_PASS = 1,

    /// FIXME: doc
    RF_PATH_FILTER_HIGH_PASS = 2,
};

/// FIXME: doc
enum operacake_ports {
    OPERACAKE_PA1 = 0,
    OPERACAKE_PA2 = 1,
    OPERACAKE_PA3 = 2,
    OPERACAKE_PA4 = 3,
    OPERACAKE_PB1 = 4,
    OPERACAKE_PB2 = 5,
    OPERACAKE_PB3 = 6,
    OPERACAKE_PB4 = 7,
};

/// FIXME: doc
enum sweep_style {
    /// FIXME: doc
    LINEAR      = 0,

    /// FIXME: doc
    INTERLEAVED = 1,
};

/// FIXME: doc
typedef struct hackrf_device hackrf_device;

/// FIXME: doc
typedef struct {
    /// FIXME: doc
    hackrf_device* device;

    /// FIXME: doc
    uint8_t* buffer;

    /// FIXME: doc
    int buffer_length;

    /// FIXME: doc
    int valid_length;

    /// FIXME: doc
    void* rx_ctx;

    /// FIXME: doc
    void* tx_ctx;
} hackrf_transfer;

// FIXME: remove this
typedef struct {
    uint32_t part_id[2];
    uint32_t serial_no[4];
} read_partid_serialno_t;

/// FIXME: doc
struct hackrf_device_list {
    /// FIXME: doc
    char** serial_numbers;

    /// FIXME: doc
    enum hackrf_usb_board_id* usb_board_ids;

    /// FIXME: doc
    int* usb_device_index;

    /// FIXME: doc
    int devicecount;

    /// FIXME: doc
    void** usb_devices;

    /// FIXME: doc
    int usb_devicecount;
};

/// FIXME: doc
typedef struct hackrf_device_list hackrf_device_list_t;

/// FIXME: doc
/// If the return value of a callback of this type is anything other than 0,
/// the transfer will be stopped (and that returned value is thrown away).
typedef int (*hackrf_sample_block_cb_fn)(hackrf_transfer* transfer);

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C"
{
#endif

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_init(void);

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_exit(void);

/// FIXME: doc
extern ADDAPI const char* ADDCALL
hackrf_library_version(void);

/// FIXME: doc
extern ADDAPI const char* ADDCALL
hackrf_library_release(void);

// -----------------------------------------------------------------------------
// ---- Functions related to enumerating HackRF devices ------------------------
// -----------------------------------------------------------------------------

/// FIXME: doc
extern ADDAPI hackrf_device_list_t* ADDCALL
hackrf_device_list(void);

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_device_list_open(hackrf_device_list_t* list,
                        int                   idx,
                        hackrf_device**       device);

/// FIXME: doc
extern ADDAPI void ADDCALL
hackrf_device_list_free(hackrf_device_list_t* list);

// -----------------------------------------------------------------------------
// ---- Functions related to connecting to a HackRF device ---------------------
// -----------------------------------------------------------------------------

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_open(hackrf_device** device);

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_open_by_serial(const char*     desired_serial_number,
                      hackrf_device** device);

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_close(hackrf_device* device);

// -----------------------------------------------------------------------------
// ---- Functions related to streaming -----------------------------------------
// -----------------------------------------------------------------------------

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_start_rx(hackrf_device*            device,
                hackrf_sample_block_cb_fn callback,
                void*                     rx_ctx);

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_stop_rx(hackrf_device* device);

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_start_tx(hackrf_device*            device,
                hackrf_sample_block_cb_fn callback,
                void*                     tx_ctx);

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_stop_tx(hackrf_device* device);

/// FIXME: doc
/// FIXME: maybe this should return a different enum
/// return HACKRF_TRUE if success
extern ADDAPI enum hackrf_error ADDCALL
hackrf_is_streaming(hackrf_device* device);

// -----------------------------------------------------------------------------
// ---- MAX2837-related functions ----------------------------------------------
// -----------------------------------------------------------------------------

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_max2837_read(hackrf_device* device,
                    uint8_t        register_number,
                    uint16_t*      value);

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_max2837_write(hackrf_device* device,
                     uint8_t        register_number,
                     uint16_t       value);

// -----------------------------------------------------------------------------
// ---- Si5351C-related functions ----------------------------------------------
// -----------------------------------------------------------------------------

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_si5351c_read(hackrf_device* device,
                    uint16_t       register_number,
                    uint16_t*      value);

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_si5351c_write(hackrf_device* device,
                     uint16_t       register_number,
                     uint16_t       value);

// -----------------------------------------------------------------------------
// ---- Baseband filter-related functions --------------------------------------
// -----------------------------------------------------------------------------

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_set_baseband_filter_bandwidth(hackrf_device* device,
                                     uint32_t       bandwidth_hz);

// -----------------------------------------------------------------------------
// ---- RFFC5071-related functions ---------------------------------------------
// -----------------------------------------------------------------------------

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_rffc5071_read(hackrf_device* device,
                     uint8_t        register_number,
                     uint16_t*      value);

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_rffc5071_write(hackrf_device* device,
                      uint8_t        register_number,
                      uint16_t       value);

// -----------------------------------------------------------------------------
// ---- SPI flash memory-related functions -------------------------------------
// -----------------------------------------------------------------------------

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_spiflash_erase(hackrf_device* device);

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_spiflash_write(hackrf_device* device,
                      uint32_t       address,
                      uint16_t       length,
                      unsigned char* data);

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_spiflash_read(hackrf_device* device,
                     uint32_t       address,
                     uint16_t       length,
                     unsigned char* data);

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_spiflash_status(hackrf_device* device,
                       uint8_t*       data);

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_spiflash_clear_status(hackrf_device* device);

// -----------------------------------------------------------------------------
// ---- CPLD-related functions -------------------------------------------------
// -----------------------------------------------------------------------------

/// FIXME: doc
/// device will need to be reset after hackrf_cpld_write
extern ADDAPI enum hackrf_error ADDCALL
hackrf_cpld_write(hackrf_device* device,
                  unsigned char* data,
                  unsigned int   total_length);

// -----------------------------------------------------------------------------
// ---- Board metadata-related functions ---------------------------------------
// -----------------------------------------------------------------------------

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_board_id_read(hackrf_device* device,
                     uint8_t* value);

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_version_string_read(hackrf_device* device,
                           char*          version,
                           uint8_t        length);

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_usb_api_version_read(hackrf_device* device,
                            uint16_t*      version);

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_board_partid_serialno_read(hackrf_device*          device,
                                  read_partid_serialno_t* read_partid_serialno);

// -----------------------------------------------------------------------------
// ---- RF-related functions ---------------------------------------------------
// -----------------------------------------------------------------------------

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_set_freq(hackrf_device* device,
                uint64_t       freq_hz);

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_set_freq_explicit(hackrf_device*      device,
                         uint64_t            if_freq_hz,
                         uint64_t            lo_freq_hz,
                         enum rf_path_filter path);

/// FIXME: doc
/// currently 8-20Mhz - either as a fraction, i.e. freq 20000000hz divider 2 -> 10Mhz or as plain old 10000000hz (double)
/// preferred rates are 8, 10, 12.5, 16, 20Mhz due to less jitter
extern ADDAPI enum hackrf_error ADDCALL
hackrf_set_sample_rate_manual(hackrf_device* device,
                              uint32_t       freq_hz,
                              uint32_t       divider);

/// FIXME: doc
extern ADDAPI enum hackrf_error ADDCALL
hackrf_set_sample_rate(hackrf_device* device,
                       double         freq_hz);

/// FIXME: doc
/// external amp, bool on/off
extern ADDAPI enum hackrf_error ADDCALL
hackrf_set_amp_enable(hackrf_device* device,
                      uint8_t        value);

/// FIXME: doc
/// range 0-40 step 8d, IF gain in osmosdr
extern ADDAPI enum hackrf_error ADDCALL
hackrf_set_lna_gain(hackrf_device* device,
                    uint32_t       value);

/// FIXME: doc
/// range 0-62 step 2db, BB gain in osmosdr
extern ADDAPI enum hackrf_error ADDCALL
hackrf_set_vga_gain(hackrf_device* device,
                    uint32_t       value);

/// FIXME: doc
/// range 0-47 step 1db
extern ADDAPI enum hackrf_error ADDCALL
hackrf_set_txvga_gain(hackrf_device* device,
                      uint32_t       value);

/// FIXME: doc
/// antenna port power control
extern ADDAPI enum hackrf_error ADDCALL
hackrf_set_antenna_enable(hackrf_device* device,
                          uint8_t        value);

/// FIXME: doc
/// Compute nearest freq for bw filter (manual filter)
extern ADDAPI uint32_t ADDCALL
hackrf_compute_baseband_filter_bw_round_down_lt(uint32_t bandwidth_hz);

/// FIXME: doc
/// Compute best default value depending on sample rate (auto filter)
extern ADDAPI uint32_t ADDCALL
hackrf_compute_baseband_filter_bw(uint32_t bandwidth_hz);

/// FIXME: doc
/// This function requires USB API version 0x1002 or higher
/// set hardware sync mode
extern ADDAPI enum hackrf_error ADDCALL
hackrf_set_hw_sync_mode(hackrf_device* device,
                        uint8_t        value);

/// FIXME: doc
/// This function requires USB API version 0x1002 or higher
/// Start sweep mode
extern ADDAPI enum hackrf_error ADDCALL
hackrf_init_sweep(hackrf_device*   device,
                  const uint16_t*  frequency_list,
                  int              num_ranges,
                  uint32_t         num_bytes,
                  uint32_t         step_width,
                  uint32_t         offset,
                  enum sweep_style style);

// -----------------------------------------------------------------------------
// ---- Enum-to-string conversion functions ------------------------------------
// -----------------------------------------------------------------------------

/// FIXME: doc
extern ADDAPI const char* ADDCALL
hackrf_error_name(enum hackrf_error errcode);

/// FIXME: doc
extern ADDAPI const char* ADDCALL
hackrf_board_id_name(enum hackrf_board_id board_id);

/// FIXME: doc
extern ADDAPI const char* ADDCALL
hackrf_usb_board_id_name(enum hackrf_usb_board_id usb_board_id);

/// FIXME: doc
extern ADDAPI const char* ADDCALL
hackrf_filter_path_name(enum rf_path_filter path);

// -----------------------------------------------------------------------------
// ---- Operacake functions ----------------------------------------------------
// -----------------------------------------------------------------------------

/// FIXME: doc
/// This function requires USB API version 0x1002 or higher
extern ADDAPI enum hackrf_error ADDCALL
hackrf_get_operacake_boards(hackrf_device* device,
                            uint8_t*       boards);


/// FIXME: doc
/// This function requires USB API version 0x1002 or higher
extern ADDAPI enum hackrf_error ADDCALL
hackrf_set_operacake_ports(hackrf_device*       device,
                           uint8_t              address,
                           enum operacake_ports port_a,
                           enum operacake_ports port_b);


/// FIXME: doc
/// This function requires USB API version 0x1002 or higher
extern ADDAPI enum hackrf_error ADDCALL
hackrf_set_operacake_ranges(hackrf_device* device,
                            uint8_t*       ranges,
                            uint8_t        num_ranges);

/// FIXME: doc
/// FIXME: is this also an Operacake function?
/// This function requires USB API version 0x1002 or higher
extern ADDAPI enum hackrf_error ADDCALL
hackrf_reset(hackrf_device* device);

/// FIXME: doc
/// FIXME: is this also an Operacake function?
/// This function requires USB API version 0x1002 or higher
extern ADDAPI enum hackrf_error ADDCALL
hackrf_set_clkout_enable(hackrf_device* device,
                         uint8_t        value);

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif /*HACKRF_H*/

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
