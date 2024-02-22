/*
Copyright (c) 2012-2022 Great Scott Gadgets <info@greatscottgadgets.com>
Copyright (c) 2012, Jared Boone <jared@sharebrained.com>
Copyright (c) 2013, Benjamin Vernoux <titanmkd@gmail.com>

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
#include <sys/types.h>
#include <stdbool.h> // for bool
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

	#define ADDAPI
	#define ADDCALL

#endif

/**
 * @defgroup library Library related functions and enums
 * 
 * @brief Library initialization, exit, error handling, etc.
 * 
 * # Library initialization & exit
 * 
 * The libhackrf library needs to be initialized in order to use most of its functions. This can be achieved via the function @ref hackrf_init. This initializes internal state and initializes `libusb`. You should only call this function on startup, but it's safe to call it later as well, only it does nothing.
 * 
 * When exiting the program, a call to @ref hackrf_exit should be called. This releases all resources, stops background thread and exits `libusb`. This function should only be called if all streaming is stopped and all devices are closed via @ref hackrf_close, else the error @ref HACKRF_ERROR_NOT_LAST_DEVICE is returned.
 * 
 * # Error handling
 * 
 * Many of the functions in libhackrf can signal errors via returning @ref hackrf_error. This enum is backed by an integer, thus these functions are declared to return an int, but they in fact return an enum variant. The special case @ref HACKRF_SUCCESS signals no errors, so return values should be matched for that. It is also set to the value 0, so boolean conversion can also be used. The function @ref hackrf_error_name can be used to convert the enum into a human-readable string, useful for logging the error.
 * 
 * There is a special variant @ref HACKRF_TRUE, used by some functions that return boolean. This fact is explicitly mentioned at those functions.
 * 
 * Typical error-handling code example:
 * ```c
 *  result = hackrf_init();
 *  if (result != HACKRF_SUCCESS) {
 *      fprintf(stderr,
 *          "hackrf_init() failed: %s (%d)\n",
 *          hackrf_error_name(result),
 *          result);
 *      return EXIT_FAILURE;
 *  }
 * ```
 * 
 * Instead of `if (result != HACKRF_SUCCESS)` the line `if (result)` can also be used with the exact same behaviour.
 * 
 * The special case @ref HACKRF_TRUE is only used by @ref hackrf_is_streaming
 * 
 * # Enum conversion
 * 
 * Most of the enums defined in libhackrf have a corresponding `_name` function that converts the enum value into a human-readable string. All strings returned by these functions are statically allocated and do not need to be `free`d. An example is the already mentioned @ref hackrf_error_name function for the @ref hackrf_error enum.
 * 
 * # Library internals
 * 
 * The library uses `libusb` (version 1.0) to communicate with HackRF hardware. It uses both the synchronous and asynchronous API for communication (asynchronous for streaming data to/from the device, and synchronous for everything else). The asynchronous API requires to periodically call a variant of `libusb_handle_events`, so the library creates a new "transfer thread" for each device doing that using the `pthread` library. The library uses multiple transfers for each device (@ref hackrf_get_transfer_queue_depth).
 *
 * # USB API versions
 * As all functionality of HackRF devices requires cooperation between the firmware and the host, both devices can have outdated software. If host machine software is outdated, the new functions will be unalaviable in `hackrf.h`, causing linking errors. If the device firmware is outdated, the functions will return @ref HACKRF_ERROR_USB_API_VERSION.
 * Since device firmware and USB API are separate (but closely related), USB API has its own version numbers.
 * Here is a list of all the functions that require a certain minimum USB API version, up to version 0x0107
 * ## 0x0102
 * - @ref hackrf_set_hw_sync_mode
 * - @ref hackrf_init_sweep
 * - @ref hackrf_set_operacake_ports
 * - @ref hackrf_reset
 * ## 0x0103
 * - @ref hackrf_spiflash_status
 * - @ref hackrf_spiflash_clear_status
 * - @ref hackrf_set_operacake_ranges
 * - @ref hackrf_set_operacake_freq_ranges
 * - @ref hackrf_set_clkout_enable
 * - @ref hackrf_operacake_gpio_test
 * - @ref hackrf_cpld_checksum
 * ## 0x0104
 * - @ref hackrf_set_ui_enable
 * - @ref hackrf_start_rx_sweep
 * ## 0x0105
 * - @ref hackrf_get_operacake_boards
 * - @ref hackrf_set_operacake_mode
 * - @ref hackrf_get_operacake_mode
 * - @ref hackrf_set_operacake_dwell_times
 * ## 0x0106
 * - @ref hackrf_get_m0_state
 * - @ref hackrf_set_tx_underrun_limit
 * - @ref hackrf_set_rx_overrun_limit
 * - @ref hackrf_get_clkin_status
 * - @ref hackrf_board_rev_read
 * - @ref hackrf_supported_platform_read
 * ## 0x0107
 * - @ref hackrf_set_leds
 */

/**
 * @defgroup device Device listing, opening, closing and querying
 * 
 * @brief Managing HackRF devices and querying information about them
 * 
 * The libhackrf library interacts via HackRF hardware through a @ref hackrf_device handle. This handle is opaque, meaning its fields are internal to the library and should not be accessed by user code. To use a device, it first needs to be opened, than it can be interacted with, and finally the device needs to be closed via @ref hackrf_close.
 * 
 * # Opening devices
 * 
 * ## Open first device
 * 
 * @ref hackrf_open opens the first USB device (chosen by libusb). Useful if only one HackRF device is expected to be present.
 * 
 * ## Open by serial
 * 
 * @ref hackrf_open_by_serial opens a device by a given serial (suffix). If no serial is specified it defaults to @ref hackrf_open
 * 
 * ## Open by listing
 * 
 * All connected HackRF devices can be listed via @ref hackrf_device_list. The list must be freed by @ref hackrf_device_list_free.
 * 
 * This struct lists all devices and their serial numbers. Any one of them can be opened by @ref hackrf_device_list_open. All the fields should be treated read-only!
 * 
 * # Closing devices
 * 
 * If the device is not needed anymore, then it can be closed via @ref hackrf_close. Closing a device terminates all ongoing transfers, and resets the device to IDLE mode.
 * 
 * # Querying device information
 * 
 * ## Board ID
 * 
 * Board ID identifies the type of HackRF board connected. See the enum @ref hackrf_board_id for possible values. The value can be read by @ref hackrf_board_id_read and converted into a human-readable string using @ref hackrf_board_id_name. When reading, the initial value of the enum should be @ref BOARD_ID_UNDETECTED.
 * 
 * ## Version string
 * 
 * Version string identifies the firmware version on the board. It can be read with the function @ref hackrf_version_string_read
 * 
 * ## USB API version
 * 
 * USB API version identifies the USB API supported by the device's firmware. It is coded as a xx.xx 16-bit value, and can be read by @ref hackrf_usb_api_version_read
 * 
 * Example of reading firmware and USB API version (from [hackrf_info.c](https://github.com/greatscottgadgets/hackrf/blob/eff4a20022ca5d7f11405c3cdeea6c4195e347d0/host/hackrf-tools/src/hackrf_info.c#L157-L178)):
 * 
 *  ```c
 *  result = hackrf_version_string_read(device, &version[0], 255);
 *  if (result != HACKRF_SUCCESS) {
 *      fprintf(stderr,
 *          "hackrf_version_string_read() failed: %s (%d)\n",
 *          hackrf_error_name(result),
 *          result);
 *      return EXIT_FAILURE;
 *  }
 *
 *
 *  result = hackrf_usb_api_version_read(device, &usb_version);
 *  if (result != HACKRF_SUCCESS) {
 *      fprintf(stderr,
 *          "hackrf_usb_api_version_read() failed: %s (%d)\n",
 *          hackrf_error_name(result),
 *          result);
 *      return EXIT_FAILURE;
 *  }
 *  printf("Firmware Version: %s (API:%x.%02x)\n",
 *      version,
 *      (usb_version >> 8) & 0xFF,
 *      usb_version & 0xFF);
 *  ```
 * 
 * 
 * ## Part ID and serial number
 * 
 * The part ID and serial number of the MCU. Read via @ref hackrf_board_partid_serialno_read. See the documentation of the MCU for details.
 * 
 * ## Board revision
 * 
 * Board revision identifies revision of the HackRF board inside a device. Read via @ref hackrf_board_rev_read and converted into a human-readable string via @ref hackrf_board_rev_name. See @ref hackrf_board_rev for possible values. When reading, the value should be initialized with @ref BOARD_REV_UNDETECTED
 * 
 * ## Supported platform
 * 
 * Identifies the platform supported by the firmare of the HackRF device. Read via @ref hackrf_supported_platform_read. Returns a bitfield. Can identify bad firmware version on device.
 * 
 */

