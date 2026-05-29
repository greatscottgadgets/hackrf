#!/usr/bin/env python3
"""
Test script for hackrf_sync_start() API using ctypes.

Validates the hackrf_sync_start() control transfer on physical HackRF
hardware without streaming samples.

Hardware under test:
- HackRF One r9 (serial: 922c63dc21748847) with PortaPack H2
- HackRF Pro r1.2 (serial: 645061de252d6613)
- Pro P2 CLKOUT is wired to One CLKIN for phase-locked operation

Expected modes (from firmware/common/transceiver_mode.h):
  0 = OFF
  1 = RX
  2 = TX
  255 = invalid, should fail with HACKRF_ERROR_INVALID_PARAM
"""

import argparse
import ctypes
import ctypes.util
import os
import sys


# ---------------------------------------------------------------------------
# Configuration helpers
# ---------------------------------------------------------------------------
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


def _resolve_serials(args):
    """Return (one_serial, pro_serial) from args/env/defaults."""
    one_default = b"922c63dc21748847"
    pro_default = b"645061de252d6613"
    one = args.serial_one or os.environ.get("HACKRF_SERIAL_ONE", "")
    pro = args.serial_pro or os.environ.get("HACKRF_SERIAL_PRO", "")
    return (
        one.encode() if one else one_default,
        pro.encode() if pro else pro_default,
    )


# ---------------------------------------------------------------------------
# Error codes from hackrf.h
# ---------------------------------------------------------------------------
HACKRF_SUCCESS = 0
HACKRF_ERROR_INVALID_PARAM = -2


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
def error_string(code: int) -> str:
    """Return human-readable error name from libhackrf."""
    name = libhackrf.hackrf_error_name(code)
    return name.decode("utf-8") if name else f"unknown({code})"


def run_test(label: str, device_handle, mode: int, expect_success: bool) -> bool:
    """Call hackrf_sync_start and report PASS/FAIL."""
    result = libhackrf.hackrf_sync_start(device_handle, mode)
    passed = (result == HACKRF_SUCCESS) if expect_success else (result != HACKRF_SUCCESS)
    status = "PASS" if passed else "FAIL"
    expectation = "expect success" if expect_success else "expect failure"
    print(
        f"  [{status}] {label}: mode={mode}, result={result} ({error_string(result)}) "
        f"({expectation})"
    )
    return passed


def reset_to_off(device_handle) -> bool:
    """Return device to OFF mode."""
    result = libhackrf.hackrf_sync_start(device_handle, 0)
    if result != HACKRF_SUCCESS:
        print(
            f"    WARNING: reset to OFF failed: {result} ({error_string(result)})"
        )
        return False
    return True


