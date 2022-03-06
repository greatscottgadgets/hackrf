#
# Python wrapper for libhackrf
#
# Copyright 2019 Mathieu Peyrega <mathieu.peyrega@gmail.com>
#
# This file is part of HackRF.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

import logging
from ctypes import *
from enum import IntEnum

logging.basicConfig()


#
# libhackrf enums
#
class LibHackRfReturnCode(IntEnum):
    HACKRF_SUCCESS = 0
    HACKRF_TRUE = 1
    HACKRF_ERROR_INVALID_PARAM = -2
    HACKRF_ERROR_NOT_FOUND = -5
    HACKRF_ERROR_BUSY = -6
    HACKRF_ERROR_NO_MEM = -11
    HACKRF_ERROR_LIBUSB = -1000
    HACKRF_ERROR_THREAD = -1001
    HACKRF_ERROR_STREAMING_THREAD_ERR = -1002
    HACKRF_ERROR_STREAMING_STOPPED = -1003
    HACKRF_ERROR_STREAMING_EXIT_CALLED = -1004
    HACKRF_ERROR_USB_API_VERSION = -1005
    HACKRF_ERROR_NOT_LAST_DEVICE = -2000
    HACKRF_ERROR_OTHER = -9999


class LibHackRfBoardIds(IntEnum):
    BOARD_ID_JELLYBEAN = 0
    BOARD_ID_JAWBREAKER = 1
    BOARD_ID_HACKRF_ONE = 2
    BOARD_ID_RAD1O = 3
    BOARD_ID_INVALID = 0xFF


class LibHackRfUSBBoardIds(IntEnum):
    USB_BOARD_ID_JAWBREAKER = 0x604B
    USB_BOARD_ID_HACKRF_ONE = 0x6089
    USB_BOARD_ID_RAD1O = 0xCC15
    USB_BOARD_ID_INVALID = 0xFFFF


class LibHackRfPathFilter(IntEnum):
    RF_PATH_FILTER_BYPASS = 0
    RF_PATH_FILTER_LOW_PASS = 1
    RF_PATH_FILTER_HIGH_PASS = 2


class LibHackRfTransceiverMode(IntEnum):
    TRANSCEIVER_MODE_OFF = 0
    TRANSCEIVER_MODE_RX = 1
    TRANSCEIVER_MODE_TX = 2
    TRANSCEIVER_MODE_SS = 3


class LibHackRfHwMode(IntEnum):
    HW_MODE_OFF = 0
    HW_MODE_ON = 1


#
#   C structs or datatypes needed to interface Python and C
#
hackrf_device_p = c_void_p


class hackrf_transfer(Structure):
    _fields_ = [("device", hackrf_device_p),
                ("buffer", POINTER(c_ubyte)),
                ("buffer_length", c_int),
                ("valid_length", c_int),
                ("rx_ctx", c_void_p),
                ("tx_ctx", c_void_p)]


class read_partid_serialno_t(Structure):
    _fields_ = [("part_id", c_uint32 * 2),
                ("serial_no", c_uint32 * 4)]


class hackrf_device_list_t(Structure):
    _fields_ = [("serial_numbers", POINTER(c_char_p)),
                ("usb_board_ids", POINTER(c_int)),
                ("usb_device_index", POINTER(c_int)),
                ("devicecount", c_int),
                ("usb_devices", POINTER(c_void_p)),
                ("usb_devicecount", c_int)]


hackrf_transfer_callback_t = CFUNCTYPE(c_int, POINTER(hackrf_transfer))