/**
 * @defgroup configuration Configuration of the RF hardware
 * 
 * @brief Configuring gain, sample rate, filter bandwidth, etc.
 * 
 * # Amplifiers and gains
 * 
 * There are 5 different amplifiers in the HackRF One. Most of them have variable gain, but some of them can be either enabled / disabled. Please note that most of the gain settings are not precise, and they depend on the used frequency as well.
 * 
 * ![hackrf components](https://hackrf.readthedocs.io/en/latest/_images/block-diagram.png)
 * 
 * (image taken from https://hackrf.readthedocs.io/en/latest/hardware_components.html)
 * 
 * 
 * ## RX path
 * 
 * - baseband gain in the MAX2837 ("BB" or "VGA") - 0-62dB in 2dB steps, configurable via the @ref hackrf_set_vga_gain function
 * - RX IF gain in the MAX2837 ("IF") - 0-40dB with 8dB steps, configurabe via the @ref hackrf_set_lna_gain function
 * - RX RF amplifier near the antenna port ("RF") - 0 or ~11dB, either enabled or disabled via the @ref hackrf_set_amp_enable (same function is used for enabling/disabling the TX RF amp in TX mode)
 * 
 * ## TX path
 * - TX IF gain in the MAX2837 ("IF" or "VGA") - 0-47dB in 1dB steps, configurable via @ref hackrf_set_txvga_gain
 * - TX RF amplifier near the antenna port ("RF") - 0 or ~11dB, either enabled or disabled via the @ref hackrf_set_amp_enable (same function is used for enabling/disabling the RX RF amp in RX mode)
 * 
 * # Tuning
 * 
 * The HackRF One can tune to nearly any frequency between 1-6000MHz (and the theoretical limit is even a bit higher). This is achieved via up/downconverting the RF section of the MAX2837 transceiver IC with the RFFC5072 mixer/synthesizer's local oscillator. The mixer produces the sum and difference frequencies of the IF and LO frequencies, and a LPF or HPF filter can be used to select one of the resulting frequencies. There is also the possibility to bypass the filter and use the IF as-is. The IF and LO frequencies can be programmed independently, and the behaviour is selectable. See the function @ref hackrf_set_freq_explicit for more details on it.
 * 
 * There is also the convenience function @ref hackrf_set_freq that automatically select suitable LO and IF frequencies and RF path for a desired frequency. It should be used in most cases.
 * 
 * # Filtering
 * 
 * The MAX2837 has an internal selectable baseband filter for both RX and TX. Its width can be set via @ref hackrf_set_baseband_filter_bandwidth, but only some values are valid. Valid values can be acquired via the functions @ref hackrf_compute_baseband_filter_bw_round_down_lt and @ref hackrf_compute_baseband_filter_bw.
 * 
 * **NOTE** in order to avoid aliasing, the bandwidth should not exceed the sample rate. As a sensible default, the firmware auto-sets the baseband filter bandwidth to a value \f$ \le 0.75 \cdot F_s \f$ whenever the sample rate is changed, thus setting a custom value should be done after setting the samplerate.
 * 
 * # Sample rate
 * 
 * The sample rate of the ADC/DAC can be set between 2-20MHz via @ref hackrf_set_sample_rate or @ref hackrf_set_sample_rate_manual. This also automatially adjusts the baseband filter bandwidth to a suitable value.
 * 
 * # Clocking
 * 
 * The HackRF one has external clock input and clock output connectors for 10MHz 3.3V clock signals. It automatically switches to the external clock if it's detected, and its status is readable with @ref hackrf_get_clkin_status. The external clock can be enabled by the @ref hackrf_set_clkout_enable function.
 * 
 * # Bias-tee
 * 
 * The HackRF one has a built in bias-tee (also called (antenna) port power in some of the documentation) capable of delivering 50mA@3V3 for powering small powered antennas or amplifiers. It can be enabled via the @ref hackrf_set_antenna_enable function. Please note that when the device is returning to IDLE mode, the firmware automatically disables this feature. This means it can't be enabled permanently like with the RTL-SDR, and all software using the HackRF must enable this separatlely.
 */

/**
 * @defgroup streaming Transmit & receive operation
 * 
 * @brief RX and TX, callbacks
 * 
 * ## Streaming
 * 
 * There are 3 different streaming modes supported by HackRF:
 * - transmitting (TX)
 * - receiving (RX)
 * - swept receiving (SWEEP)
 * 
 * Each mode needs to be initialized before use, then the mode needs to be entered with the `hackrf_start_*` function. Data transfer happens through callbacks.
 * 
 * There are 3 types of callbacks in the library:
 * - transfer callback
 * - flush callback
 * - block complete callback
 * 
 * Steps for starting an RX or TX operation:
 * - initialize libhackrf
 * - open device
 * - setup device (frequency, samplerate, gain, etc)
 * - setup callbacks, start operation (`hackrf_start_*`)
 * - the main program should go to sleep
 * - when done, the transfer callback should return non-zero value, and signal the main thread to stop
 * - stop operation via `hackrf_stop_*`
 * - close device, exit library, etc.
 * 
 * Data is transfered through the USB connection via setting up multiple async libusb transfers (@ref hackrf_get_transfer_queue_depth). In TX mode, the transfers needs to be filled before submitting, and in RX mode, they need to be read out when they are done. This is done using the transfer callback - it receives a @ref hackrf_transfer object and needs to transfer the data to/from it. As it's needed for all operations, this gets called whenever we need to move data, so every time a transfer is finished (and before the first transfer in TX mode). There's a "transfer complete callback" that only gets called when a transfer is completed. It does not need to do anything special tho, and is optional.
 * 
 * Streaming can be stopped via returning a non-zero value from the transfer callback, but that does NOT reset the device to IDLE mode, it only stops data transfers. In TX mode, when this happens, and the transmitter runs out of data to transmit, it will start transmitting all 0 values (but in older firmware versions, it started repeating the last buffer). To actually stop the operation, a call to `hackrf_stop_*` is needed. Since the callback operate in an async libusb context, such a call can't be made from there, only from the main thread, so it must be signaled through some means (for example, a global variable, or better, a `pthread_cond`) to stop. In RX mode, this signaling can be done from the transfer callback, but in TX mode, we must make sure that we only stop the operation when the last transfer is completed and the device transmitted it, or we might lose it. For this reason, the third **flush callback** exists, that gets called when this happens. It is adivsed to only signal the main thread to stop from this callback.
 * 
 * The function @ref hackrf_is_streaming can be used to check if the device is streaming or not.
 * 
 * ### Transfer callback
 * 
 * Set when starting an operation with @ref hackrf_start_tx, @ref hackrf_start_rx or @ref hackrf_start_rx_sweep. This callback supplies / receives data. This function takes a @ref hackrf_transfer struct as a parameter, and fill/read data to/from its buffer. This function runs in an async libusb context, meaning it should not iteract with the libhackrf library in other ways. The callback can return a boolean value, if its return value is non-zero then it won't be called again, meaning that no future transfers will take place, and (in TX case) the flush callback will be called shortly.
 * 
 * ### Block complete callback
 * 
 * This callback is optional, and only applicable in TX mode. It gets called whenever a data transfer is finished, and can read the data. It needs to do nothing at all. This callback can be set using @ref hackrf_set_tx_block_complete_callback
 * 
 * ### Flush callback
 * 
 * This callback is optional, and only applicable in TX mode. It get called when the last transfer is completed, and it's advisable to only stop streaming via this callback. This callback can be set using @ref hackrf_enable_tx_flush
 * 
 * ### Example TX code utilizing the transfer and flush callbacks.
 * ```
 * // Transmit a 440Hz triangle wave through FM (144.5MHz) using the libhackrf API 
 * // Copyright (c) 2022 László Baráth "Uncle Dino" HA7DN <https://github.com/Sasszem>
 * #include <libhackrf/hackrf.h>
 * #include <math.h>
 * #include <stdio.h>
 * #include <pthread.h>
 * #include <unistd.h>
 * #include <complex.h>
 * #include <stdint.h>
 * 
 * const double f_mod = 440;
 * const uint64_t sample_rate = 10000000;
 * 
 * double triangle() {
 *     // Generate an f_mod frequency triangle wave in the -1 - 1 region
 *     // each call to this function generates a single sample
 *     static double state;
 *     static uint64_t samples_generated;
 *     
 *     const uint64_t period_in_samples = sample_rate / f_mod;
 *     const double step = 4.0 / period_in_samples; // we need to go from -1 to 1 in half the period
 * 
 *     if (samples_generated < period_in_samples / 2 )
 *         state += step;
 *     else
 *         state -= step;
 * 
 *     // this way we don't need to modulo it
 *     if (samples_generated ++ == period_in_samples)
 *         samples_generated = 0;
 * 
 *     return state - 1.0;
 * }
 * 
 * volatile double complex phasor = 1.0;
 * int xfered_samples = 0;
 * int samples_to_xfer = 5*sample_rate;
 * volatile int should_stop = 0;
 * 
 * pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
 * pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
 * 
 * int transfer_callback(hackrf_transfer *transfer) {
 *     int8_t *signed_buffer = (int8_t*)transfer->buffer;
 *     for (int i = 0; i<transfer->buffer_length; i+=2) {
 *         phasor *= cexp(I*6.28*3000 / sample_rate*triangle());
 *         // any IQ samples can be written here, now I'm doing FM modulation with a triangle wave
 *         signed_buffer[i] = 128 * creal(phasor);
 *         signed_buffer[i+1] = 128 * cimag(phasor);
 *     }
 *     transfer->valid_length = transfer->buffer_length;
 *     xfered_samples += transfer->buffer_length;
 *     if (xfered_samples >= samples_to_xfer) {
 *         return 1;
 *     }
 *     return 0;
 * }
 * 
 * void flush_callback(hackrf_transfer *transfer) {
 *     pthread_mutex_lock(&mutex);
 *     pthread_cond_broadcast(&cond);
 *     pthread_mutex_unlock(&mutex);
 * }
 * 
 * int main() {        
 *     hackrf_init();
 *     hackrf_device *device = NULL;
 *     hackrf_open(&device);
 *     
 *     hackrf_set_freq(device, 144500000);
 *     hackrf_set_sample_rate(device, 10000000);
 *     hackrf_set_amp_enable(device, 1);
 *     hackrf_set_txvga_gain(device, 20);
 *     // hackrf_set_tx_underrun_limit(device, 100000); // new-ish library function, not always available
 *     hackrf_enable_tx_flush(device, flush_callback, NULL);
 *     hackrf_start_tx(device, transfer_callback, NULL);
 * 
 *     pthread_mutex_lock(&mutex);
 *     pthread_cond_wait(&cond, &mutex); // wait fo transfer to complete
 *     
 *     hackrf_stop_tx(device);
 *     hackrf_close(device);
 *     hackrf_exit();
 *     return 0;
 * }
 * ```
 * 
 * This code can be compiled using `gcc -o triangle triangle.c -lm -lhackrf`. It generates and transmits a 440Hz triangle wave using FM modulation on the 2m HAM band (**check your local laws and regulations on transmitting and only transmit on bands you have license to!**).
 * 
 * For a more complete example, including error handling and more settings, see [hackrf_transfer.c](https://github.com/greatscottgadgets/hackrf/blob/master/host/hackrf-tools/src/hackrf_transfer.c)
 * 
 * ## Underrun and overrun
 * 
 * Underrun/overrun detection can be enabled using @ref hackrf_set_tx_underrun_limit or @ref hackrf_set_rx_overrun_limit limit. This causes the HackRF to stop operation if more than the specified amount of samples get lost, for example in case of your program crashing, USB connection faliure, etc.
 * 
 * ## Sweeping
 * 
 * Sweeping mode is kind of special. In this mode, the device can be programmed to a list of frequencies to tune on, record set amount of samples and then tune to the next frequency and repeat. It can be setup via @ref hackrf_init_sweep and started with @ref hackrf_start_rx_sweep. In this mode, **the callback does not receive raw samples**, but blocks of samples prefixed with a frequency header specifying the tuned frequency. 
 * 
 * See [hackrf_sweep.c](https://github.com/greatscottgadgets/hackrf/blob/master/host/hackrf-tools/src/hackrf_sweep.c#L236-L249) for a full example, and especialy [the start of the RX callback](https://github.com/greatscottgadgets/hackrf/blob/eff4a20022ca5d7f11405c3cdeea6c4195e347d0/host/hackrf-tools/src/hackrf_sweep.c#L236-L249) for parsing the frequency header.
 * 
 * ## HW sync mode
 * 
 * @ref hackrf_set_hw_sync_mode can be used to setup HW sync mode ([see the documentation on this mode](https://hackrf.readthedocs.io/en/latest/hardware_triggering.html)). This mode allows multiple HackRF Ones to synchronize operations, or one HackRF One to synchrnonize on an external trigger source.
 */

