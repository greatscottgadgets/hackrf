#!/usr/bin/env python3
"""
Coherence validation test for synchronized HackRF devices.

This test validates two levels of synchronization:

1. CLOCK COHERENCE: Pro CLKOUT → One CLKIN is detected and stable.
   This is verified by enabling CLKOUT on the Pro and checking that
   the One reports "clock signal detected".

2. TRIGGER COHERENCE: Both devices armed with sync_start(RX) will
   begin sampling on the same trigger edge. This requires:
   - Shared clock (Pro CLKOUT → One CLKIN) ✓
   - Trigger line: Pro trigger_out → One trigger_in (optional)

For a full end-to-end proof with actual IQ captures, you need:
   - A signal splitter feeding both devices from the same source, OR
   - Both antennas very close together receiving the same strong signal

Without that, the cross-correlation of ambient noise will not peak.
"""

import ctypes
import subprocess
import sys

LIBHACKRF_PATH = "/Users/ivo/hackrf/host/build/libhackrf/src/libhackrf.dylib"
HACKRF_TRANSFER = "/Users/ivo/hackrf/host/build/hackrf-tools/src/hackrf_transfer"
HACKRF_CLOCK = "/Users/ivo/hackrf/host/build/hackrf-tools/src/hackrf_clock"

ONE_SERIAL = b"922c63dc21748847"
PRO_SERIAL = b"645061de252d6613"

HACKRF_SUCCESS = 0

try:
    libhackrf = ctypes.CDLL(LIBHACKRF_PATH)
except OSError as e:
    print(f"FAIL: Could not load libhackrf: {e}")
    sys.exit(1)

libhackrf.hackrf_init.argtypes = []
libhackrf.hackrf_init.restype = ctypes.c_int

libhackrf.hackrf_open_by_serial.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_void_p)]
libhackrf.hackrf_open_by_serial.restype = ctypes.c_int

libhackrf.hackrf_close.argtypes = [ctypes.c_void_p]
libhackrf.hackrf_close.restype = ctypes.c_int

libhackrf.hackrf_exit.argtypes = []
libhackrf.hackrf_exit.restype = ctypes.c_int

libhackrf.hackrf_sync_start.argtypes = [ctypes.c_void_p, ctypes.c_uint8]
libhackrf.hackrf_sync_start.restype = ctypes.c_int

libhackrf.hackrf_set_hw_sync_mode.argtypes = [ctypes.c_void_p, ctypes.c_uint8]
libhackrf.hackrf_set_hw_sync_mode.restype = ctypes.c_int


def run_cmd(cmd: list) -> tuple:
    p = subprocess.run(cmd, capture_output=True, text=True)
    return p.returncode, p.stdout, p.stderr


def test_clkin_detection() -> bool:
    """Verify that One detects clock from Pro."""
    print("=" * 70)
    print("Test 1: CLKIN Detection (shared clock validation)")
    print("=" * 70)

    # Check Pro CLKOUT status
    rc, out, err = run_cmd([HACKRF_CLOCK, "-i", "-d", PRO_SERIAL.decode()])
    print(f"  Pro CLKIN status: {out.strip()}")

    # Enable CLKOUT on Pro
    print("  Enabling Pro CLKOUT...")
    rc, out, err = run_cmd([HACKRF_CLOCK, "-o", "1", "-d", PRO_SERIAL.decode()])
    if rc != 0:
        print(f"  FAIL: Could not enable CLKOUT: {err.strip()}")
        return False

    # Check One CLKIN status
    rc, out, err = run_cmd([HACKRF_CLOCK, "-i", "-d", ONE_SERIAL.decode()])
    status = out.strip()
    print(f"  One CLKIN status: {status}")

    if "clock signal detected" in status:
        print("  ✅ PASS — One detects Pro CLKOUT. Sample clocks are locked.")
        return True
    else:
        print("  ❌ FAIL — No clock detected. Check CLKIN cable.")
        return False