#
#   libhackrf Python wrapper class
#
class HackRF(object):
    #   Class attibutes
    __libhackrf = None
    __libhackrfpath = None
    __libraryversion = None
    __libraryrelease = None
    __instances = list()
    __openedInstances = dict()
    __logger = logging.getLogger("pyHackRF")
    __logger.setLevel(logging.CRITICAL)

    @classmethod
    def setLogLevel(cls, level):
        cls.__logger.setLevel(level)

    def __init__(self, libhackrf_path='libhackrf.so.0'):
        if (not __class__.initialized()):
            __class__.initialize(libhackrf_path)
        else:
            __class__.__logger.debug("Instanciating " + __class__.__name__ + " object number #%d",
                                     len(__class__.__instances))

            #   Instances attributes
            #   Description, serial and internals
            self.__pDevice = hackrf_device_p(None)
            self.__boardId = None
            self.__usbboardId = None
            self.__usbIndex = None
            self.__usbAPIVersion = None
            self.__boardFwVersionString = None
            self.__partId = None
            self.__serialNo = None
            self.__CPLDcrc = None
            self.__txCallback = None
            self.__rxCallback = None
            #   RF state and settings
            self.__transceiverMode = LibHackRfTransceiverMode.TRANSCEIVER_MODE_OFF
            self.__hwSyncMode = LibHackRfHwMode.HW_MODE_OFF
            self.__clockOutMode = LibHackRfHwMode.HW_MODE_OFF
            self.__amplificatorMode = LibHackRfHwMode.HW_MODE_OFF
            self.__antennaPowerMode = LibHackRfHwMode.HW_MODE_OFF

            self.__crystalppm = 0.

            # trial to implement getters, but this would probably be better to
            # have it done from the .c library rather than a DIY solution here
            # relevant parts have been commented out
            # self.__lnaGain = 0
            # self.__vgaGain = 0
            # self.__txvgaGain = 0
            # self.__basebandFilterBandwidth = 0
            # self.__frequency = 0
            # self.__loFrequency = 0
            # self.__rfFilterPath = LibHackRfPathFilter.RF_PATH_FILTER_BYPASS

        __class__.__instances.append(self)

    def __del__(self):
        __class__.__instances.remove(self)
        __class__.__logger.debug(__class__.__name__ + " __del__ being called")
        if (len(__class__.__instances) == 0):
            __class__.__logger.debug(__class__.__name__ + " __del__ being called on the last instance")

    @classmethod
    def initialized(cls):
        return cls.__libhackrf is not None

    @classmethod
    def initialize(cls, libhackrf_path='libhackrf.so.0'):
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if (not cls.initialized()):
            cls.__libhackrfpath = libhackrf_path
            cls.__libhackrf = CDLL(cls.__libhackrfpath)

            #
            #   begin of C to Python bindings
            #

            #
            #   Library initialization / deinitialization
            #

            # extern ADDAPI int ADDCALL hackrf_init();
            cls.__libhackrf.hackrf_init.restype = c_int
            cls.__libhackrf.hackrf_init.argtypes = []

            # extern ADDAPI int ADDCALL hackrf_exit();
            cls.__libhackrf.hackrf_exit.restype = c_int
            cls.__libhackrf.hackrf_exit.argtypes = []

            # extern ADDAPI const char* ADDCALL hackrf_library_version();
            cls.__libhackrf.hackrf_library_version.restype = c_char_p
            cls.__libhackrf.hackrf_library_version.argtypes = []
            # extern ADDAPI const char* ADDCALL hackrf_library_release();
            cls.__libhackrf.hackrf_library_release.restype = c_char_p
            cls.__libhackrf.hackrf_library_release.argtypes = []

            # extern ADDAPI const char* ADDCALL hackrf_error_name(enum hackrf_error errcode);
            cls.__libhackrf.hackrf_error_name.restype = c_char_p
            cls.__libhackrf.hackrf_error_name.argtypes = [c_int]

            # extern ADDAPI int ADDCALL hackrf_open(hackrf_device** device);
            # purposely not offered as a replacement logic is set-up

            #
            # Not implemented yet
            #

            # extern ADDAPI int ADDCALL hackrf_max2837_read(hackrf_device* device, uint8_t register_number, uint16_t* value);
            # extern ADDAPI int ADDCALL hackrf_max2837_write(hackrf_device* device, uint8_t register_number, uint16_t value);
            # extern ADDAPI int ADDCALL hackrf_si5351c_read(hackrf_device* device, uint16_t register_number, uint16_t* value);
            # extern ADDAPI int ADDCALL hackrf_si5351c_write(hackrf_device* device, uint16_t register_number, uint16_t value);
            # extern ADDAPI int ADDCALL hackrf_rffc5071_read(hackrf_device* device, uint8_t register_number, uint16_t* value);
            # extern ADDAPI int ADDCALL hackrf_rffc5071_write(hackrf_device* device, uint8_t register_number, uint16_t value);
            # extern ADDAPI int ADDCALL hackrf_spiflash_erase(hackrf_device* device);
            # extern ADDAPI int ADDCALL hackrf_spiflash_write(hackrf_device* device, const uint32_t address, const uint16_t length, unsigned char* const data);
            # extern ADDAPI int ADDCALL hackrf_spiflash_read(hackrf_device* device, const uint32_t address, const uint16_t length, unsigned char* data);
            # extern ADDAPI int ADDCALL hackrf_spiflash_status(hackrf_device* device, uint8_t* data);
            # extern ADDAPI int ADDCALL hackrf_spiflash_clear_status(hackrf_device* device);
            # extern ADDAPI int ADDCALL hackrf_cpld_write(hackrf_device* device, unsigned char* const data, const unsigned int total_length);
            # extern ADDAPI int ADDCALL hackrf_get_operacake_boards(hackrf_device* device, uint8_t* boards);
            # extern ADDAPI int ADDCALL hackrf_set_operacake_ports(hackrf_device* device, uint8_t address, uint8_t port_a, uint8_t port_b);
            # extern ADDAPI int ADDCALL hackrf_set_operacake_ranges(hackrf_device* device, uint8_t* ranges, uint8_t num_ranges);
            # extern ADDAPI int ADDCALL hackrf_operacake_gpio_test(hackrf_device* device, uint8_t address, uint16_t* test_result);

            #
            #   General low level hardware management
            #   list, open, close
            #

            # extern ADDAPI hackrf_device_list_t* ADDCALL hackrf_device_list();
            cls.__libhackrf.hackrf_device_list.restype = POINTER(hackrf_device_list_t)
            cls.__libhackrf.hackrf_device_list.argtypes = []

            # extern ADDAPI void ADDCALL hackrf_device_list_free(hackrf_device_list_t *list);
            cls.__libhackrf.hackrf_device_list_free.restype = None
            cls.__libhackrf.hackrf_device_list_free.argtypes = [POINTER(hackrf_device_list_t)]

            # extern ADDAPI int ADDCALL hackrf_open(hackrf_device** device);
            cls.__libhackrf.hackrf_open.restype = c_int
            cls.__libhackrf.hackrf_open.argtypes = [POINTER(hackrf_device_p)]

            # extern ADDAPI int ADDCALL hackrf_open_by_serial(const char* const desired_serial_number, hackrf_device** device);
            cls.__libhackrf.hackrf_open_by_serial.restype = c_int
            cls.__libhackrf.hackrf_open_by_serial.arg_types = [c_char_p, POINTER(hackrf_device_p)]

            # extern ADDAPI int ADDCALL hackrf_device_list_open(hackrf_device_list_t *list, int idx, hackrf_device** device);
            cls.__libhackrf.hackrf_device_list_open.restype = c_int
            cls.__libhackrf.hackrf_device_list_open.arg_types = [POINTER(hackrf_device_list_t), c_int,
                                                                 POINTER(hackrf_device_p)]

            # extern ADDAPI int ADDCALL hackrf_close(hackrf_device* device);
            cls.__libhackrf.hackrf_close.restype = c_int
            cls.__libhackrf.hackrf_close.argtypes = [hackrf_device_p]

            # extern ADDAPI int ADDCALL hackrf_reset(hackrf_device* device);
            cls.__libhackrf.hackrf_reset.restype = c_int
            cls.__libhackrf.hackrf_reset.argtypes = [hackrf_device_p]

            # extern ADDAPI int ADDCALL hackrf_board_id_read(hackrf_device* device, uint8_t* value);
            cls.__libhackrf.hackrf_board_id_read.restype = c_int
            cls.__libhackrf.hackrf_board_id_read.argtypes = [hackrf_device_p, POINTER(c_uint8)]

            # extern ADDAPI int ADDCALL hackrf_version_string_read(hackrf_device* device, char* version, uint8_t length);
            cls.__libhackrf.hackrf_version_string_read.restype = c_int
            cls.__libhackrf.hackrf_version_string_read.argtypes = [hackrf_device_p, POINTER(c_char), c_uint8]

            # extern ADDAPI int ADDCALL hackrf_usb_api_version_read(hackrf_device* device, uint16_t* version);
            cls.__libhackrf.hackrf_usb_api_version_read.restype = c_int
            cls.__libhackrf.hackrf_usb_api_version_read.argtypes = [hackrf_device_p, POINTER(c_uint16)]

            # extern ADDAPI int ADDCALL hackrf_board_partid_serialno_read(hackrf_device* device, read_partid_serialno_t* read_partid_serialno);
            cls.__libhackrf.hackrf_board_partid_serialno_read.restype = c_int
            cls.__libhackrf.hackrf_board_partid_serialno_read.argtypes = [hackrf_device_p,
                                                                          POINTER(read_partid_serialno_t)]

            # extern ADDAPI int ADDCALL hackrf_cpld_checksum(hackrf_device* device, uint32_t* crc);
            # this is now disabled by default in libhackrf (see hackrf.h line 323)
            #cls.__libhackrf.hackrf_cpld_checksum.restype = c_int
            #cls.__libhackrf.hackrf_cpld_checksum.argtypes = [hackrf_device_p, POINTER(c_uint32)]

            # extern ADDAPI const char* ADDCALL hackrf_board_id_name(enum hackrf_board_id board_id);
            cls.__libhackrf.hackrf_board_id_name.restype = c_char_p
            cls.__libhackrf.hackrf_board_id_name.argtypes = [c_int]
            # extern ADDAPI const char* ADDCALL hackrf_usb_board_id_name(enum hackrf_usb_board_id usb_board_id);
            cls.__libhackrf.hackrf_usb_board_id_name.restype = c_char_p
            cls.__libhackrf.hackrf_usb_board_id_name.argtypes = [c_int]

            # extern ADDAPI const char* ADDCALL hackrf_filter_path_name(const enum rf_path_filter path);
            cls.__libhackrf.hackrf_filter_path_name.restype = c_char_p
            cls.__libhackrf.hackrf_filter_path_name.argtypes = [c_int]

            # extern ADDAPI int ADDCALL hackrf_set_hw_sync_mode(hackrf_device* device, const uint8_t value);
            cls.__libhackrf.hackrf_set_hw_sync_mode.restype = c_int
            cls.__libhackrf.hackrf_set_hw_sync_mode.argtypes = [hackrf_device_p, c_uint8]

            # extern ADDAPI int ADDCALL hackrf_set_clkout_enable(hackrf_device* device, const uint8_t value);
            cls.__libhackrf.hackrf_set_clkout_enable.restype = c_int
            cls.__libhackrf.hackrf_set_clkout_enable.argtypes = [hackrf_device_p, c_uint8]

            #
            #   RF settings
            #
            # extern ADDAPI int ADDCALL hackrf_set_baseband_filter_bandwidth(hackrf_device* device, const uint32_t bandwidth_hz);
            cls.__libhackrf.hackrf_set_baseband_filter_bandwidth.restype = c_int
            cls.__libhackrf.hackrf_set_baseband_filter_bandwidth.argtypes = [hackrf_device_p, c_uint32]

            # extern ADDAPI int ADDCALL hackrf_set_freq(hackrf_device* device, const uint64_t freq_hz);
            cls.__libhackrf.hackrf_set_freq.restype = c_int
            cls.__libhackrf.hackrf_set_freq.argtypes = [hackrf_device_p, c_uint64]

            # extern ADDAPI int ADDCALL hackrf_set_freq_explicit(hackrf_device* device, const uint64_t if_freq_hz, const uint64_t lo_freq_hz, const enum rf_path_filter path);
            cls.__libhackrf.hackrf_set_freq_explicit.restype = c_int
            cls.__libhackrf.hackrf_set_freq_explicit.argtypes = [hackrf_device_p, c_uint64, c_uint64, c_uint32]

            # extern ADDAPI int ADDCALL hackrf_set_sample_rate_manual(hackrf_device* device, const uint32_t freq_hz, const uint32_t divider);
            cls.__libhackrf.hackrf_set_sample_rate_manual.restype = c_int
            cls.__libhackrf.hackrf_set_sample_rate_manual.argtypes = [hackrf_device_p, c_uint32, c_uint32]

            # extern ADDAPI int ADDCALL hackrf_set_sample_rate(hackrf_device* device, const double freq_hz);
            cls.__libhackrf.hackrf_set_sample_rate.restype = c_int
            cls.__libhackrf.hackrf_set_sample_rate.argtypes = [hackrf_device_p, c_double]

            # extern ADDAPI int ADDCALL hackrf_set_lna_gain(hackrf_device* device, uint32_t value);
            cls.__libhackrf.hackrf_set_lna_gain.restype = c_int
            cls.__libhackrf.hackrf_set_lna_gain.argtypes = [hackrf_device_p, c_uint32]

            # extern ADDAPI int ADDCALL hackrf_set_vga_gain(hackrf_device* device, uint32_t value);
            cls.__libhackrf.hackrf_set_vga_gain.restype = c_int
            cls.__libhackrf.hackrf_set_vga_gain.argtypes = [hackrf_device_p, c_uint32]

            # extern ADDAPI int ADDCALL hackrf_set_txvga_gain(hackrf_device* device, uint32_t value);
            cls.__libhackrf.hackrf_set_txvga_gain.restype = c_int
            cls.__libhackrf.hackrf_set_txvga_gain.argtypes = [hackrf_device_p, c_uint32]

            # extern ADDAPI int ADDCALL hackrf_set_amp_enable(hackrf_device* device, const uint8_t value);
            cls.__libhackrf.hackrf_set_amp_enable.restype = c_int
            cls.__libhackrf.hackrf_set_amp_enable.argtypes = [hackrf_device_p, c_uint8]

            # extern ADDAPI int ADDCALL hackrf_set_antenna_enable(hackrf_device* device, const uint8_t value);
            cls.__libhackrf.hackrf_set_antenna_enable.restype = c_int
            cls.__libhackrf.hackrf_set_antenna_enable.argtypes = [hackrf_device_p, c_uint8]

            # extern ADDAPI uint32_t ADDCALL hackrf_compute_baseband_filter_bw_round_down_lt(const uint32_t bandwidth_hz);
            cls.__libhackrf.hackrf_compute_baseband_filter_bw_round_down_lt.restype = c_int
            cls.__libhackrf.hackrf_compute_baseband_filter_bw_round_down_lt.argtypes = [c_uint32]

            # extern ADDAPI uint32_t ADDCALL hackrf_compute_baseband_filter_bw(const uint32_t bandwidth_hz);
            cls.__libhackrf.hackrf_compute_baseband_filter_bw.restype = c_int
            cls.__libhackrf.hackrf_compute_baseband_filter_bw.argtypes = [c_uint32]

            #
            #   Transfers management
            #

            # extern ADDAPI int ADDCALL hackrf_is_streaming(hackrf_device* device);
            cls.__libhackrf.hackrf_is_streaming.restype = c_int
            cls.__libhackrf.hackrf_is_streaming.argtypes = [hackrf_device_p]

            # extern ADDAPI int ADDCALL hackrf_start_rx(hackrf_device* device, hackrf_sample_block_cb_fn callback, void* rx_ctx);
            cls.__libhackrf.hackrf_start_rx.restype = c_int
            cls.__libhackrf.hackrf_start_rx.argtypes = [hackrf_device_p, hackrf_transfer_callback_t, c_void_p]

            # extern ADDAPI int ADDCALL hackrf_start_tx(hackrf_device* device, hackrf_sample_block_cb_fn callback, void* tx_ctx);
            cls.__libhackrf.hackrf_start_tx.restype = c_int
            cls.__libhackrf.hackrf_start_tx.argtypes = [hackrf_device_p, hackrf_transfer_callback_t, c_void_p]

            # extern ADDAPI int ADDCALL hackrf_stop_rx(hackrf_device* device);
            cls.__libhackrf.hackrf_stop_rx.restype = c_int
            cls.__libhackrf.hackrf_stop_rx.argtypes = [hackrf_device_p]

            # extern ADDAPI int ADDCALL hackrf_stop_tx(hackrf_device* device);
            cls.__libhackrf.hackrf_stop_tx.restype = c_int
            cls.__libhackrf.hackrf_stop_tx.argtypes = [hackrf_device_p]

            # extern ADDAPI int ADDCALL hackrf_init_sweep(hackrf_device* device, const uint16_t* frequency_list, const int num_ranges, const uint32_t num_bytes, const uint32_t step_width, const uint32_t offset, const enum sweep_style style);
            cls.__libhackrf.hackrf_init_sweep.restype = c_int
            cls.__libhackrf.hackrf_init_sweep.argtypes = [hackrf_device_p, POINTER(c_uint16), c_int, c_uint32, c_uint32,
                                                          c_uint32, c_uint32]

            #
            #   end of C to Python bindings
            #

            result = cls.__libhackrf.hackrf_init()

            if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
                cls.__logger.error(
                    cls.__name__ + " class initialization failed, error=(%d," + __class__.getHackRfErrorCodeName(
                        result) + ")", result)
            else:
                cls.__libraryversion = cls.__libhackrf.hackrf_library_version().decode("UTF-8")
                cls.__logger.debug(cls.__name__ + " library version : " + cls.__libraryversion)
                cls.__libraryrelease = cls.__libhackrf.hackrf_library_release().decode("UTF-8")
                cls.__logger.debug(cls.__name__ + " library release : " + cls.__libraryrelease)
                cls.__logger.debug(cls.__name__ + " class initialization successfull")

        else:
            __class__.__logger.debug(cls.__name__ + " is already initialized")
        return result

    @classmethod
    def getInstanceByDeviceHandle(cls, pDevice):
        return cls.__openedInstances.get(pDevice, None)

    @classmethod
    def getDeviceListPointer(cls):
        if (not __class__.initialized()):
            __class__.initialize()

        pHackRfDeviceList = cls.__libhackrf.hackrf_device_list()
        return pHackRfDeviceList

    @classmethod
    def freeDeviceList(cls, pList):
        if (not cls.initialized()):
            cls.initialize()

        pHackRfDeviceList = cls.__libhackrf.hackrf_device_list_free(pList)

    @classmethod
    def getHackRfErrorCodeName(cls, ec):
        if (not cls.initialized()):
            cls.initialize()

        return cls.__libhackrf.hackrf_error_name(ec).decode("UTF-8")

    @classmethod
    def getBoardNameById(cls, bid):
        if (not cls.initialized()):
            cls.initialize()

        return cls.__libhackrf.hackrf_board_id_name(bid).decode("UTF-8")

    @classmethod
    def getUsbBoardNameById(cls, usbbid):
        if (not cls.initialized()):
            cls.initialize()

        return cls.__libhackrf.hackrf_usb_board_id_name(usbbid).decode("UTF-8")

    @classmethod
    def getHackRFFilterPathNameById(cls, rfpid):
        if (not cls.initialized()):
            cls.initialize()

        return cls.__libhackrf.hackrf_filter_path_name(rfpid).decode("UTF-8")

    @classmethod
    def deinitialize(cls):
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if (cls.initialized()):
            for hackrf in cls.__instances:
                hackrf.stop()

            result = cls.__libhackrf.hackrf_exit()
            if (result == LibHackRfReturnCode.HACKRF_SUCCESS):
                cls.__libhackrf = None
            else:
                cls.__logger.error(
                    cls.__name__ + " class deinitialization failed, error=(%d," + __class__.getHackRfErrorCodeName(
                        result) + ")", result)

        return result

    @classmethod
    def getLibraryVersion(cls):
        if (not cls.initialized()):
            cls.initialize()

        return cls.__libraryversion

    @classmethod
    def getLibraryRelease(cls):
        if (not cls.initialized()):
            cls.initialize()

        return cls.__libraryrelease

    @classmethod
    def computeBaseBandFilterBw(cls, bandwidth):
        if (not cls.initialized()):
            cls.initialize()

        return cls.__libhackrf.hackrf_compute_baseband_filter_bw(bandwidth)

    @classmethod
    def computeBaseBandFilterBwRoundDownLt(cls, bandwidth):
        if (not cls.initialized()):
            cls.initialize()

        return cls.__libhackrf.hackrf_compute_baseband_filter_bw_round_down_lt(bandwidth)

    def opened(self):
        return self.__pDevice.value is not None

    def closed(self):
        return self.__pDevice.value is None

    def open(self, openarg=-1):
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if not self.opened():
            if (isinstance(openarg, int)):
                result = self.__openByIndex(openarg)
            elif (isinstance(openarg, str)):
                result = self.__openBySerial(openarg.lower())
        else:
            __class__.__logger.debug("Trying to open an already opened " + __class__.__name__)
        if (result == LibHackRfReturnCode.HACKRF_SUCCESS):
            __class__.__openedInstances[self.__pDevice.value] = self
        return result

    def close(self):
        __class__.__logger.debug("Trying to close a " + __class__.__name__)
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if self.opened():
            result = __class__.__libhackrf.hackrf_close(self.__pDevice)
            if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
                __class__.__logger.error(
                    "Error (%d," + __class__.getHackRfErrorCodeName(result) + ") while closing a " + __class__.__name__,
                    result)
            else:
                __class__.__logger.info("Success closing " + __class__.__name__)
                __class__.__openedInstances.pop(self.__pDevice.value, None)
                self.__pDevice.value = None
                self.__boardId = None
                self.__usbboardId = None
                self.__usbIndex = None
                self.__usbAPIVersion = None
                self.__boardFwVersionString = None
                self.__partId = None
                self.__serialNo = None
                self.__CPLDcrc = None
                self.__txCallback = None
                self.__rxCallback = None
                self.__transceiverMode = LibHackRfTransceiverMode.TRANSCEIVER_MODE_OFF
                self.__hwSyncMode = LibHackRfHwMode.HW_MODE_OFF
                self.__clockOutMode = LibHackRfHwMode.HW_MODE_OFF
                self.__amplificatorMode = LibHackRfHwMode.HW_MODE_OFF
                self.__antennaPowerMode = LibHackRfHwMode.HW_MODE_OFF
                self.__crystalppm = 0
                # self.__lnaGain = 0
                # self.__vgaGain = 0
                # self.__txvgaGain = 0
                # self.__basebandFilterBandwidth = 0
                # self.__frequency = 0
                # self.__loFrequency = 0
                # self.__rfFilterPath = LibHackRfPathFilter.RF_PATH_FILTER_BYPASS

        else:
            __class__.__logger.debug("Trying to close a non-opened " + __class__.__name__)
        return result

    def reset(self):
        __class__.__logger.debug("Trying to reset a " + __class__.__name__)
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if self.opened():
            result = __class__.__libhackrf.hackrf_reset(self.__pDevice)
        else:
            __class__.__logger.debug("Trying to reset a non-opened " + __class__.__name__)
        return result

    def getBoardSerialNumberString(self, words_separator=''):
        if not self.opened():
            __class__.__logger.error(
                __class__.__name__ + " getBoardSerialNumberString() has been called on a closed instance")
            raise Exception(__class__.__name__ + " getBoardSerialNumberString() has been called on a closed instance")
        else:
            return (
                        "{:08x}" + words_separator + "{:08x}" + words_separator + "{:08x}" + words_separator + "{:08x}").format(
                self.__serialNo[0], self.__serialNo[1], self.__serialNo[2], self.__serialNo[3])

    def getBoardSerialNumber(self):
        if not self.opened():
            __class__.__logger.error(
                __class__.__name__ + " getBoardSerialNumber() has been called on a closed instance")
            raise Exception(__class__.__name__ + " getBoardSerialNumber() has been called on a closed instance")
        else:
            return self.__serialNo

    def __readBoardSerialNumber(self):
        #   Board SerialNo and PartID
        serinfo = read_partid_serialno_t((-1, -1), (-1, -1, -1, -1))
        result = __class__.__libhackrf.hackrf_board_partid_serialno_read(self.__pDevice, byref(serinfo))
        if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
            __class__.__logger.error(
                "Error (%d," + __class__.getHackRfErrorCodeName(result) + ") on hackrf_board_partid_serialno_read",
                result)
        else:
            self.__partId = (serinfo.part_id[0], serinfo.part_id[1])
            __class__.__logger.debug(
                __class__.__name__ + " opened board part id : " + "{:08x}:{:08x}".format(self.__partId[0],
                                                                                         self.__partId[1]))
            self.__serialNo = (serinfo.serial_no[0], serinfo.serial_no[1], serinfo.serial_no[2], serinfo.serial_no[3])
            __class__.__logger.debug(
                __class__.__name__ + " opened board serial number : " + self.getBoardSerialNumberString(':'))

    def __openByIndex(self, deviceindex):
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER

        pHDL = __class__.getDeviceListPointer()

        if (deviceindex == -1):
            __class__.__logger.debug("Try to open first available HackRF")
            __class__.__logger.debug("%d devices detected", pHDL.contents.devicecount)
            for index in range(0, pHDL.contents.devicecount):
                __class__.__logger.debug("trying to open device index %d", index)
                result = self.open(index)
                if (result == LibHackRfReturnCode.HACKRF_SUCCESS):
                    break
                else:
                    __class__.__logger.debug("tested hackrf not available")

        else:
            __class__.__logger.debug("Trying to open HackRF with index=%d", deviceindex)

            result = __class__.__libhackrf.hackrf_device_list_open(pHDL, deviceindex, byref(self.__pDevice))
            if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
                __class__.__logger.error("Error (%d," + __class__.getHackRfErrorCodeName(
                    result) + ") while opening " + __class__.__name__ + " with index=%d", result, deviceindex)
            else:
                self.__readBoardSerialNumber()
                self.__usbboardId = pHDL.contents.usb_board_ids[deviceindex]
                self.__usbIndex = pHDL.contents.usb_device_index[deviceindex]
                self.__readBoardInfos()
                __class__.__logger.info("Success opening " + __class__.__name__)

        __class__.freeDeviceList(pHDL)
        return result

    def __openBySerial(self, deviceserial):
        __class__.__logger.debug("Trying to open a HackRF by serial number: " + deviceserial)
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        result = __class__.__libhackrf.hackrf_open_by_serial(c_char_p(deviceserial.encode("UTF-8")),
                                                             byref(self.__pDevice))
        if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
            __class__.__logger.error("Error (%d," + __class__.getHackRfErrorCodeName(
                result) + ") while opening " + __class__.__name__ + " with serial number=" + deviceserial, result)
        else:
            self.__readBoardSerialNumber()
            pHDL = __class__.getDeviceListPointer()
            for deviceindex in range(0, pHDL.contents.devicecount):
                if pHDL.contents.serial_numbers[deviceindex].decode("UTF-8") == self.getBoardSerialNumberString():
                    self.__usbboardId = pHDL.contents.usb_board_ids[deviceindex]
                    self.__usbIndex = pHDL.contents.usb_device_index[deviceindex]
                    break;
            __class__.freeDeviceList(pHDL)
            self.__readBoardInfos()
            __class__.__logger.info("Success opening " + __class__.__name__)

        return result

    def __readBoardInfos(self):
        if not self.opened():
            __class__.__logger.error(__class__.__name__ + " __readBoardInfos() has been called on a closed instance")
        else:
            #   Board Id
            bId = c_uint8(0)
            result = __class__.__libhackrf.hackrf_board_id_read(self.__pDevice, byref(bId))
            if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
                __class__.__logger.error(
                    "Error (%d," + __class__.getHackRfErrorCodeName(result) + ") on hackrf_board_id_read", result)
            else:
                self.__boardId = LibHackRfBoardIds(bId.value)
                __class__.__logger.debug(
                    __class__.__name__ + " opened board id : %d, " + self.__boardId.name + ", " + __class__.getBoardNameById(
                        self.__boardId), self.__boardId.value)
                __class__.__logger.debug(__class__.__name__ + " opened usbboard id : " + "{:04x}, ".format(
                    self.__usbboardId) + __class__.getUsbBoardNameById(self.__usbboardId))

            #   Board Firmware Version
            bfwversion_size = 128
            bfwversion = (c_char * bfwversion_size)()
            result = __class__.__libhackrf.hackrf_version_string_read(self.__pDevice, bfwversion, bfwversion_size)
            if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
                __class__.__logger.error(
                    "Error (%d," + __class__.getHackRfErrorCodeName(result) + ") on hackrf_version_string_read", result)
            else:
                self.__boardFwVersionString = bfwversion.value.decode("UTF-8")
                __class__.__logger.debug(
                    __class__.__name__ + " opened board firmware version : " + self.__boardFwVersionString)

            #   Board USB API version
            bUSB_API_ver = c_uint16(0)
            result = __class__.__libhackrf.hackrf_usb_api_version_read(self.__pDevice, byref(bUSB_API_ver))
            if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
                __class__.__logger.error(
                    "Error (%d," + __class__.getHackRfErrorCodeName(result) + ") on hackrf_usb_api_version_read",
                    result)
            else:
                self.__usbAPIVersion = bUSB_API_ver.value
                __class__.__logger.debug(
                    __class__.__name__ + " opened board USB API version : " + "{:02x}:{:02x}".format(
                        self.__usbAPIVersion >> 8, self.__usbAPIVersion & 0xFF))

            #   Board CLPD checksum
            #cpld_checsum = c_uint32(-1)
            #result = __class__.__libhackrf.hackrf_cpld_checksum(self.__pDevice, byref(cpld_checsum))
            #if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
            #    __class__.__logger.error(
            #        "Error (%d," + __class__.getHackRfErrorCodeName(result) + ") on hackrf_cpld_checksum", result)
            #else:
            #    self.__CPLDcrc = cpld_checsum.value
            #    __class__.__logger.debug(
            #        __class__.__name__ + " opened board CPLD checksum : " + "{:08x}".format(self.__CPLDcrc))

    def printBoardInfos(self):
        if not self.opened():
            print(__class__.__name__ + " is closed and informations cannot be displayed")
        else:
            print(__class__.__name__ + " board id            : " + "{:d}".format(
                self.__boardId.value) + ", " + self.__boardId.name)
            print(__class__.__name__ + " board name          : " + __class__.getBoardNameById(self.__boardId))
            print(__class__.__name__ + " board USB id        : " + "0x{:04x}".format(
                self.__usbboardId) + ", " + __class__.getUsbBoardNameById(self.__usbboardId))
            print(__class__.__name__ + " board USB index     : " + "0x{:04x}".format(self.__usbIndex))
            print(__class__.__name__ + " board USB API       : " + "{:02x}:{:02x}".format(self.__usbAPIVersion >> 8,
                                                                                          self.__usbAPIVersion & 0xFF))
            print(__class__.__name__ + " board firmware      : " + self.__boardFwVersionString)
            print(__class__.__name__ + " board part id       : " + "{:08x}:{:08x}".format(self.__partId[0],
                                                                                          self.__partId[1]))
            print(__class__.__name__ + " board part id       : " + self.getBoardSerialNumberString(':'))
            #print(__class__.__name__ + " board CPLD checksum : " + "0x{:08x}".format(self.__CPLDcrc))

    def stop(self):
        # TODO : implement transfer stopping logics ?
        self.close()

    def setHwSyncMode(self, mode):
        __class__.__logger.debug(__class__.__name__ + " Trying to set HwSyncMode")
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if self.opened():
            result = __class__.__libhackrf.hackrf_set_hw_sync_mode(self.__pDevice, c_uint8(LibHackRfHwMode(mode).value))
            if (result == LibHackRfReturnCode.HACKRF_SUCCESS):
                self.__hwSyncMode = mode
        else:
            __class__.__logger.debug("Trying to set HwSyncMode for non-opened " + __class__.__name__)
        return result

    def getHwSyncMode(self):
        return self.__hwSyncMode

    def setClkOutMode(self, mode):
        __class__.__logger.debug(__class__.__name__ + " Trying to set ClkOutMode")
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if self.opened():
            result = __class__.__libhackrf.hackrf_set_clkout_enable(self.__pDevice,
                                                                    c_uint8(LibHackRfHwMode(mode).value))
            if (result == LibHackRfReturnCode.HACKRF_SUCCESS):
                self.__clockOutMode = mode
        else:
            __class__.__logger.debug("Trying to set ClkOutMode for non-opened " + __class__.__name__)
        return result

    def getClkOutMode(self):
        return self.__clockOutMode

    def setAmplifierMode(self, mode):
        __class__.__logger.debug(__class__.__name__ + " Trying to set AmplifierMode")
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if self.opened():
            result = __class__.__libhackrf.hackrf_set_amp_enable(self.__pDevice, c_uint8(LibHackRfHwMode(mode).value))
            if (result == LibHackRfReturnCode.HACKRF_SUCCESS):
                self.__amplificatorMode = mode
        else:
            __class__.__logger.debug("Trying to set AmplifierMode for non-opened " + __class__.__name__)
        return result

    def getAmplifierMode(self):
        return self.__amplificatorMode

    def setAntennaPowerMode(self, mode):
        __class__.__logger.debug(__class__.__name__ + " Trying to set AntennaPowerMode")
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if self.opened():
            result = __class__.__libhackrf.hackrf_set_antenna_enable(self.__pDevice,
                                                                     c_uint8(LibHackRfHwMode(mode).value))
            if (result == LibHackRfReturnCode.HACKRF_SUCCESS):
                self.__antennaPowerMode = mode
        else:
            __class__.__logger.debug("Trying to set AntennaPowerMode for non-opened " + __class__.__name__)
        return result

    def getAntennaPowerMode(self):
        return self.__antennaPowerMode

    def isStreaming(self):
        __class__.__logger.debug(__class__.__name__ + " Trying to call isStreaming")
        if self.opened() and self.getTransceiverMode() != LibHackRfTransceiverMode.TRANSCEIVER_MODE_OFF:
            return __class__.__libhackrf.hackrf_is_streaming(self.__pDevice) == LibHackRfReturnCode.HACKRF_TRUE
        else:
            print("isStreaming corner case")
            __class__.__logger.debug(
                "Trying to call isStreaming for non-opened or non transmitting " + __class__.__name__)
            return False

    def stopRX(self):
        __class__.__logger.debug(__class__.__name__ + " Trying to stop RX")
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if self.opened() and self.getTransceiverMode() != LibHackRfTransceiverMode.TRANSCEIVER_MODE_OFF:
            result = __class__.__libhackrf.hackrf_stop_rx(self.__pDevice)
            if (result == LibHackRfReturnCode.HACKRF_SUCCESS):
                __class__.__logger.info("Success stopping RX")
                self.__transceiverMode = LibHackRfTransceiverMode.TRANSCEIVER_MODE_OFF
            else:
                __class__.__logger.error(
                    "Error (%d," + __class__.getHackRfErrorCodeName(result) + ") while stopping RX ", result)
        else:
            __class__.__logger.debug("Trying to stop RX for non-opened or non transmitting " + __class__.__name__)
        return result

    def stopTX(self):
        __class__.__logger.debug(__class__.__name__ + " Trying to stop TX")
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if self.opened() and self.getTransceiverMode() != LibHackRfTransceiverMode.TRANSCEIVER_MODE_OFF:
            result = __class__.__libhackrf.hackrf_stop_tx(self.__pDevice)
            if (result == LibHackRfReturnCode.HACKRF_SUCCESS):
                __class__.__logger.info("Success stopping TX")
                self.__transceiverMode = LibHackRfTransceiverMode.TRANSCEIVER_MODE_OFF
            else:
                __class__.__logger.error(
                    "Error (%d," + __class__.getHackRfErrorCodeName(result) + ") while stopping TX ", result)
        else:
            __class__.__logger.debug("Trying to stop TX for non-opened or non transmitting " + __class__.__name__)
        return result

    def startRX(self, callback, rx_context):
        __class__.__logger.debug(__class__.__name__ + " Trying to start RX")
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if self.opened() and self.getTransceiverMode() == LibHackRfTransceiverMode.TRANSCEIVER_MODE_OFF:
            self.__rxCallback = hackrf_transfer_callback_t(callback)
            if rx_context is None:
                result = __class__.__libhackrf.hackrf_start_rx(self.__pDevice, self.__rxCallback,
                                                               None)
            else:
                result = __class__.__libhackrf.hackrf_start_rx(self.__pDevice, self.__rxCallback,
                                                               byref(rx_context))
            if (result == LibHackRfReturnCode.HACKRF_SUCCESS):
                __class__.__logger.info("Success starting RX")
                self.__transceiverMode = LibHackRfTransceiverMode.TRANSCEIVER_MODE_RX
            else:
                __class__.__logger.error(
                    "Error (%d," + __class__.getHackRfErrorCodeName(result) + ") while starting RX ", result)
        else:
            __class__.__logger.debug("Trying to start RX for non-opened or in transmission " + __class__.__name__)
        return result

    def startTX(self, callback, tx_context):
        __class__.__logger.debug(__class__.__name__ + " Trying to start TX")
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if self.opened() and self.getTransceiverMode() == LibHackRfTransceiverMode.TRANSCEIVER_MODE_OFF:
            self.__txCallback = hackrf_transfer_callback_t(callback)
            if tx_context is None:
                result = __class__.__libhackrf.hackrf_start_tx(self.__pDevice, self.__txCallback,
                                                               None)
            else:
                result = __class__.__libhackrf.hackrf_start_tx(self.__pDevice, self.__txCallback,
                                                               byref(tx_context))
            if (result == LibHackRfReturnCode.HACKRF_SUCCESS):
                __class__.__logger.info("Success starting TX")
                self.__transceiverMode = LibHackRfTransceiverMode.TRANSCEIVER_MODE_TX
            else:
                __class__.__logger.error(
                    "Error (%d," + __class__.getHackRfErrorCodeName(result) + ") while starting TX ", result)
        else:
            __class__.__logger.debug("Trying to start TX for non-opened or in transmission " + __class__.__name__)
        return result

    def getTransceiverMode(self):
        return self.__transceiverMode

    def setLNAGain(self, gain):
        __class__.__logger.debug(__class__.__name__ + " Trying to set LNA gain")
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if self.opened():
            result = __class__.__libhackrf.hackrf_set_lna_gain(self.__pDevice, gain)
            # if (result == LibHackRfReturnCode.HACKRF_SUCCESS):
            #    self.__lnaGain = gain
        else:
            __class__.__logger.debug("Trying to set LNA gain for non-opened " + __class__.__name__)
        return result

    # def getLNAGain(self):
    #    return self.__lnaGain

    def setVGAGain(self, gain):
        __class__.__logger.debug(__class__.__name__ + " Trying to set VGA gain")
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if self.opened():
            result = __class__.__libhackrf.hackrf_set_vga_gain(self.__pDevice, gain)
            # if (result == LibHackRfReturnCode.HACKRF_SUCCESS):
            #    self.__vgaGain = gain
        else:
            __class__.__logger.debug("Trying to set VGA gain for non-opened " + __class__.__name__)
        return result

    # def getVGAGain(self):
    #    return self.__vgaGain

    def setTXVGAGain(self, gain):
        __class__.__logger.debug(__class__.__name__ + " Trying to set TX VGA gain")
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if self.opened():
            result = __class__.__libhackrf.hackrf_set_txvga_gain(self.__pDevice, gain)
            # if (result == LibHackRfReturnCode.HACKRF_SUCCESS):
            #    self.__txvgaGain = gain
        else:
            __class__.__logger.debug("Trying to set TX VGA gain for non-opened " + __class__.__name__)
        return result

    # def getTXVGAGain(self):
    #    return self.__txvgaGain

    def setBasebandFilterBandwidth(self, bandwidth):
        __class__.__logger.debug(__class__.__name__ + " Trying to set baseband filter bandwidth")
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if self.opened():
            result = __class__.__libhackrf.hackrf_set_baseband_filter_bandwidth(self.__pDevice, bandwidth)
            # if (result == LibHackRfReturnCode.HACKRF_SUCCESS):
            #    self.__basebandFilterBandwidth = bandwidth
        else:
            __class__.__logger.debug("Trying to set baseband filter bandwidth " + __class__.__name__)
        return result

    # def getBasebandFilterBandwidth(self):
    #    return self.__basebandFilterBandwidth

    def setFrequency(self, frequency):
        __class__.__logger.debug(__class__.__name__ + " Trying to set frequency")
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if self.opened():
            result = __class__.__libhackrf.hackrf_set_freq(self.__pDevice, self.__correctFrequency(frequency))
            # if (result == LibHackRfReturnCode.HACKRF_SUCCESS):
            #    self.__frequency = frequency
        else:
            __class__.__logger.debug("Trying to set frequency " + __class__.__name__)
        return result

    def __correctFrequency(self, frequency):
        return int(frequency * (1.0 - (self.__crystalppm / 1000000.0)))

    def __correctSampleRate(self, samplerate):
        #
        # Not sure why the +0.5 is there. I copied it from hackrf_transfer
        # equivalent source code but this is probably not necessary, especially
        # because there is already a +0.5 done in hackrf.c hackrf_set_sample_rate
        #
        return int(samplerate * (1.0 - (self.__crystalppm / 1000000.0)) + 0.5)

    # def getFrequency(self):
    #    return self.__frequency

    def setFrequencyExplicit(self, if_frequency, lo_frequency, rf_path):
        __class__.__logger.debug(__class__.__name__ + " Trying to set frequency with details")
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if self.opened():
            result = __class__.__libhackrf.hackrf_set_freq_explicit(self.__pDevice,
                                                                    self.__correctFrequency(if_frequency),
                                                                    self.__correctFrequency(lo_frequency),
                                                                    LibHackRfPathFilter(rf_path).value)
            # if (result == LibHackRfReturnCode.HACKRF_SUCCESS):
            #    self.__ifFrequency = if_frequency
            #    self.__loFrequency = lo_frequency
            #    self.__rfFilterPath = LibHackRfPathFilter(rf_path)
        else:
            __class__.__logger.debug("Trying to set frequency with details " + __class__.__name__)
        return result

    # def getIntermediateFrequency(self):
    #    return self.__ifFrequency

    # def getLocalOscillatorFrequency(self):
    #    return self.__loFrequency

    # def getRfFilterPath(self):
    #    return self.__rfFilterPath

    #
    #   This method should be called before setting frequency or baseband as in state
    #   it acts by modifying the values passed to libhackrf functions
    #
    def setCrystalPPM(self, ppm):
        __class__.__logger.debug("This method must be called before setting frequency or samplerate")
        self.__crystalppm = ppm

    def getCrystalPPM(self):
        return self.__crystalppm

    def setSampleRate(self, samplerate):
        __class__.__logger.debug(__class__.__name__ + " Trying to set samplerate")
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if self.opened():
            result = __class__.__libhackrf.hackrf_set_sample_rate(self.__pDevice, self.__correctSampleRate(samplerate))
            # if (result == LibHackRfReturnCode.HACKRF_SUCCESS):
            #    self.__sampleRate = samplerate
        else:
            __class__.__logger.debug("Trying to set samplerate " + __class__.__name__)
        return result

    def setSampleRateManual(self, samplerate, divider):
        __class__.__logger.debug(__class__.__name__ + " Trying to set samplerate")
        result = LibHackRfReturnCode.HACKRF_ERROR_OTHER
        if self.opened():
            result = __class__.__libhackrf.hackrf_set_sample_rate_manual(self.__pDevice,
                                                                         self.__correctSampleRate(samplerate), divider)
            # if (result == LibHackRfReturnCode.HACKRF_SUCCESS):
            #    self.__sampleRate = samplerate
        else:
            __class__.__logger.debug("Trying to set samplerate " + __class__.__name__)
        return result