/**
 * @defgroup debug Firmware flashing & debugging
 * @brief Firmware flashing and directly accessing hardware components 
 * 
 * 
 * # Firmware flashing
 * 
 * **IMPORTANT** You should try to use the existing flashing utilities (`hackrf_spiflash`) to flash new firmware to the device! Incorrect usage of the SPIFLASH functions (especially @ref hackrf_spiflash_erase an @ref hackrf_spiflash_write) can brick the device, and DFU mode will be needed to unbrick it!
 * 
 * Firmware flashing can be achieved via writing to the SPI flash holding the firmware of the ARM microcontroller. This can be achieved by the `hackrf_spiflash_*` functions.
 * 
 * The Spartan II CPLD inside the HackRF One devices could also be reconfigured in the past, but in newer firmwares, the ARM MCU automatically reconfigures it on startup with a bitstream baked into the firmware image, thus the function @ref hackrf_cpld_write has no effect, and CPLD flashing can only be done by building a custom firmware (or the automatic loading can be disabled this way as well). The function @ref hackrf_cpld_write and the util `hackrf_cpldjtag` are **deprecated** and only kept for backward compatibility with older firmware versions.
 *  
 * # Debugging
 * 
 * The functions in this section can be used to directly read/write internal registers of the chips inside a HackRF One unit. See the page  <a href="https://hackrf.readthedocs.io/en/latest/hardware_components.html">Hardware Components</a> for more details on them.
 * 
 * Here's a brief introduction on the various chips in the HackRF One unit:
 * 
 * ## MAX2837 2.3 to 2.7 GHz transceiver
 * This transceiver chip is the RF modulator/demodulator of the HackRF One. This chip sends/receives analoge I/Q samples to/from the MAX5864 ADC/DAC chip. 
 * 
 * Its registers are accessible through the functions @ref hackrf_max2837_read and @ref hackrf_max2837_write
 * 
 * ## MAX5864 ADC/DAC
 * This chip converts received analgoe I/Q samples to digital and transmitted I/Q samples to analoge. It connects to the main ARM MCU through the CPLD. No configuration is needed for it, only the sample rate can be set via the clock generator IC.
 * 
 * ## Si5351C Clock generator
 * This chip supplies clock signals to all of the other chips. It can synthesize a wide range of frequencies from its clock inputs (internal or external). It uses a fixed 800-MHz internal clock (synthesized via a PLL).
 * 
 * Its registers are accessible through the functions @ref hackrf_si5351c_read and @ref hackrf_si5351c_write
 * 
 * ## RFFC5072 Synthesizer/mixer
 * This mixer mixes the RF signal with an internally synthesized local oscillator signal and thus results in the sum and difference frequencies. Combined with the LPF or HPF filters and the frequency setting in the MAX2837 IC it can be used to tune to any frequency in the 0-6000MHz range.
 * 
 * Its registers are accessible through the functions @ref hackrf_rffc5071_read and @ref hackrf_rffc5071_write
 * 
 * ## LPC4320 ARM MCU
 * This is the main processor of the unit. It's a multi-core ARM processor. It's configured to boot from a W25Q80B SPI flash, but can also be booted from DFU in order to unbrick a bricked unit. It communicated with the host PC via USB.
 * 
 * Some operation details are available via the function @ref hackrf_get_m0_state
 * 
 * ## W25Q80B SPI flash
 * 
 * This chip holds the firmware for the LPC4320 ARM MCU.
 * 
 * It's accessible through the functions @ref hackrf_spiflash_read, @ref hackrf_spiflash_write, @ref hackrf_spiflash_erase, @ref hackrf_spiflash_status and @ref hackrf_spiflash_clear_status
 * 
 * ## XC2C64A CPLD
 * This CPLD sits between the MAX5864 ADC/DAC and the main MCU, and mainly performs data format conversion and some synchronisation.
 * 
 * Its bitstream is auto-loaded on reset by the ARM MCU (from the firmware image), but in older versions, it was possible to reconfigure it via @ref hackrf_cpld_write, and the (since temporarly removed) `hackrf_cpld_checksum` function could verify the firmware in the configuration flash (again, overwritten on startup, so irrelevant).
 * 
 * See <a href="https://github.com/greatscottgadgets/hackrf/issues/609">issue 608</a>, <a href="https://github.com/greatscottgadgets/hackrf/issues/1140">issue 1140</a> and <a href="https://github.com/greatscottgadgets/hackrf/issues/1141">issue 1141</a> for some more details on this!
 */

/**
 * @defgroup operacake Opera Cake add-on board functions
 * 
 * Various functions related to the Opera Cake add-on boards.
 * 
 * These boards are versatile RF switching boards capable of switching two primary ports (A0 and B0) to any of 8 (A1-A4 and B1-B4) secondary ports (with the only rule that A0 and B0 can not be connected to the same side/bank of secondary ports at the same time).
 * 
 * There are 3 operating modes:
 * - manual setup
 * - frequency-based setup
 * - time-based setup
 * 
 * ### Manual setup
 * 
 * This mode allows A0 and B0 to be connected to any of the secondary ports. This mode is configured with @ref hackrf_set_operacake_ports. 
 * 
 * ### Frequency-based setup
 * 
 * In this mode the Opera Cake board automatically switches A0 to a port depending on the tuning frequency. Up to @ref HACKRF_OPERACAKE_MAX_FREQ_RANGES frequency ranges can be setup using @ref hackrf_set_operacake_freq_ranges, in a priority order. Port B0 mirrors A0 on the opposite side (but both B and A side ports can be specified for connections to A0)
 * 
 * ### Time-based setup
 * 
 * In this mode the Opera Cake board automatically switches A0 to a port for a set amount of time (specified in samples). Up to @ref HACKRF_OPERACAKE_MAX_DWELL_TIMES times can be setup via @ref hackrf_set_operacake_dwell_times. Port B0 mirrors A0 on the opposite side.
 * 
 * ## Opera Cake setup
 * 
 * Opera Cake boards can be listed with @ref hackrf_get_operacake_boards, but if only one board is connected, than using address 0 defaults to it.
 * 
 * Opera Cake mode can be setup via @ref hackrf_set_operacake_mode, then the corresponding configuration function can be called.
 * 
 * ## Multiple boards
 * 
 * There can be up to @ref HACKRF_OPERACAKE_MAX_BOARDS boards connected to a single HackRF One. They can be assigned individual addresses via onboard jumpers, see the [documentation page](https://hackrf.readthedocs.io/en/latest/opera_cake.html) for details.
 * **Note**: the operating modes of the boards can be set individually via @ref hackrf_set_operacake_mode, but in frequency or time mode, every board configured to that mode will use the same switching plan!
 * 
 * 
 */

/**
 * Number of samples per tuning when sweeping
 * @ingroup streaming
 */
#define SAMPLES_PER_BLOCK 8192

/**
 * Number of bytes per tuning for sweeping
 * @ingroup streaming
 */
#define BYTES_PER_BLOCK 16384

/**
 * Maximum number of sweep ranges to be specified for @ref hackrf_init_sweep
 * @ingroup streaming
 */
#define MAX_SWEEP_RANGES 10

/**
 * Invalid Opera Cake add-on board address, placeholder in @ref hackrf_get_operacake_boards
 * @ingroup operacake
 */
#define HACKRF_OPERACAKE_ADDRESS_INVALID 0xFF

/**
 * Maximum number of connected Opera Cake add-on boards
 * @ingroup operacake
 */
#define HACKRF_OPERACAKE_MAX_BOARDS 8

/**
 * Maximum number of specifiable dwell times for Opera Cake add-on boards
 * @ingroup operacake
 */
#define HACKRF_OPERACAKE_MAX_DWELL_TIMES 16

/**
 * Maximum number of specifiable frequency ranges for Opera Cake add-on boards
 * @ingroup operacake
 */
#define HACKRF_OPERACAKE_MAX_FREQ_RANGES 8

/**
 * error enum, returned by many libhackrf functions
 * 
 * Many functions that are specified to return INT are actually returning this enum
 * @ingroup library
 */
enum hackrf_error {
	/**
	 * no error happened
	 */
	HACKRF_SUCCESS = 0,
	/**
	 * TRUE value, returned by some functions that return boolean value. Only a few functions can return this variant, and this fact should be explicitly noted at those functions.
	 */
	HACKRF_TRUE = 1,
	/**
	 * The function was called with invalid parameters.
	 */
	HACKRF_ERROR_INVALID_PARAM = -2,
	/**
	 * USB device not found, returned at opening.
	 */
	HACKRF_ERROR_NOT_FOUND = -5,
	/**
	 * Resource is busy, possibly the device is already opened.
	 */
	HACKRF_ERROR_BUSY = -6,
	/**
	 * Memory allocation (on host side) failed
	 */
	HACKRF_ERROR_NO_MEM = -11,
	/**
	 * LibUSB error, use @ref hackrf_error_name to get a human-readable error string (using `libusb_strerror`)
	 */
	HACKRF_ERROR_LIBUSB = -1000,
	/**
	 * Error setting up transfer thread (pthread-related error)
	 */
	HACKRF_ERROR_THREAD = -1001,
	/**
	 * Streaming thread could not start due to an error
	 */
	HACKRF_ERROR_STREAMING_THREAD_ERR = -1002,
	/**
	 * Streaming thread stopped due to an error
	 */
	HACKRF_ERROR_STREAMING_STOPPED = -1003,
	/**
	 * Streaming thread exited (normally)
	 */
	HACKRF_ERROR_STREAMING_EXIT_CALLED = -1004,
	/**
	 * The installed firmware does not support this function
	 */
	HACKRF_ERROR_USB_API_VERSION = -1005,
	/**
	 * Can not exit library as one or more HackRFs still in use
	 */
	HACKRF_ERROR_NOT_LAST_DEVICE = -2000,
	/**
	 * Unspecified error
	 */
	HACKRF_ERROR_OTHER = -9999,
};

/**
 * Made by GSG bit in @ref hackrf_board_rev enum and in platform ID
 * @ingroup device
 */
#define HACKRF_BOARD_REV_GSG (0x80)

/**
 * JAWBREAKER platform bit in result of @ref hackrf_supported_platform_read
 * @ingroup device
 */
