#!/usr/bin/env python3
"""
Physical test for FPGA bitstream guard on HackRF Pro.

The Pro firmware should reject FPGA bitstream switches while the
transceiver is in RX or TX mode (opmode != OFF).
"""

import argparse
import ctypes
import ctypes.util
import os
import sys


def _resolve_lib():
    """Return path to libhackrf, using args > env > find_library > default."""
    default = "/Users/ivo/hackrf/host/build/libhackrf/src/libhackrf.dylib"
    path = os.environ.get("HACKRF_LIB")
    if path:
        return path
    found = ctypes.util.find_library("hackrf")
    if found:
        return found
    return default


def _resolve_serial(args):
    """Return Pro serial from args/env/default."""
    default = b"645061de252d6613"
    serial = args.serial_pro or os.environ.get("HACKRF_SERIAL_PRO", "")
    return serial.encode() if serial else default


HACKRF_SUCCESS = 0


def error_string(code: int) -> str:
    name = libhackrf.hackrf_error_name(code)
    return name.decode("utf-8") if name else f"unknown({code})"


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Physical test for FPGA bitstream guard on HackRF Pro"
    )
    parser.add_argument(
        "--lib",
        default=None,
        help="Path to libhackrf (default: $HACKRF_LIB or ctypes.util.find_library('hackrf'))",
    )
    parser.add_argument(
        "--serial-pro",
        default=None,
        help="Serial number of HackRF Pro (default: $HACKRF_SERIAL_PRO or built-in default)",
    )
    args = parser.parse_args()

    lib_path = args.lib or _resolve_lib()
    SERIAL = _resolve_serial(args)

    global libhackrf
    try:
        libhackrf = ctypes.CDLL(lib_path)
    except OSError as e:
        print(f"FAIL: Could not load libhackrf from {lib_path}: {e}")
        sys.exit(1)

    libhackrf.hackrf_init.argtypes = []
    libhackrf.hackrf_init.restype = ctypes.c_int

    libhackrf.hackrf_open_by_serial.argtypes = [
        ctypes.c_char_p,
        ctypes.POINTER(ctypes.c_void_p),
    ]
    libhackrf.hackrf_open_by_serial.restype = ctypes.c_int

    libhackrf.hackrf_close.argtypes = [ctypes.c_void_p]
    libhackrf.hackrf_close.restype = ctypes.c_int

    libhackrf.hackrf_exit.argtypes = []
    libhackrf.hackrf_exit.restype = ctypes.c_int

    libhackrf.hackrf_error_name.argtypes = [ctypes.c_int]
    libhackrf.hackrf_error_name.restype = ctypes.c_char_p

    libhackrf.hackrf_sync_start.argtypes = [ctypes.c_void_p, ctypes.c_uint8]
    libhackrf.hackrf_sync_start.restype = ctypes.c_int

    libhackrf.hackrf_set_fpga_bitstream.argtypes = [ctypes.c_void_p, ctypes.c_uint8]
    libhackrf.hackrf_set_fpga_bitstream.restype = ctypes.c_int

    print("=" * 70)
    print("FPGA bitstream guard test")
    print("=" * 70)
    print()

    result = libhackrf.hackrf_init()
    if result != HACKRF_SUCCESS:
        print(f"FAIL: hackrf_init() failed: {result} ({error_string(result)})")
        return 1
    print("hackrf_init() OK")

    device_ptr = ctypes.c_void_p()
    result = libhackrf.hackrf_open_by_serial(SERIAL, ctypes.byref(device_ptr))
    if result != HACKRF_SUCCESS:
        print(f"FAIL: Could not open device: {result} ({error_string(result)})")
        libhackrf.hackrf_exit()
        return 1
    print(f"Opened HackRF Pro (serial={SERIAL.decode()})")

    # Ensure OFF mode
    result = libhackrf.hackrf_sync_start(device_ptr, 0)
    if result != HACKRF_SUCCESS:
        print(f"WARNING: reset to OFF failed: {result} ({error_string(result)})")
    else:
        print("Device is in OFF mode")

    # Start RX mode
    result = libhackrf.hackrf_sync_start(device_ptr, 1)
    if result != HACKRF_SUCCESS:
        print(f"FAIL: sync_start RX failed: {result} ({error_string(result)})")
        libhackrf.hackrf_close(device_ptr)
        libhackrf.hackrf_exit()
        return 1
    print("Started RX mode")

    # Attempt FPGA bitstream switch while in RX mode
    bitstream_index = 0
    result = libhackrf.hackrf_set_fpga_bitstream(device_ptr, bitstream_index)
    print(
        f"hackrf_set_fpga_bitstream(index={bitstream_index}) while RX: "
        f"result={result} ({error_string(result)})"
    )

    if result == HACKRF_SUCCESS:
        print("FAIL: Bitstream switch was accepted while in RX mode (guard not working)")
        guard_passed = False
    else:
        print("PASS: Bitstream switch rejected while in RX mode")
        guard_passed = True

    success_path_passed = False

    # Reset to OFF mode
    result_off = libhackrf.hackrf_sync_start(device_ptr, 0)
    if result_off != HACKRF_SUCCESS:
        print(f"WARNING: reset to OFF failed: {result_off} ({error_string(result_off)})")
    else:
        print("Device reset to OFF mode")

    # Success-path: switch should be accepted in OFF mode
    result = libhackrf.hackrf_set_fpga_bitstream(device_ptr, 0)
    print(
        f"hackrf_set_fpga_bitstream(index=0) while OFF: "
        f"result={result} ({error_string(result)})"
    )
    if result == HACKRF_SUCCESS:
        print("PASS: Bitstream switch accepted while in OFF mode")
        success_path_passed = True
    else:
        print("FAIL: Bitstream switch rejected while in OFF mode")
        success_path_passed = False

    libhackrf.hackrf_close(device_ptr)
    print("Device closed")

    libhackrf.hackrf_exit()
    print("hackrf_exit() OK")
    print()
    print("=" * 70)
    if guard_passed and success_path_passed:
        print("FPGA bitstream guard + success-path test PASSED")
    else:
        print("FPGA bitstream guard + success-path test FAILED")
    print("=" * 70)
    return 0 if (guard_passed and success_path_passed) else 1


if __name__ == "__main__":
    sys.exit(main())