def test_sync_start_trigger_arm() -> bool:
    """Verify that sync_start arms the trigger and rejects invalid modes."""
    print("\n" + "=" * 70)
    print("Test 2: sync_start Trigger Arm / Disarm")
    print("=" * 70)

    result = libhackrf.hackrf_init()
    if result != HACKRF_SUCCESS:
        print(f"  FAIL: hackrf_init() failed")
        return False

    one = ctypes.c_void_p()
    pro = ctypes.c_void_p()

    if libhackrf.hackrf_open_by_serial(ONE_SERIAL, ctypes.byref(one)) != HACKRF_SUCCESS:
        print("  FAIL: Could not open One")
        return False
    if libhackrf.hackrf_open_by_serial(PRO_SERIAL, ctypes.byref(pro)) != HACKRF_SUCCESS:
        print("  FAIL: Could not open Pro")
        libhackrf.hackrf_close(one)
        return False

    all_pass = True

    # Test: arm both devices
    print("  Arming One (RX + trigger)...", end=" ")
    rc = libhackrf.hackrf_sync_start(one, 1)
    print("PASS" if rc == HACKRF_SUCCESS else f"FAIL ({rc})")
    if rc != HACKRF_SUCCESS:
        all_pass = False

    print("  Arming Pro (RX + trigger)...", end=" ")
    rc = libhackrf.hackrf_sync_start(pro, 1)
    print("PASS" if rc == HACKRF_SUCCESS else f"FAIL ({rc})")
    if rc != HACKRF_SUCCESS:
        all_pass = False

    # Test: disarm both devices
    print("  Disarming One (OFF)...", end=" ")
    rc = libhackrf.hackrf_sync_start(one, 0)
    print("PASS" if rc == HACKRF_SUCCESS else f"FAIL ({rc})")
    if rc != HACKRF_SUCCESS:
        all_pass = False

    print("  Disarming Pro (OFF)...", end=" ")
    rc = libhackrf.hackrf_sync_start(pro, 0)
    print("PASS" if rc == HACKRF_SUCCESS else f"FAIL ({rc})")
    if rc != HACKRF_SUCCESS:
        all_pass = False

    # Test: invalid mode rejected
    print("  Invalid mode (255) on One...", end=" ")
    rc = libhackrf.hackrf_sync_start(one, 255)
    print("PASS (rejected)" if rc != HACKRF_SUCCESS else "FAIL (accepted)")
    if rc == HACKRF_SUCCESS:
        all_pass = False

    libhackrf.hackrf_close(one)
    libhackrf.hackrf_close(pro)
    libhackrf.hackrf_exit()

    if all_pass:
        print("  ✅ PASS — sync_start arms/disarms correctly on both devices")
    else:
        print("  ❌ FAIL — some sync_start operations failed")
    return all_pass


def test_triggered_capture_note() -> bool:
    """Document what is needed for full triggered-capture validation."""
    print("\n" + "=" * 70)
    print("Test 3: Triggered Capture Coherence (IQ phase validation)")
    print("=" * 70)
    print("""
  This test requires additional hardware setup:

  1. SIGNAL SOURCE
     - A strong CW tone or FM station fed to BOTH devices via a splitter
     - OR: Pro transmitting CW on one antenna, both receiving on another

  2. TRIGGER LINE (optional but recommended for full validation)
     - Pro P2 set to trigger_out:  hackrf_clock -2 trigger_out
     - One P20 (trigger_in) connected to Pro P2 via coax/jumper

  3. VALIDATION PROCEDURE
     a. Enable Pro CLKOUT:  hackrf_clock -o 1 -d <pro_serial>
     b. Verify One CLKIN:   hackrf_clock -i -d <one_serial>
     c. Arm both devices:   hackrf_sync_start(RX) on both
     d. If trigger line wired, pulse it (or use hackrf_transfer -H)
     e. Capture IQ from both simultaneously
     f. Cross-correlate — peak delay should be 0±1 samples
     g. Peak phase should be constant across repeated captures

  Without the signal splitter, ambient noise captures will not
  correlate. Test 1 (CLKIN detection) and Test 2 (trigger arm)
  together prove the synchronization infrastructure is functional.
""")
    return True


def main() -> int:
    print("HackRF Coherence Validation Test")
    print(f"One: {ONE_SERIAL.decode()}, Pro: {PRO_SERIAL.decode()}")
    print()

    p1 = test_clkin_detection()
    p2 = test_sync_start_trigger_arm()
    p3 = test_triggered_capture_note()

    print("\n" + "=" * 70)
    if p1 and p2:
        print("OVERALL: PASS — Clock shared and trigger arm verified")
        print("=" * 70)
        print("\nNext step for full validation:")
        print("  - Feed a strong signal to both devices via splitter")
        print("  - Run simultaneous triggered captures")
        print("  - Verify cross-correlation peak is stable")
        return 0
    else:
        print("OVERALL: FAIL")
        print("=" * 70)
        return 1


if __name__ == "__main__":
    sys.exit(main())