#define HACKRF_PLATFORM_JAWBREAKER (1 << 0)
/**
 * HACKRF ONE (pre r9) platform bit in result of @ref hackrf_supported_platform_read
 * @ingroup device
 */
#define HACKRF_PLATFORM_HACKRF1_OG (1 << 1)
/**
 * RAD1O platform bit in result of @ref hackrf_supported_platform_read
 * @ingroup device
 */
#define HACKRF_PLATFORM_RAD1O (1 << 2)
/**
 * HACKRF ONE (r9 or later) platform bit in result of @ref hackrf_supported_platform_read
 * @ingroup device
 */
#define HACKRF_PLATFORM_HACKRF1_R9 (1 << 3)

/**
 * HACKRF board id enum
 * 
 * Returned by @ref hackrf_board_id_read and can be converted to a human-readable string using @ref hackrf_board_id_name
 * @ingroup device
 */
enum hackrf_board_id {
	/**
	 * Jellybean (pre-production revision, not supported)
	 */
	BOARD_ID_JELLYBEAN = 0,
	/**
	 * Jawbreaker (beta platform, 10-6000MHz, no bias-tee)
	 */
	BOARD_ID_JAWBREAKER = 1,
	/**
	 * HackRF One (prior to rev 9, same limits: 1-6000MHz, 20MSPS, bias-tee)
	 */
	BOARD_ID_HACKRF1_OG = 2,
	/**
	 * RAD1O (Chaos Computer Club special edition with LCD & other features. 50M-4000MHz, 20MSPS, no bias-tee)
	 */
	BOARD_ID_RAD1O = 3,
	/**
	 * HackRF One (rev. 9 & later. 1-6000MHz, 20MSPS, bias-tee)
	 */
	BOARD_ID_HACKRF1_R9 = 4,
	/**
	 * Unknown board (failed detection)
	 */
	BOARD_ID_UNRECOGNIZED = 0xFE,
	/**
	 * Unknown board (detection not yet attempted, should be default value)
	 */
	BOARD_ID_UNDETECTED = 0xFF,
};

/**
 * These deprecated board ID names are provided for API compatibility.
 * @ingroup device
 */
#define BOARD_ID_HACKRF_ONE (BOARD_ID_HACKRF1_OG)
/**
 * These deprecated board ID names are provided for API compatibility.
 * @ingroup device
 */
#define BOARD_ID_INVALID (BOARD_ID_UNDETECTED)

/**
 * Board revision enum. 
 * 
 * Returned by @ref hackrf_board_rev_read and can be converted into human-readable name by @ref hackrf_board_rev_name. MSB (`board_rev & HACKRF_BOARD_REV_GSG`) should signify if the board was built by GSG or not. @ref hackrf_board_rev_name ignores this information.
 * @ingroup device
 */
enum hackrf_board_rev {
	/**
	 * Older than rev6
	 */
	BOARD_REV_HACKRF1_OLD = 0,
	/**
	 * board revision 6, generic
	 */
	BOARD_REV_HACKRF1_R6 = 1,
	/**
	 * board revision 7, generic
	 */
	BOARD_REV_HACKRF1_R7 = 2,
	/**
	 * board revision 8, generic
	 */
	BOARD_REV_HACKRF1_R8 = 3,
	/**
	 * board revision 9, generic
	 */
	BOARD_REV_HACKRF1_R9 = 4,
	/**
	 * board revision 10, generic
	 */
	BOARD_REV_HACKRF1_R10 = 5,

	/**
	 * board revision 6, made by GSG
	 */
	BOARD_REV_GSG_HACKRF1_R6 = 0x81,
	/**
	 * board revision 7, made by GSG
	 */
	BOARD_REV_GSG_HACKRF1_R7 = 0x82,
	/**
	 * board revision 8, made by GSG
	 */
	BOARD_REV_GSG_HACKRF1_R8 = 0x83,
	/**
	 * board revision 9, made by GSG
	 */
	BOARD_REV_GSG_HACKRF1_R9 = 0x84,
	/**
	 * board revision 10, made by GSG
	 */
	BOARD_REV_GSG_HACKRF1_R10 = 0x85,

	/**
	 * unknown board revision (detection failed)
	 */
	BOARD_REV_UNRECOGNIZED = 0xFE,
	/**
	 * unknown board revision (detection not yet attempted)
	 */
	BOARD_REV_UNDETECTED = 0xFF,
};

/**
 * USB board ID (product ID) enum
 * 
 * Contains USB-IF product id (field `idProduct` in `libusb_device_descriptor`). Can be used to identify general type of hardware.
 * Only used in @ref hackrf_device_list.usb_board_ids field of @ref hackrf_device_list, and can be converted into human-readable string via @ref hackrf_usb_board_id_name.
 * @ingroup device
 */
enum hackrf_usb_board_id {
	/**
	 * Jawbreaker (beta platform) USB product id
	 */
	USB_BOARD_ID_JAWBREAKER = 0x604B,
	/**
	 * HackRF One USB product id
	 */
	USB_BOARD_ID_HACKRF_ONE = 0x6089,
	/**
	 * RAD1O (custom version) USB product id
	 */
	USB_BOARD_ID_RAD1O = 0xCC15,
	/**
	 * Invalid / unknown USB product id
	 */
	USB_BOARD_ID_INVALID = 0xFFFF,
};

/**
 * RF filter path setting enum
 * 
 * Used only when performing explicit tuning using @ref hackrf_set_freq_explicit, or can be converted into a human readable string using @ref hackrf_filter_path_name.
 * This can select the image rejection filter (U3, U8 or none) to use - using switches U5, U6, U9 and U11. When no filter is selected, the mixer itself is bypassed.
 * @ingroup configuration
 */
enum rf_path_filter {
	/**
	 * No filter is selected, **the mixer is bypassed**, \f$f_{center} = f_{IF}\f$
	 */
	RF_PATH_FILTER_BYPASS = 0,
	/**
	 * LPF is selected, \f$f_{center} = f_{IF} - f_{LO}\f$
	 */
	RF_PATH_FILTER_LOW_PASS = 1,
	/**
	 * HPF is selected, \f$f_{center} = f_{IF} + f_{LO}\f$
	 */
	RF_PATH_FILTER_HIGH_PASS = 2,
};

/**
 * Opera Cake secondary ports (A1-A4, B1-B4)
 * @ingroup operacake
 */
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

/**
 * Opera Cake port switching mode. Set via @ref hackrf_set_operacake_mode and quaried via @ref hackrf_get_operacake_mode
 * @ingroup operacake
 */
enum operacake_switching_mode {
	/**
	 * Port connections are set manually using @ref hackrf_set_operacake_ports. Both ports can be specified, but not on the same side.
	 */
	OPERACAKE_MODE_MANUAL,
	/**
	 * Port connections are switched automatically when the frequency is changed. Frequency ranges can be set using @ref hackrf_set_operacake_freq_ranges. In this mode, B0 mirrors A0
	 */
	OPERACAKE_MODE_FREQUENCY,
	/**
	 * Port connections are switched automatically over time. dwell times can be set with @ref hackrf_set_operacake_dwell_times. In this mode, B0 mirrors A0
	 */
	OPERACAKE_MODE_TIME,
};

/**
 * sweep mode enum
 * 
 * Used by @ref hackrf_init_sweep, to set sweep parameters.
 * 
 * Linear mode is no longer used by the hackrf_sweep command line tool and in general the interleaved method is always preferable, but the linear mode remains available for backward compatibility and might be useful in some special circumstances.
 * @ingroup streaming
 */
enum sweep_style {
	/**
	 * step_width is added to the current frequency at each step.
	 */
	LINEAR = 0,
	/**
	 * each step is divided into two interleaved sub-steps, allowing the host to select the best portions of the FFT of each sub-step and discard the rest.
	 */
	INTERLEAVED = 1,
};

/**
 * Opaque struct for hackrf device info. Object can be created via @ref hackrf_open, @ref hackrf_device_list_open or @ref hackrf_open_by_serial and be destroyed via @ref hackrf_close
 * @ingroup device
 */
typedef struct hackrf_device hackrf_device;

/**
 * USB transfer information passed to RX or TX callback.
 * A callback should treat all these fields as read-only except that a TX
 * callback should write to the data buffer and may write to valid_length to
 * indicate that a smaller number of bytes is to be transmitted.
 * @ingroup streaming
 */
typedef struct {
	/** HackRF USB device for this transfer */
	hackrf_device* device;
	/** transfer data buffer (interleaved 8 bit I/Q samples) */
	uint8_t* buffer;
	/** length of data buffer in bytes */
	int buffer_length;
	/** number of buffer bytes that were transferred */
	int valid_length;
	/** User provided RX context. Not used by the library, but available to transfer callbacks for use. Set along with the transfer callback using @ref hackrf_start_rx or @ref hackrf_start_rx_sweep */
	void* rx_ctx;
	/** User provided TX context. Not used by the library, but available to transfer callbacks for use. Set along with the transfer callback using @ref hackrf_start_tx*/
	void* tx_ctx;
} hackrf_transfer;

/**
 * @ingroup device
 * MCU (LPC43xx) part ID and serial number. See the documentation of the MCU for details! Read via @ref hackrf_board_partid_serialno_read
 */
typedef struct {
	/**
	 * MCU part ID register value
	*/
	uint32_t part_id[2];
	/**
	 * MCU device unique ID (serial number)
	*/
	uint32_t serial_no[4];
} read_partid_serialno_t;

/**
 * Opera Cake port setting in @ref OPERACAKE_MODE_TIME operation
 * @ingroup operacake
 */
typedef struct {
	/**
	 * Dwell time for port (in number of samples) 
	 */
	uint32_t dwell;
	/**
	 * Port to connect A0 to (B0 mirrors this choice) Must be one of @ref operacake_ports
	 */
	uint8_t port;
} hackrf_operacake_dwell_time;

/**
 * Opera Cake port setting in @ref OPERACAKE_MODE_FREQUENCY operation
 * @ingroup operacake
 */
typedef struct {
	/**
	 * Start frequency (in MHz)
	 */
	uint16_t freq_min;
	/**
	 * Stop frequency (in MHz)
	 */
	uint16_t freq_max;
	/**
	 * Port (A0) to use for that frequency range. Port B0 mirrors this. Must be one of @ref operacake_ports
	 */
	uint8_t port;
} hackrf_operacake_freq_range;

/** 
 * Helper struct for hackrf_bias_t_user_setting.  If 'do_update' is true, then the values of 'change_on_mode_entry'
 * and 'enabled' will be used as the new default.  If 'do_update' is false, the current default will not change.
*/
typedef struct {
	bool do_update;
	bool change_on_mode_entry;
	bool enabled;
} hackrf_bool_user_settting;