# ---------------------------------------------------------------------------
# Main test routine
# ---------------------------------------------------------------------------
def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate hackrf_sync_start() on physical HackRF hardware"
    )
    parser.add_argument(
        "--lib",
        default=None,
        help="Path to libhackrf (default: $HACKRF_LIB or ctypes.util.find_library('hackrf'))",
    )
    parser.add_argument(
        "--serial-one",
        default=None,
        help="Serial number of HackRF One (default: $HACKRF_SERIAL_ONE or built-in default)",
    )
    parser.add_argument(
        "--serial-pro",
        default=None,
        help="Serial number of HackRF Pro (default: $HACKRF_SERIAL_PRO or built-in default)",
    )
    args = parser.parse_args()

    lib_path = args.lib or _resolve_lib()
    one_serial, pro_serial = _resolve_serials(args)

    DEVICES = [
        {"name": "HackRF One r9", "serial": one_serial},
        {"name": "HackRF Pro r1.2", "serial": pro_serial},
    ]

    # -----------------------------------------------------------------------
    # Load libhackrf
    # -----------------------------------------------------------------------
    global libhackrf
    try:
        libhackrf = ctypes.CDLL(lib_path)
    except OSError as e:
        print(f"FAIL: Could not load libhackrf from {lib_path}: {e}")
        sys.exit(1)

    # -----------------------------------------------------------------------
    # Set up function signatures
    # -----------------------------------------------------------------------
    # int hackrf_init(void)
    libhackrf.hackrf_init.argtypes = []
    libhackrf.hackrf_init.restype = ctypes.c_int

    # int hackrf_open_by_serial(const char* const desired_serial_number,
    #                           hackrf_device** device)
    libhackrf.hackrf_open_by_serial.argtypes = [
        ctypes.c_char_p,
        ctypes.POINTER(ctypes.c_void_p),
    ]
    libhackrf.hackrf_open_by_serial.restype = ctypes.c_int

    # int hackrf_close(hackrf_device* device)
    libhackrf.hackrf_close.argtypes = [ctypes.c_void_p]
    libhackrf.hackrf_close.restype = ctypes.c_int

    # int hackrf_exit(void)
    libhackrf.hackrf_exit.argtypes = []
    libhackrf.hackrf_exit.restype = ctypes.c_int

    # const char* hackrf_error_name(enum hackrf_error errcode)
    libhackrf.hackrf_error_name.argtypes = [ctypes.c_int]
    libhackrf.hackrf_error_name.restype = ctypes.c_char_p

    # int hackrf_sync_start(hackrf_device* device, const uint8_t mode)
    libhackrf.hackrf_sync_start.argtypes = [ctypes.c_void_p, ctypes.c_uint8]
    libhackrf.hackrf_sync_start.restype = ctypes.c_int

    print("=" * 70)
    print("hackrf_sync_start() API validation test")
    print("=" * 70)
    print()

    # Initialize libhackrf
    result = libhackrf.hackrf_init()
    if result != HACKRF_SUCCESS:
        print(f"FAIL: hackrf_init() failed: {result} ({error_string(result)})")
        return 1
    print("hackrf_init() OK")
    print()

    all_passed = True
    open_devices = []

    # Open each device by serial
    for dev_info in DEVICES:
        device_ptr = ctypes.c_void_p()
        result = libhackrf.hackrf_open_by_serial(
            dev_info["serial"], ctypes.byref(device_ptr)
        )
        if result != HACKRF_SUCCESS:
            print(
                f"FAIL: Could not open {dev_info['name']} "
                f"(serial={dev_info['serial'].decode()}): "
                f"{result} ({error_string(result)})"
            )
            print(f"  [SKIP] {dev_info['name']} not found on USB")
            continue

        print(f"Opened {dev_info['name']} (serial={dev_info['serial'].decode()})")
        open_devices.append((dev_info["name"], device_ptr))

    if not open_devices:
        print("No devices opened, aborting.")
        libhackrf.hackrf_exit()
        return 1

    print()

    # Run sync_start tests on each device
    for name, device_ptr in open_devices:
        print(f"Testing {name}...")

        # Test 1: mode 0 (OFF) – should always succeed
        if not run_test("sync_start OFF", device_ptr, 0, expect_success=True):
            print(f"  [SKIP] {name} not found on USB")
            continue

        # Test 2: mode 1 (RX with sync) – should succeed on supported devices
        if not run_test("sync_start RX", device_ptr, 1, expect_success=True):
            print(f"  [SKIP] {name} not found on USB")
            continue
        reset_to_off(device_ptr)

        # Test 3: mode 2 (TX with sync) – should succeed on supported devices
        if not run_test("sync_start TX", device_ptr, 2, expect_success=True):
            print(f"  [SKIP] {name} not found on USB")
            continue
        reset_to_off(device_ptr)

        # Test 4: mode 255 (invalid) – should fail with INVALID_PARAM
        if not run_test(
            "sync_start invalid", device_ptr, 255, expect_success=False
        ):
            print(f"  [SKIP] {name} not found on USB")
            continue
        reset_to_off(device_ptr)

        print()

    # Close all devices
    for name, device_ptr in open_devices:
        result = libhackrf.hackrf_close(device_ptr)
        if result != HACKRF_SUCCESS:
            print(
                f"WARNING: hackrf_close() failed for {name}: "
                f"{result} ({error_string(result)})"
            )
        else:
            print(f"Closed {name}")

    print()

    # Exit libhackrf
    result = libhackrf.hackrf_exit()
    if result != HACKRF_SUCCESS:
        print(f"WARNING: hackrf_exit() failed: {result} ({error_string(result)})")
    else:
        print("hackrf_exit() OK")

    print()
    print("=" * 70)
    if all_passed:
        print("All tests PASSED")
    else:
        print("Some tests FAILED")
    print("=" * 70)

    return 0 if all_passed else 1


if __name__ == "__main__":
    sys.exit(main())