/** 
 * User settings for user-supplied bias tee defaults.
 * 
 * @ingroup device
 * 
*/
typedef struct {
	hackrf_bool_user_settting tx;
	hackrf_bool_user_settting rx;
	hackrf_bool_user_settting off;
} hackrf_bias_t_user_settting_req;

/** 
 * State of the SGPIO loop running on the M0 core. 
 * @ingroup debug
 */
typedef struct {
	/** Requested mode. Possible values are 0(IDLE), 1(WAIT), 2(RX), 3(TX_START), 4(TX_RUN)*/
	uint16_t requested_mode;
	/** Request flag, 0 means request is completed, any other value means request is pending.*/
	uint16_t request_flag;
	/** Active mode. Possible values are the same as in @ref hackrf_m0_state.requested_mode */
	uint32_t active_mode;
	/** Number of bytes transferred by the M0. */
	uint32_t m0_count;
	/** Number of bytes transferred by the M4. */
	uint32_t m4_count;
	/** Number of shortfalls. */
	uint32_t num_shortfalls;
	/** Longest shortfall in bytes. */
	uint32_t longest_shortfall;
	/** Shortfall limit in bytes. */
	uint32_t shortfall_limit;
	/** Threshold m0_count value (in bytes) for next mode change. */
	uint32_t threshold;
	/** Mode which will be switched to when threshold is reached. Possible values are the same as in @ref hackrf_m0_state.requested_mode */
	uint32_t next_mode;
	/** Error, if any, that caused the M0 to revert to IDLE mode. Possible values are 0 (NONE), 1 (RX_TIMEOUT) and 2(TX_TIMEOUT)*/
	uint32_t error;
} hackrf_m0_state;

/**
 * List of connected HackRF devices
 * 
 * Acquired via @ref hackrf_device_list and should be freeed via @ref hackrf_device_list_free. Individual devices can be opened via @ref hackrf_device_list_open
 * @ingroup device
 */
struct hackrf_device_list {
	/**
	 * Array of human-readable serial numbers. Each entry can be NULL!
	 */
	char** serial_numbers;
	/**
	 * ID of each board, based on USB product ID. Can be used for general HW identification without opening the device.
	 */
	enum hackrf_usb_board_id* usb_board_ids;
	/**
	 * USB device index for a given HW entry. Intended for internal use only.
	 */
	int* usb_device_index;

	/**
	 * Number of connected HackRF devices, the length of arrays @ref serial_numbers, @ref usb_board_ids and @ref usb_device_index.
	 */
	int devicecount;

	/**
	 * All USB devices (as `libusb_device**` array)
	 */
	void** usb_devices;
	/**
	 * Number of all queried USB devices. Length of array @ref usb_devices.
	 */
	int usb_devicecount;
};

typedef struct hackrf_device_list hackrf_device_list_t;

/**
 * Sample block callback, used in RX and TX (set via @ref hackrf_start_rx, @ref hackrf_start_rx_sweep and @ref hackrf_start_tx). In each mode, it is called when data needs to be handled, meaning filling samples in TX mode or reading them in RX modes.
 * 
 * In TX mode, it should refill the transfer buffer with new raw IQ data, and set @ref hackrf_transfer.valid_length.
 * 
 * In RX mode, it should copy/process the contents of the transfer buffer's valid part.
 * 
 * In RX SWEEP mode, it receives multiple "blocks" of data, each with a 10-byte header containing the tuned frequency followed by the samples. See @ref hackrf_init_sweep for more info.
 * 
 * The callback should return 0 if it wants to be called again, and any other value otherwise. Stopping the RX/TX/SWEEP is still done with @ref hackrf_stop_rx and @ref hackrf_stop_tx, and those should be called from the main thread, so this callback should signal the main thread that it should stop. Signaling the main thread to stop TX should be done from the flush callback in order to guarantee that no samples are discarded, see @ref hackrf_flush_cb_fn
 * @ingroup streaming
 */
typedef int (*hackrf_sample_block_cb_fn)(hackrf_transfer* transfer);

/**
 * Block complete callback. 
 * 
 * Set via @ref hackrf_set_tx_block_complete_callback, called when a transfer is finished to the device's buffer, regardless if the transfer was successful or not. It can signal the main thread to stop on failure, can catch USB transfer errors and can also gather statistics about the transfered data.
 * @ingroup streaming
 */
typedef void (*hackrf_tx_block_complete_cb_fn)(hackrf_transfer* transfer, int);

/**
 * Flush (end of transmission) callback
 * 
 * Will be called when the last samples are transmitted and stopping transmission will result in no samples getting lost. Should signal the main thread that it should stop transmission via @ref hackrf_stop_tx
 * @ingroup streaming
 */
typedef void (*hackrf_flush_cb_fn)(void* flush_ctx, int);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize libhackrf
 * 
 * Should be called before any other libhackrf function. Initializes libusb. Can be safely called multiple times.
 * @return @ref HACKRF_SUCCESS on success or @ref HACKRF_ERROR_LIBUSB
 * @ingroup library
 */
extern ADDAPI int ADDCALL hackrf_init();

/**
 * Exit libhackrf
 * 
 * Should be called before exit. No other libhackrf functions should be called after it. Can be safely called multiple times.
 * @return @ref HACKRF_SUCCESS on success or @ref HACKRF_ERROR_NOT_LAST_DEVICE if not all devices were closed properly.
 * @ingroup library
 */
extern ADDAPI int ADDCALL hackrf_exit();

/**
 * Get library version.
 * 
 * Can be called before @ref hackrf_init
 * @return library version as a human-readable string
 * @ingroup library
 */
extern ADDAPI const char* ADDCALL hackrf_library_version();

/**
 * Get library release.
 * 
 * Can be called before @ref hackrf_init
 * @return library version as a human-readable string. 
 * @ingroup library
 */
extern ADDAPI const char* ADDCALL hackrf_library_release();

/**
 * List connected HackRF devices
 * @return list of connected devices. The list should be freed with @ref hackrf_device_list_free
 * @ingroup device
 */
extern ADDAPI hackrf_device_list_t* ADDCALL hackrf_device_list();

/**
 * Open a @ref hackrf_device from a device list
 * @param[in] list device list to open device from
 * @param[in] idx index of the device to open
 * @param[out] device device handle to open
 * @return @ref HACKRF_SUCCESS on success, @ref HACKRF_ERROR_INVALID_PARAM on invalid parameters or other @ref hackrf_error variant
 * @ingroup device
 */
extern ADDAPI int ADDCALL hackrf_device_list_open(
	hackrf_device_list_t* list,
	int idx,
	hackrf_device** device);

/**
 * Free a previously allocated @ref hackrf_device_list list.
 * @param[in] list list to free
 * @ingroup device
 */
extern ADDAPI void ADDCALL hackrf_device_list_free(hackrf_device_list_t* list);

/**
 * Open first available HackRF device
 * @param[out] device device handle
 * @return @ref HACKRF_SUCCESS on success, @ref HACKRF_ERROR_INVALID_PARAM if @p device is NULL, @ref HACKRF_ERROR_NOT_FOUND if no HackRF devices are found or other @ref hackrf_error variant
 * @ingroup device
 */
extern ADDAPI int ADDCALL hackrf_open(hackrf_device** device);

/**
 * Open HackRF device by serial number
 * @param[in] desired_serial_number serial number of device to open. If NULL then default to first device found.
 * @param[out] device device handle
 * @return @ref HACKRF_SUCCESS on success, @ref HACKRF_ERROR_INVALID_PARAM if @p device is NULL, @ref HACKRF_ERROR_NOT_FOUND if no HackRF devices are found or other @ref hackrf_error variant
 * @ingroup device
 */
extern ADDAPI int ADDCALL hackrf_open_by_serial(
	const char* const desired_serial_number,
	hackrf_device** device);

/**
 * Close a previously opened device
 * @param[in] device device to close
 * @return @ref HACKRF_SUCCESS on success or variant of @ref hackrf_error
 * @ingroup device
 */
extern ADDAPI int ADDCALL hackrf_close(hackrf_device* device);

/**
 * Start receiving
 * 
 * Should be called after setting gains, frequency and sampling rate, as these values won't get reset but instead keep their last value, thus their state is unknown.
 * 
 * The callback is called with a @ref hackrf_transfer object whenever the buffer is full. The callback is called in an async context so no libhackrf functions should be called from it. The callback should treat its argument as read-only.
 * @param device device to configure
 * @param callback rx_callback
 * @param rx_ctx User provided RX context. Not used by the library, but available to @p callback as @ref hackrf_transfer.rx_ctx.
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup streaming
 */
extern ADDAPI int ADDCALL hackrf_start_rx(
	hackrf_device* device,
	hackrf_sample_block_cb_fn callback,
	void* rx_ctx);

/**
 * Stop receiving
 * 
 * @param device device to stop RX on
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup streaming
 */
extern ADDAPI int ADDCALL hackrf_stop_rx(hackrf_device* device);

/**
 * Start transmitting
 * 
 *
 * Should be called after setting gains, frequency and sampling rate, as these values won't get reset but instead keep their last value, thus their state is unknown. 
 * Setting flush function (using @ref hackrf_enable_tx_flush) and/or setting block complete callback (using @ref hackrf_set_tx_block_complete_callback) (if these features are used) should also be done before this.
 * 
 * The callback is called with a @ref hackrf_transfer object whenever a transfer buffer is needed to be filled with samples. The callback is called in an async context so no libhackrf functions should be called from it. The callback should treat its argument as read-only, except the @ref hackrf_transfer.buffer and @ref hackrf_transfer.valid_length.
 * @param device device to configure
 * @param callback tx_callback
 * @param tx_ctx User provided TX context. Not used by the library, but available to @p callback as @ref hackrf_transfer.tx_ctx.
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup streaming
 */
extern ADDAPI int ADDCALL hackrf_start_tx(
	hackrf_device* device,
	hackrf_sample_block_cb_fn callback,
	void* tx_ctx);

/**
 * Setup callback to be called when an USB transfer is completed.
 * 
 * This callback will be called whenever an USB transfer to the device is completed, regardless if it was successful or not (indicated by the second parameter).
 * 
 * @param device device to configure
 * @param callback callback to call when a transfer is completed
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant 
 * @ingroup streaming
 */
extern ADDAPI int ADDCALL hackrf_set_tx_block_complete_callback(
	hackrf_device* device,
	hackrf_tx_block_complete_cb_fn callback);

/**
 * Setup flush (end-of-transmission) callback
 * 
 * This callback will be called when all the data was transmitted and all data transfers were completed. First parameter is supplied context, second parameter is success flag.
 * 
 * @param device device to configure
 * @param callback callback to call when all transfers were completed
 * @param flush_ctx context (1st parameter of callback)
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant 
 * @ingroup streaming
 */
extern ADDAPI int ADDCALL hackrf_enable_tx_flush(
	hackrf_device* device,
	hackrf_flush_cb_fn callback,
	void* flush_ctx);

/**
 * Stop transmission
 * 
 * @param device device to stop TX on
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant 
 * @ingroup streaming
 */
extern ADDAPI int ADDCALL hackrf_stop_tx(hackrf_device* device);

/**
 * Get the state of the M0 code on the LPC43xx MCU
 * 
 * Requires USB API version 0x0106 or above!
 * @param[in] device device to query
 * @param[out] value MCU code state
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant  
 * @ingroup debug
 */
extern ADDAPI int ADDCALL hackrf_get_m0_state(
	hackrf_device* device,
	hackrf_m0_state* value);

/**
 * Set transmit underrun limit
 * 
 * When this limit is set, after the specified number of samples (bytes, not whole IQ pairs) missing the device will automatically return to IDLE mode, thus stopping operation. Useful for handling cases like program/computer crashes or other problems. The default value 0 means no limit.
 * 
 * Requires USB API version 0x0106 or above!
 * @param device device to configure
 * @param value number of samples to wait before auto-stopping
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant   
 * @ingroup streaming
 */
extern ADDAPI int ADDCALL hackrf_set_tx_underrun_limit(
	hackrf_device* device,
	uint32_t value);

/**
 * Set receive overrun limit
 * 
 * When this limit is set, after the specified number of samples (bytes, not whole IQ pairs) missing the device will automatically return to IDLE mode, thus stopping operation. Useful for handling cases like program/computer crashes or other problems. The default value 0 means no limit.
 * 
 * Requires USB API version 0x0106 or above!
 * @param device device to configure
 * @param value number of samples to wait before auto-stopping
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup streaming
 */
extern ADDAPI int ADDCALL hackrf_set_rx_overrun_limit(
	hackrf_device* device,
	uint32_t value);

/**
 * Query device streaming status
 * 
 * @param device device to query
 * @return @ref HACKRF_TRUE if the device is streaming, else one of @ref HACKRF_ERROR_STREAMING_THREAD_ERR, @ref HACKRF_ERROR_STREAMING_STOPPED or @ref HACKRF_ERROR_STREAMING_EXIT_CALLED
 * @ingroup streaming
 */
extern ADDAPI int ADDCALL hackrf_is_streaming(hackrf_device* device);

/**
 * Directly read the registers of the MAX2837 transceiver IC
 * 
 * Intended for debugging purposes only!
 * 
 * @param[in] device device to query
 * @param[in] register_number register number to read
 * @param[out] value value of the specified register
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup debug
 */
extern ADDAPI int ADDCALL hackrf_max2837_read(
	hackrf_device* device,
	uint8_t register_number,
	uint16_t* value);

/**
 * Directly write the registers of the MAX2837 transceiver IC
 * 
 * Intended for debugging purposes only!
 * 
 * @param device device to write
 * @param register_number register number to write
 * @param value value to write in the specified register
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup debug
 */
extern ADDAPI int ADDCALL hackrf_max2837_write(
	hackrf_device* device,
	uint8_t register_number,
	uint16_t value);

/**
 * Directly read the registers of the Si5351C clock generator IC
 * 
 * Intended for debugging purposes only!
 * 
 * @param[in] device device to query
 * @param[in] register_number register number to read
 * @param[out] value value of the specified register
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup debug
 */
extern ADDAPI int ADDCALL hackrf_si5351c_read(
	hackrf_device* device,
	uint16_t register_number,
	uint16_t* value);

/**
 * Directly write the registers of the Si5351 clock generator IC
 * 
 * Intended for debugging purposes only!
 * 
 * @param[in] device device to write
 * @param[in] register_number register number to write
 * @param[out] value value to write in the specified register
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup debug
 */
extern ADDAPI int ADDCALL hackrf_si5351c_write(
	hackrf_device* device,
	uint16_t register_number,
	uint16_t value);

/**
 * Set baseband filter bandwidth
 * 
 * Possible values: 1.75, 2.5, 3.5, 5, 5.5, 6, 7, 8, 9, 10, 12, 14, 15, 20, 24, 28MHz, default \f$ \le 0.75 \cdot F_s \f$
 * The functions @ref hackrf_compute_baseband_filter_bw and @ref hackrf_compute_baseband_filter_bw_round_down_lt can be used to get a valid value nearest to a given value.
 * 
 * Setting the sample rate causes the filter bandwidth to be (re)set to its default \f$ \le 0.75 \cdot F_s \f$ value, so setting sample rate should be done before setting filter bandwidth.
 * 
 * @param device device to configure
 * @param bandwidth_hz baseband filter bandwidth in Hz
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup configuration
 */
extern ADDAPI int ADDCALL hackrf_set_baseband_filter_bandwidth(
	hackrf_device* device,
	const uint32_t bandwidth_hz);

/**
 * Directly read the registers of the RFFC5071/5072 mixer-synthesizer IC
 * 
 * Intended for debugging purposes only!
 * 
 * @param[in] device device to query
 * @param[in] register_number register number to read
 * @param[out] value value of the specified register
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup debug
 */
extern ADDAPI int ADDCALL hackrf_rffc5071_read(
	hackrf_device* device,
	uint8_t register_number,
	uint16_t* value);

/**
 * Directly write the registers of the RFFC5071/5072 mixer-synthesizer IC
 * 
 * Intended for debugging purposes only!
 * 
 * @param[in] device device to write
 * @param[in] register_number register number to write
 * @param[out] value value to write in the specified register
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup debug
 */
extern ADDAPI int ADDCALL hackrf_rffc5071_write(
	hackrf_device* device,
	uint8_t register_number,
	uint16_t value);

/**
 * Erase firmware image on the SPI flash
 * 
 * Should be followed by writing a new image, or the HackRF will be soft-bricked (still rescuable in DFU mode)
 * 
 * @param device device to ersase
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup debug
 */
extern ADDAPI int ADDCALL hackrf_spiflash_erase(hackrf_device* device);

/**
 * Write firmware image on the SPI flash
 * 
 * Should only be used for firmware updating. Can brick the device, but it's still rescuable in DFU mode.
 * 
 * @param device device to write on
 * @param address address to write to. Should start at 0
 * @param length length of data to write. Must be at most 256. 
 * @param data data to write
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup debug
 */
extern ADDAPI int ADDCALL hackrf_spiflash_write(
	hackrf_device* device,
	const uint32_t address,
	const uint16_t length,
	unsigned char* const data);

/**
 * Read firmware image on the SPI flash
 * 
 * Should only be used for firmware verification.
 * 
 * @param device device to read from
 * @param address address to read from. Firmware should start at 0
 * @param length length of data to read. Must be at most 256. 
 * @param data pointer to buffer
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup debug
 */
extern ADDAPI int ADDCALL hackrf_spiflash_read(
	hackrf_device* device,
	const uint32_t address,
	const uint16_t length,
	unsigned char* data);

/**
 * Read the status registers of the W25Q80BV SPI flash chip
 * 
 * See the datasheet for details of the status registers. The two registers are read in order.
 * 
 * Requires USB API version 0x0103 or above!
 * @param[in] device device to query
 * @param[out] data char[2] array of the status registers
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup debug
 */
extern ADDAPI int ADDCALL hackrf_spiflash_status(hackrf_device* device, uint8_t* data);

/**
 * Clear the status registers of the W25Q80BV SPI flash chip
 * 
 * See the datasheet for details of the status registers.
 * 
 * Requires USB API version 0x0103 or above!
 * @param device device to clear
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup debug
 */
extern ADDAPI int ADDCALL hackrf_spiflash_clear_status(hackrf_device* device);

/* device will need to be reset after hackrf_cpld_write */

/**
 * Write configuration bitstream into the XC2C64A-7VQ100C CPLD
 * 
 * @deprecated this function writes the bitstream, but the firmware auto-overrides at each reset, so no changes will take effect
 * @param device device to configure
 * @param data CPLD bitstream data
 * @param total_length length of the bitstream to write
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup debug
 */
extern ADDAPI int ADDCALL hackrf_cpld_write(
	hackrf_device* device,
	unsigned char* const data,
	const unsigned int total_length);

/**
 * Read @ref hackrf_board_id from a device
 * 
 * The result can be converted into a human-readable string via @ref hackrf_board_id_name
 * 
 * 
 * @param[in] device device to query
 * @param[out] value @ref hackrf_board_id enum value
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup device
 */
extern ADDAPI int ADDCALL hackrf_board_id_read(hackrf_device* device, uint8_t* value);

/**
 * Read HackRF firmware version as a string
 * 
 * 
 * @param[in] device device to query
 * @param[out] version version string
 * @param[in] length length of allocated string **without null byte** (so set it to `length(arr)-1`)
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup device
 */
extern ADDAPI int ADDCALL hackrf_version_string_read(
	hackrf_device* device,
	char* version,
	uint8_t length);

/**
 * Read HackRF USB API version
 * 
 * Read version as MM.mm 16-bit value, where MM is the major and mm is the minor version, encoded as the hex digits of the 16-bit number. 
 * 
 * Example code from `hackrf_info.c` displaying the result:
 * ```c
 * printf("Firmware Version: %s (API:%x.%02x)\n",
 *             version,
 *             (usb_version >> 8) & 0xFF,
 *             usb_version & 0xFF);
 * ```
 * 
 * @param[in] device device to query
 * @param[out] version USB API version
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup device
 */
extern ADDAPI int ADDCALL hackrf_usb_api_version_read(
	hackrf_device* device,
	uint16_t* version);

/**
 * Set the center frequency
 * 
 * Simple (auto) tuning via specifying a center frequency in Hz
 * 
 * This setting is not exact and depends on the PLL settings. Exact resolution is not determined, but the actual tuned frequency will be quariable in the future.
 * 
 * @param device device to tune
 * @param freq_hz center frequency in Hz. Defaults to 900MHz. Should be in range 1-6000MHz, but 0-7250MHz is possible. The resolution is ~50Hz, I could not find the exact number.
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup configuration
 */
extern ADDAPI int ADDCALL hackrf_set_freq(hackrf_device* device, const uint64_t freq_hz);

/**
 * Set the center frequency via explicit tuning
 * 
 * Center frequency is set to \f$f_{center} = f_{IF} + k\cdot f_{LO}\f$ where \f$k\in\left\{-1; 0; 1\right\}\f$, depending on the value of @p path. See the documentation of @ref rf_path_filter for details
 * 
 * @param device device to tune
 * @param if_freq_hz tuning frequency of the MAX2837 transceiver IC in Hz. Must be in the range of 2150-2750MHz
 * @param lo_freq_hz tuning frequency of the RFFC5072 mixer/synthesizer IC in Hz. Must be in the range 84.375-5400MHz, defaults to 1000MHz. No effect if @p path is set to @ref RF_PATH_FILTER_BYPASS
 * @param path filter path for mixer. See the documentation for @ref rf_path_filter for details
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup configuration
 */
extern ADDAPI int ADDCALL hackrf_set_freq_explicit(
	hackrf_device* device,
	const uint64_t if_freq_hz,
	const uint64_t lo_freq_hz,
	const enum rf_path_filter path);

/**
 * Set sample rate explicitly
 * 
 * Sample rate should be in the range 2-20MHz, with the default being 10MHz. Lower & higher values are technically possible, but the performance is not guaranteed.
 * 
 * This function sets the sample rate by specifying a clock frequency in Hz and a divider, so the resulting sample rate will be @p freq_hz / @p divider.
 * This function also sets the baseband filter bandwidth to a value \f$ \le 0.75 \cdot F_s \f$, so any calls to @ref hackrf_set_baseband_filter_bandwidth should only be made after this.
 * 
 * @param device device to configure
 * @param freq_hz sample rate base frequency in Hz
 * @param divider frequency divider. Must be in the range 1-31
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup configuration
 */
extern ADDAPI int ADDCALL hackrf_set_sample_rate_manual(
	hackrf_device* device,
	const uint32_t freq_hz,
	const uint32_t divider);

/**
 * Set sample rate
 * 
 * Sample rate should be in the range 2-20MHz, with the default being 10MHz. Lower & higher values are technically possible, but the performance is not guaranteed.
 * This function also sets the baseband filter bandwidth to a value \f$ \le 0.75 \cdot F_s \f$, so any calls to @ref hackrf_set_baseband_filter_bandwidth should only be made after this.
 * 
 * @param device device to configure
 * @param freq_hz sample rate frequency in Hz. Should be in the range 2-20MHz
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup configuration
 */
extern ADDAPI int ADDCALL hackrf_set_sample_rate(
	hackrf_device* device,
	const double freq_hz);

/**
 * Enable/disable 14dB RF amplifier
 * 
 * Enable / disable the ~11dB RF RX/TX amplifiers U13/U25 via controlling switches U9 and U14.
 * 
 * @param device device to configure
 * @param value enable (1) or disable (0) amplifier
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup configuration
 */
extern ADDAPI int ADDCALL hackrf_set_amp_enable(
	hackrf_device* device,
	const uint8_t value);

/**
 * Read board part ID and serial number
 * 
 * Read MCU part id and serial number. See the documentation of the MCU for details!
 * 
 * @param[in] device device to query
 * @param[out] read_partid_serialno result of query
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup device
 */
extern ADDAPI int ADDCALL hackrf_board_partid_serialno_read(
	hackrf_device* device,
	read_partid_serialno_t* read_partid_serialno);

/**
 * Set LNA gain
 * 
 * Set the RF RX gain of the MAX2837 transceiver IC ("IF" gain setting) in decibels. Must be in range 0-40dB, with 8dB steps.
 * 
 * @param device device to configure
 * @param value RX IF gain value in dB
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup configuration
 */
extern ADDAPI int ADDCALL hackrf_set_lna_gain(hackrf_device* device, uint32_t value);

/**
 * Set baseband RX gain of the MAX2837 transceier IC ("BB" or "VGA" gain setting) in decibels. Must be in range 0-62dB with 2dB steps.
 * 
 * @param device device to configure
 * @param value RX BB gain value in dB
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup configuration
 */
extern ADDAPI int ADDCALL hackrf_set_vga_gain(hackrf_device* device, uint32_t value);

/**
 * Set RF TX gain of the MAX2837 transceiver IC ("IF" or "VGA" gain setting) in decibels. Must be in range 0-47dB in 1dB steps.
 * 
 * @param device device to configure
 * @param value TX IF gain value in dB
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup configuration
 */
extern ADDAPI int ADDCALL hackrf_set_txvga_gain(hackrf_device* device, uint32_t value);

/**
 * Enable / disable bias-tee (antenna port power)
 * 
 * Enable or disable the **3.3V (max 50mA)** bias-tee (antenna port power). Defaults to disabled.
 * 
 * **NOTE:** the firmware auto-disables this after returning to IDLE mode, so a perma-set is not possible, which means all software supporting HackRF devices must support enabling bias-tee, as setting it externally is not possible like it is with RTL-SDR for example.
 * 
 * @param device device to configure
 * @param value enable (1) or disable (0) bias-tee
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup configuration
 */
extern ADDAPI int ADDCALL hackrf_set_antenna_enable(
	hackrf_device* device,
	const uint8_t value);

/**
 * Convert @ref hackrf_error into human-readable string
 * @param errcode enum to convert
 * @return human-readable name of error
 * @ingroup library
 */
extern ADDAPI const char* ADDCALL hackrf_error_name(enum hackrf_error errcode);

/**
 * Convert @ref hackrf_board_id into human-readable string
 * 
 * @param board_id enum to convert
 * @return human-readable name of board id
 * @ingroup device
 */
extern ADDAPI const char* ADDCALL hackrf_board_id_name(enum hackrf_board_id board_id);

/**
 * Lookup platform ID (HACKRF_PLATFORM_xxx) from board id (@ref hackrf_board_id)
 * 
 * @param board_id @ref hackrf_board_id enum variant to convert
 * @return @ref HACKRF_PLATFORM_JAWBREAKER, @ref HACKRF_PLATFORM_HACKRF1_OG, @ref HACKRF_PLATFORM_RAD1O, @ref HACKRF_PLATFORM_HACKRF1_R9 or 0
 * @ingroup device
*/
extern ADDAPI uint32_t ADDCALL hackrf_board_id_platform(enum hackrf_board_id board_id);

// part of docstring is from hackrf.h
/**
 * Convert @ref hackrf_usb_board_id into human-readable string.
 * @param usb_board_id enum to convert
 * @return human-readable name of board id
 * @ingroup device
 */
extern ADDAPI const char* ADDCALL hackrf_usb_board_id_name(
	enum hackrf_usb_board_id usb_board_id);

/**
 * Convert @ref rf_path_filter into human-readable string
 * @param path enum to convert
 * @return human-readable name of filter path
 * @ingroup configuration
 */
extern ADDAPI const char* ADDCALL hackrf_filter_path_name(const enum rf_path_filter path);

/**
 * Compute nearest valid baseband filter bandwidth lower than a specified value
 * 
 * The result can be used via @ref hackrf_set_baseband_filter_bandwidth
 * 
 * @param bandwidth_hz desired filter bandwidth in Hz
 * @return the highest valid filter bandwidth lower than @p bandwidth_hz in Hz 
 * @ingroup configuration
 */
extern ADDAPI uint32_t ADDCALL hackrf_compute_baseband_filter_bw_round_down_lt(
	const uint32_t bandwidth_hz);

/**
 * Compute nearest valid baseband filter bandwidth to specified value
 * 
 * The result can be used via @ref hackrf_set_baseband_filter_bandwidth
 * 
 * @param bandwidth_hz desired filter bandwidth in Hz
 * @return nearest valid filter bandwidth in Hz
 * @ingroup configuration
 */
extern ADDAPI uint32_t ADDCALL hackrf_compute_baseband_filter_bw(
	const uint32_t bandwidth_hz);

/* All features below require USB API version 0x1002 or higher) */

/**
 * Set hardware sync mode (hardware triggering)
 * 
 * See the documentation on hardware triggering for details
 * 
 * Requires USB API version 0x0102 or above!
 * @param device device to configure
 * @param value enable (1) or disable (0) hardware triggering
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup streaming
 */
extern ADDAPI int ADDCALL hackrf_set_hw_sync_mode(
	hackrf_device* device,
	const uint8_t value);

/**
 * Initialize sweep mode
 * 
 * In this mode, in a single data transfer (single call to the RX transfer callback), multiple blocks of size @p num_bytes bytes are received with different center frequencies. At the beginning of each block, a 10-byte frequency header is present in `0x7F - 0x7F - uint64_t frequency (LSBFIRST, in Hz)` format, followed by the actual samples.
 * 
 * Requires USB API version 0x0102 or above!
 * @param device device to configure
 * @param frequency_list list of start-stop frequency pairs in MHz
 * @param num_ranges length of array @p frequency_list (in pairs, so total array length / 2!). Must be less than @ref MAX_SWEEP_RANGES
 * @param num_bytes number of bytes to capture per tuning, must be a multiple of @ref BYTES_PER_BLOCK
 * @param step_width width of each tuning step in Hz
 * @param offset frequency offset added to tuned frequencies. sample_rate / 2 is a good value
 * @param style sweep style
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup streaming
 */
extern ADDAPI int ADDCALL hackrf_init_sweep(
	hackrf_device* device,
	const uint16_t* frequency_list,
	const int num_ranges,
	const uint32_t num_bytes,
	const uint32_t step_width,
	const uint32_t offset,
	const enum sweep_style style);

/**
 * Query connected Opera Cake boards
 * 
 * Returns a @ref HACKRF_OPERACAKE_MAX_BOARDS size array of addresses, with @ref HACKRF_OPERACAKE_ADDRESS_INVALID as a placeholder
 *
 * Requires USB API version 0x0105 or above!
 * @param[in] device device to query
 * @param[out] boards list of boards
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup operacake
 */
extern ADDAPI int ADDCALL hackrf_get_operacake_boards(
	hackrf_device* device,
	uint8_t* boards);

/**
 * Setup Opera Cake operation mode
 * 
 * Requires USB API version 0x0105 or above!
 * @param device device to configure
 * @param address address of Opera Cake add-on board to configure
 * @param mode mode to use
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup operacake
 */
extern ADDAPI int ADDCALL hackrf_set_operacake_mode(
	hackrf_device* device,
	uint8_t address,
	enum operacake_switching_mode mode);

/**
 * Query Opera Cake mode
 * 
 * Requires USB API version 0x0105 or above!
 * @param[in] device device to query from
 * @param[in] address address of add-on board to query
 * @param[out] mode operation mode of the selected add-on board
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup operacake
 */
extern ADDAPI int ADDCALL hackrf_get_operacake_mode(
	hackrf_device* device,
	uint8_t address,
	enum operacake_switching_mode* mode);

/**
 * Setup Opera Cake ports in @ref OPERACAKE_MODE_MANUAL mode operation
 * 
 * Should be called after @ref hackrf_set_operacake_mode. A0 and B0 must be connected to opposite sides (A->A and B->B or A->B and B->A but not A->A and B->A or A->B and B->B)
 * 
 * Requires USB API version 0x0102 or above!
 * @param device device to configure
 * @param address address of add-on board to configure
 * @param port_a port for A0. Must be one of @ref operacake_ports
 * @param port_b port for B0. Must be one of @ref operacake_ports
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup operacake
 */
extern ADDAPI int ADDCALL hackrf_set_operacake_ports(
	hackrf_device* device,
	uint8_t address,
	uint8_t port_a,
	uint8_t port_b);

/**
 * Setup Opera Cake dwell times in @ref OPERACAKE_MODE_TIME mode operation
 * 
 * Should be called after @ref hackrf_set_operacake_mode
 * 
 * **Note:** this configuration applies to all Opera Cake boards in @ref OPERACAKE_MODE_TIME mode 
 * 
 * Requires USB API version 0x0105 or above!
 * @param device device to configure
 * @param dwell_times list of dwell times to setup
 * @param count number of dwell times to setup. Must be at most @ref HACKRF_OPERACAKE_MAX_DWELL_TIMES.
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup operacake
 */
extern ADDAPI int ADDCALL hackrf_set_operacake_dwell_times(
	hackrf_device* device,
	hackrf_operacake_dwell_time* dwell_times,
	uint8_t count);

/**
 * Setup Opera Cake frequency ranges in @ref OPERACAKE_MODE_FREQUENCY mode operation
 * 
 * Should be called after @ref hackrf_set_operacake_mode
 *
 * **Note:** this configuration applies to all Opera Cake boards in @ref OPERACAKE_MODE_FREQUENCY mode
 * 
 * Requires USB API version 0x0103 or above!
 * @param device device to configure
 * @param freq_ranges list of frequency ranges to setup
 * @param count number of ranges to setup. Must be at most @ref HACKRF_OPERACAKE_MAX_FREQ_RANGES.
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup operacake
 */
extern ADDAPI int ADDCALL hackrf_set_operacake_freq_ranges(
	hackrf_device* device,
	hackrf_operacake_freq_range* freq_ranges,
	uint8_t count);

/**
 * Reset HackRF device
 * 
 * Requires USB API version 0x0102 or above!
 * @param device device to reset
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup device
 */
extern ADDAPI int ADDCALL hackrf_reset(hackrf_device* device);

/**
 * Setup Opera Cake frequency ranges in @ref OPERACAKE_MODE_FREQUENCY mode operation
 * 
 * Old function to set ranges with. Use @ref hackrf_set_operacake_freq_ranges instead!
 * 
 * **Note:** this configuration applies to all Opera Cake boards in @ref OPERACAKE_MODE_FREQUENCY mode
 * 
 * Requires USB API version 0x0103 or above!
 * @param device device to configure
 * @param ranges ranges to setup. Should point to a valid @ref hackrf_operacake_freq_range array.
 * @param num_ranges length of ranges to setup, must be number of ranges * 5. Must be at most 8*5=40. (internally called len_ranges, possible typo)
 * @deprecated This has been replaced by @ref hackrf_set_operacake_freq_ranges
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup operacake
 */
extern ADDAPI int ADDCALL hackrf_set_operacake_ranges(
	hackrf_device* device,
	uint8_t* ranges,
	uint8_t num_ranges);

/**
 * Enable / disable CLKOUT
 * 
 * Requires USB API version 0x0103 or above!
 * @param device device to configure
 * @param value clock output enabled (0/1)
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup configuration
 */
extern ADDAPI int ADDCALL hackrf_set_clkout_enable(
	hackrf_device* device,
	const uint8_t value);

/**
 * Get CLKIN status
 * 
 * Check if an external clock signal is detected on the CLKIN port.
 * 
 * Requires USB API version 0x0106 or above!
 * @param[in] device device to read status from
 * @param[out] status external clock detected (0/1)
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup configuration
 */
extern ADDAPI int ADDCALL hackrf_get_clkin_status(hackrf_device* device, uint8_t* status);

/**
 * Perform GPIO test on an Opera Cake addon board
 * 
 * Value 0xFFFF means "GPIO mode disabled", and hackrf_operacake advises to remove additional add-on boards and retry.
 * Value 0 means all tests passed.
 * In any other values, a 1 bit signals an error. Bits are grouped in groups of 3. Encoding:
 * 0 - u1ctrl - u3ctrl0 - u3ctrl1 - u2ctrl0 - u2ctrl1
 * 
 * Requires USB API version 0x0103 or above!
 * @param[in] device device to perform test on
 * @param[in] address address of Opera Cake board to test
 * @param[out] test_result result of tests
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup operacake
 */
extern ADDAPI int ADDCALL hackrf_operacake_gpio_test(
	hackrf_device* device,
	uint8_t address,
	uint16_t* test_result);

#ifdef HACKRF_ISSUE_609_IS_FIXED
/**
 * Read CPLD checksum
 * 
 * This function is not always available, see [issue 609](https://github.com/greatscottgadgets/hackrf/issues/609)
 * 
 * Requires USB API version 0x0103 or above!
 * @param[in] device device to read checksum from
 * @param[out] crc CRC checksum of the CPLD configuration
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup debug
 */
extern ADDAPI int ADDCALL hackrf_cpld_checksum(hackrf_device* device, uint32_t* crc);
#endif /* HACKRF_ISSUE_609_IS_FIXED */

/**
 * Enable / disable UI display (RAD1O, PortaPack, etc.)
 * 
 * Enable or disable the display on display-enabled devices (Rad1o, PortaPack)
 * 
 * Requires USB API version 0x0104 or above!
 * @param device device to enable/disable UI on
 * @param value Enable UI. Must be 1 or 0
 * @return @ref HACKRF_SUCCESS on success or @ref HACKRF_ERROR_LIBUSB on usb error
 * @ingroup device
 */
extern ADDAPI int ADDCALL hackrf_set_ui_enable(hackrf_device* device, const uint8_t value);

/**
 * Start RX sweep
 * 
 * See @ref hackrf_init_sweep for more info
 *
 * Requires USB API version 0x0104 or above!
 * @param device device to start sweeping
 * @param callback rx callback processing the received data
 * @param rx_ctx User provided RX context. Not used by the library, but available to @p callback as @ref hackrf_transfer.rx_ctx.
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup streaming
 */
extern ADDAPI int ADDCALL hackrf_start_rx_sweep(
	hackrf_device* device,
	hackrf_sample_block_cb_fn callback,
	void* rx_ctx);

// docsstring partly from hackrf.c
/**
 * Get USB transfer buffer size.
 * @param[in] device unused
 * @return size in bytes
 * @ingroup library
 */
extern ADDAPI size_t ADDCALL hackrf_get_transfer_buffer_size(hackrf_device* device);

// docsstring partly from hackrf.c
/**
 * Get the total number of USB transfer buffers.
 * @param[in] device unused
 * @return number of buffers
 * @ingroup library
 */
extern ADDAPI uint32_t ADDCALL hackrf_get_transfer_queue_depth(hackrf_device* device);

/**
 * Read board revision of device
 * 
 * Requires USB API version 0x0106 or above!
 * @param[in] device device to read board revision from
 * @param[out] value revision enum, will become one of @ref hackrf_board_rev. Should be initialized with @ref BOARD_REV_UNDETECTED
 * @return @ref HACKRF_SUCCESS on success or @ref HACKRF_ERROR_LIBUSB
 * @ingroup device
 */
extern ADDAPI int ADDCALL hackrf_board_rev_read(hackrf_device* device, uint8_t* value);

/**
 * Convert board revision name
 * 
 * @param board_rev board revision enum from @ref hackrf_board_rev_read
 * @returns human-readable name of board revision. Discards GSG bit. 
 * @ingroup device
 */
extern ADDAPI const char* ADDCALL hackrf_board_rev_name(enum hackrf_board_rev board_rev);

/**
 * Read supported platform of device
 * 
 * Returns a combination of @ref HACKRF_PLATFORM_JAWBREAKER | @ref HACKRF_PLATFORM_HACKRF1_OG | @ref HACKRF_PLATFORM_RAD1O | @ref HACKRF_PLATFORM_HACKRF1_R9
 * 
 * Requires USB API version 0x0106 or above!
 * @param[in] device device to query
 * @param[out] value supported platform bitfield
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup device
 */
extern ADDAPI int ADDCALL hackrf_supported_platform_read(
	hackrf_device* device,
	uint32_t* value);

/**
 * Turn on or off (override) the LEDs of the HackRF device
 * 
 * This function can turn on or off the LEDs of the device. There are 3 controllable LEDs on the HackRF one: USB, RX and TX. On the Rad1o, there are 4 LEDs. Each LED can be set individually, but the setting might get overridden by other functions.
 * 
 * The LEDs can be set via specifying them as bits of a 8 bit number @p state, bit 0 representing the first (USB on the HackRF One) and bit 3 or 4 representing the last LED. The upper 4 or 5 bits are unused. For example, binary value 0bxxxxx101 turns on the USB and TX LEDs on the HackRF One. 
 * 
 * Requires USB API version 0x0107 or above!
 * @param device device to query
 * @param state LED states as a bitfield
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup device
 * 
*/
extern ADDAPI int ADDCALL hackrf_set_leds(hackrf_device* device, const uint8_t state);

/**
 * Configure bias tee behavior of the HackRF device when changing RF states
 * 
 * This function allows the user to configure bias tee behavior so that it can be turned on or off automatically by the HackRF when entering the RX, TX, or OFF state. By default, the HackRF switches off the bias tee when the RF path switches to OFF mode.
 * 
 * The bias tee configuration is specified via a bitfield:
 * 
 * 0000000TmmRmmOmm
 * 
 * Where setting T/R/O bits indicates that the TX/RX/Off behavior should be set to mode 'mm', 0=don't modify
 * 
 * mm specifies the bias tee mode:
 * 
 * 00 - do nothing
 * 01 - reserved, do not use
 * 10 - disable bias tee
 * 11 - enable bias tee
 * 
 * @param device device to configure
 * @param state Bias tee states, as a bitfield
 * @return @ref HACKRF_SUCCESS on success or @ref hackrf_error variant
 * @ingroup device
 * 
*/
extern ADDAPI int ADDCALL hackrf_set_user_bias_t_opts(
	hackrf_device* device,
	hackrf_bias_t_user_settting_req* req);

#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif /*__HACKRF_H__*/
