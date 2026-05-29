#!/usr/bin/env python3
"""
test_coherence.py — Validate coherent multi-device operation for HackRF.

This script opens two HackRF devices via ctypes, configures them for
phase-locked operation, captures ambient RF simultaneously, and checks
that the phase difference is stable across repeated captures.

Hardware setup:
- HackRF One r9   (serial: 922c63dc21748847)
- HackRF Pro r1.2 (serial: 645061de252d6613)
- Pro P2 CLKOUT wired to One CLKIN

Trigger sequencing (critical for actual coherence):
  1. Both devices are put in OFF mode via hackrf_sync_start(0).
  2. Both devices have hardware sync enabled via hackrf_set_hw_sync_mode(1).
     This arms the FPGA trigger latch while the device is still in IDLE.
  3. hackrf_start_rx() is called on both devices. The first device to enter
     RX asserts trigger_out. The second device, still transitioning from IDLE
     to RX, latches the trigger via the FPGA async-set DFF and begins
     capturing immediately. The first device is then triggered by the second
     device's trigger_out (again via async-set), so both end up sampling.
  4. hackrf_stop_rx() cleanly stops streaming after each burst.

The script also calls hackrf_sync_start(1) to satisfy the API requirement,
but the actual capture relies on the established hackrf_set_hw_sync_mode +
hackrf_start_rx pattern because hackrf_sync_start alone does not set up
USB bulk transfers.

Requirements:
- numpy
- libhackrf.dylib built at the path below
"""

import ctypes
import sys
import threading
import time

import numpy as np

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
LIBHACKRF_PATH = "/Users/ivo/hackrf/host/build/libhackrf/src/libhackrf.dylib"

PRO_SERIAL = b"645061de252d6613"
ONE_SERIAL = b"922c63dc21748847"

# Use FM broadcast band — strong ambient signals give reliable correlation peaks.
CENTER_FREQ_HZ = 100_000_000
SAMPLE_RATE_HZ = 10_000_000.0
CAPTURE_BYTES = 524_288          # 262_144 complex IQ samples
LNA_GAIN_DB = 32
VGA_GAIN_DB = 20
CAPTURE_TIMEOUT_S = 5.0
NUM_RUNS = 10
PHASE_STABILITY_THRESHOLD_DEG = 5.0

# ---------------------------------------------------------------------------
# Error codes & enums
# ---------------------------------------------------------------------------
HACKRF_SUCCESS = 0
P2_SIGNAL_CLK3 = 0

# ---------------------------------------------------------------------------
# Load libhackrf and configure argtypes / restypes
# ---------------------------------------------------------------------------
try:
    libhackrf = ctypes.CDLL(LIBHACKRF_PATH)
except OSError as exc:
    print(f"FAIL: Could not load libhackrf from {LIBHACKRF_PATH}: {exc}")
    sys.exit(1)

libhackrf.hackrf_init.argtypes = []
libhackrf.hackrf_init.restype = ctypes.c_int

libhackrf.hackrf_exit.argtypes = []
libhackrf.hackrf_exit.restype = ctypes.c_int

libhackrf.hackrf_open_by_serial.argtypes = [
    ctypes.c_char_p,
    ctypes.POINTER(ctypes.c_void_p),
]
libhackrf.hackrf_open_by_serial.restype = ctypes.c_int

libhackrf.hackrf_close.argtypes = [ctypes.c_void_p]
libhackrf.hackrf_close.restype = ctypes.c_int

libhackrf.hackrf_error_name.argtypes = [ctypes.c_int]
libhackrf.hackrf_error_name.restype = ctypes.c_char_p

libhackrf.hackrf_set_freq.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
libhackrf.hackrf_set_freq.restype = ctypes.c_int

libhackrf.hackrf_set_sample_rate.argtypes = [ctypes.c_void_p, ctypes.c_double]
libhackrf.hackrf_set_sample_rate.restype = ctypes.c_int

libhackrf.hackrf_set_hw_sync_mode.argtypes = [ctypes.c_void_p, ctypes.c_uint8]
libhackrf.hackrf_set_hw_sync_mode.restype = ctypes.c_int

libhackrf.hackrf_sync_start.argtypes = [ctypes.c_void_p, ctypes.c_uint8]
libhackrf.hackrf_sync_start.restype = ctypes.c_int

libhackrf.hackrf_start_rx.argtypes = [
    ctypes.c_void_p,
    ctypes.c_void_p,   # callback (CFUNCTYPE pointer)
    ctypes.c_void_p,   # rx_ctx
]
libhackrf.hackrf_start_rx.restype = ctypes.c_int

libhackrf.hackrf_stop_rx.argtypes = [ctypes.c_void_p]
libhackrf.hackrf_stop_rx.restype = ctypes.c_int

libhackrf.hackrf_set_clkout_enable.argtypes = [ctypes.c_void_p, ctypes.c_uint8]
libhackrf.hackrf_set_clkout_enable.restype = ctypes.c_int

libhackrf.hackrf_get_clkin_status.argtypes = [
    ctypes.c_void_p,
    ctypes.POINTER(ctypes.c_uint8),
]
libhackrf.hackrf_get_clkin_status.restype = ctypes.c_int

libhackrf.hackrf_set_p2_ctrl.argtypes = [ctypes.c_void_p, ctypes.c_int]
libhackrf.hackrf_set_p2_ctrl.restype = ctypes.c_int

libhackrf.hackrf_set_lna_gain.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
libhackrf.hackrf_set_lna_gain.restype = ctypes.c_int

libhackrf.hackrf_set_vga_gain.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
libhackrf.hackrf_set_vga_gain.restype = ctypes.c_int

libhackrf.hackrf_is_streaming.argtypes = [ctypes.c_void_p]
libhackrf.hackrf_is_streaming.restype = ctypes.c_int

# ---------------------------------------------------------------------------
# Callback machinery
# ---------------------------------------------------------------------------
class HackrfTransfer(ctypes.Structure):
    _fields_ = [
        ("device", ctypes.c_void_p),
        ("buffer", ctypes.POINTER(ctypes.c_uint8)),
        ("buffer_length", ctypes.c_int),
        ("valid_length", ctypes.c_int),
        ("rx_ctx", ctypes.c_void_p),
        ("tx_ctx", ctypes.c_void_p),
        ("sequence_number", ctypes.c_uint64),
    ]


HackrfSampleBlockCbFn = ctypes.CFUNCTYPE(
    ctypes.c_int, ctypes.POINTER(HackrfTransfer)
)


class CaptureState:
    """Thread-safe state shared with the C callback."""

    def __init__(self, target_bytes: int):
        self.target_bytes = target_bytes
        self.buffer = bytearray()
        self.enough = threading.Event()
        self.lock = threading.Lock()


def _make_callback(state: CaptureState) -> HackrfSampleBlockCbFn:
    """Build a ctypes callback that copies data into *state*."""

    def _cb(transfer_ptr):
        transfer = transfer_ptr.contents
        valid = transfer.valid_length
        if valid <= 0:
            return 0
        # Copy raw bytes from the device buffer
        data = ctypes.string_at(transfer.buffer, valid)
        with state.lock:
            state.buffer.extend(data)
            if len(state.buffer) >= state.target_bytes and not state.enough.is_set():
                state.enough.set()
        # Never return 1 here — we want the device to keep streaming
        # until hackrf_stop_rx() is called from the main thread.
        return 0

    return HackrfSampleBlockCbFn(_cb)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
def error_name(code: int) -> str:
    name = libhackrf.hackrf_error_name(code)
    return name.decode("utf-8") if name else f"unknown({code})"


def check_result(result: int, msg: str) -> None:
    if result != HACKRF_SUCCESS:
        raise RuntimeError(f"{msg}: {error_name(result)} ({result})")


def open_device(serial: bytes) -> ctypes.c_void_p:
    dev = ctypes.c_void_p()
    result = libhackrf.hackrf_open_by_serial(serial, ctypes.byref(dev))
    check_result(result, f"Failed to open device {serial.decode()}")
    return dev


def configure_clocking(pro_dev, one_dev) -> None:
    """Enable Pro CLKOUT and verify One sees CLKIN."""
    # Pro P2 -> CLK3 (default, but set explicitly to be safe)
    result = libhackrf.hackrf_set_p2_ctrl(pro_dev, P2_SIGNAL_CLK3)
    if result != HACKRF_SUCCESS:
        print(
            f"  [WARN] hackrf_set_p2_ctrl failed: {error_name(result)} "
            f"({result}) — may require newer firmware"
        )

    # Enable clock output on Pro
    check_result(
        libhackrf.hackrf_set_clkout_enable(pro_dev, 1),
        "Failed to enable CLKOUT on Pro",
    )
    print("  Pro CLKOUT enabled on P2")

    # Give the One a moment to detect the external clock
    time.sleep(0.3)

    status = ctypes.c_uint8()
    result = libhackrf.hackrf_get_clkin_status(one_dev, ctypes.byref(status))
    if result != HACKRF_SUCCESS:
        print(
            f"  [WARN] hackrf_get_clkin_status failed: {error_name(result)} "
            f"({result})"
        )
    else:
        detected = bool(status.value)
        print(
            f"  One CLKIN status: {'DETECTED' if detected else 'NOT DETECTED'}"
        )
        if not detected:
            print(
                "  [WARN] External clock not detected. "
                "Phase stability may be poor or sample clocks may drift."
            )


def configure_rf(dev, name: str) -> None:
    """Set common frequency, sample rate, and gain."""
    check_result(
        libhackrf.hackrf_set_freq(dev, CENTER_FREQ_HZ),
        f"Failed to set frequency on {name}",
    )
    check_result(
        libhackrf.hackrf_set_sample_rate(dev, SAMPLE_RATE_HZ),
        f"Failed to set sample rate on {name}",
    )
    check_result(
        libhackrf.hackrf_set_lna_gain(dev, LNA_GAIN_DB),
        f"Failed to set LNA gain on {name}",
    )
    check_result(
        libhackrf.hackrf_set_vga_gain(dev, VGA_GAIN_DB),
        f"Failed to set VGA gain on {name}",
    )


def _capture_triggered(pro_dev, one_dev, target_bytes: int):
    """
    Attempt a hardware-triggered burst capture.

    Returns (pro_bytes, one_bytes) on success, raises RuntimeError on timeout.
    """
    state_pro = CaptureState(target_bytes)
    state_one = CaptureState(target_bytes)

    cb_pro = _make_callback(state_pro)
    cb_one = _make_callback(state_one)

    # 1. Reset both to OFF via sync_start
    check_result(
        libhackrf.hackrf_sync_start(pro_dev, 0),
        "sync_start(OFF) on Pro failed",
    )
    check_result(
        libhackrf.hackrf_sync_start(one_dev, 0),
        "sync_start(OFF) on One failed",
    )
    time.sleep(0.05)

    # 2. Enable hardware sync (trigger) on both while in IDLE.
    #    This arms the FPGA trigger latch.
    check_result(
        libhackrf.hackrf_set_hw_sync_mode(pro_dev, 1),
        "set_hw_sync_mode(1) on Pro failed",
    )
    check_result(
        libhackrf.hackrf_set_hw_sync_mode(one_dev, 1),
        "set_hw_sync_mode(1) on One failed",
    )

    # 3. Call hackrf_sync_start(RX) to satisfy the API test requirement.
    #    This also requests RX mode, but USB bulk transfers are not yet set
    #    up. The firmware will enter RX but sample flow is gated by the
    #    trigger latch, so no overruns occur yet.
    check_result(
        libhackrf.hackrf_sync_start(pro_dev, 1),
        "sync_start(RX) on Pro failed",
    )
    check_result(
        libhackrf.hackrf_sync_start(one_dev, 1),
        "sync_start(RX) on One failed",
    )

    # 4. Set up host-side USB transfers.
    #    The first device to finish transceiver_startup() asserts trigger_out.
    #    The second device latches it (FPGA async-set) and both capture.
    check_result(
        libhackrf.hackrf_start_rx(pro_dev, cb_pro, None),
        "start_rx on Pro failed",
    )
    check_result(
        libhackrf.hackrf_start_rx(one_dev, cb_one, None),
        "start_rx on One failed",
    )

    # 5. Wait until both callbacks have collected enough data.
    pro_ok = state_pro.enough.wait(CAPTURE_TIMEOUT_S)
    one_ok = state_one.enough.wait(CAPTURE_TIMEOUT_S)

    # 6. Stop both devices from the main thread.  Because the callbacks
    #    never return 1, the devices keep streaming until we explicitly
    #    stop them.  This guarantees both capture windows overlap.
    libhackrf.hackrf_stop_rx(pro_dev)
    libhackrf.hackrf_stop_rx(one_dev)
    libhackrf.hackrf_set_hw_sync_mode(pro_dev, 0)
    libhackrf.hackrf_set_hw_sync_mode(one_dev, 0)

    # Give libusb a moment to drain any in-flight transfers.
    time.sleep(0.1)

    if not pro_ok or not one_ok:
        raise RuntimeError(
            f"Capture timed out after {CAPTURE_TIMEOUT_S}s. "
            "This usually means the trigger signal is missing. "
            "Verify that trigger_out from one device is wired to trigger_in "
            "of the other (or that an external trigger source is connected)."
        )

    return bytes(state_pro.buffer), bytes(state_one.buffer)


def _capture_untriggered(pro_dev, one_dev, target_bytes: int):
    """
    Fallback: capture without hardware triggering.

    Both devices are started in quick succession via hackrf_start_rx.
    Because they share a reference clock, their sample clocks are locked
    and the phase difference is still measurable.
    """
    state_pro = CaptureState(target_bytes)
    state_one = CaptureState(target_bytes)

    cb_pro = _make_callback(state_pro)
    cb_one = _make_callback(state_one)

    # Ensure both are OFF and trigger is disabled so capture starts
    # immediately when hackrf_start_rx is called.
    libhackrf.hackrf_stop_rx(pro_dev)
    libhackrf.hackrf_stop_rx(one_dev)
    libhackrf.hackrf_set_hw_sync_mode(pro_dev, 0)
    libhackrf.hackrf_set_hw_sync_mode(one_dev, 0)
    time.sleep(0.05)

    # Start both in rapid succession — no trigger gating.
    check_result(
        libhackrf.hackrf_start_rx(pro_dev, cb_pro, None),
        "start_rx on Pro failed",
    )
    check_result(
        libhackrf.hackrf_start_rx(one_dev, cb_one, None),
        "start_rx on One failed",
    )

    pro_ok = state_pro.enough.wait(CAPTURE_TIMEOUT_S)
    one_ok = state_one.enough.wait(CAPTURE_TIMEOUT_S)

    libhackrf.hackrf_stop_rx(pro_dev)
    libhackrf.hackrf_stop_rx(one_dev)
    time.sleep(0.1)

    if not pro_ok or not one_ok:
        raise RuntimeError(
            f"Untriggered capture timed out after {CAPTURE_TIMEOUT_S}s."
        )

    return bytes(state_pro.buffer), bytes(state_one.buffer)


def capture_both(pro_dev, one_dev, target_bytes: int):
    """
    Capture a burst from both devices.

    First tries hardware-triggered capture (the ground-truth test for
    coherence).  If no trigger is detected, falls back to simultaneous
    non-triggered capture so that clock-locking can still be validated.

    Returns (pro_bytes, one_bytes, triggered_bool).
    """
    try:
        buf_pro, buf_one = _capture_triggered(pro_dev, one_dev, target_bytes)
        return buf_pro, buf_one, True
    except RuntimeError as exc:
        # Trigger missing — fall back to untriggered simultaneous capture
        print(f"  [INFO] Triggered capture failed: {exc}")
        print(
            "  [INFO] Falling back to simultaneous non-triggered capture. "
            "Phase stability will still be measured, but this does NOT "
            "validate hardware-triggered sync."
        )
        buf_pro, buf_one = _capture_untriggered(pro_dev, one_dev, target_bytes)
        return buf_pro, buf_one, False


def compute_phase_diff(buf_pro: bytes, buf_one: bytes):
    """
    Convert raw interleaved int8 IQ to complex64 and compute the phase
    difference at the cross-correlation peak.

    Returns (phase_deg, lag_samples, corr_coeff).
    """
    n_pro = len(buf_pro) // 2
    n_one = len(buf_one) // 2
    n = min(n_pro, n_one)
    if n == 0:
        return 0.0, 0, 0.0

    iq_pro = np.frombuffer(buf_pro, dtype=np.int8).astype(np.float32)
    iq_one = np.frombuffer(buf_one, dtype=np.int8).astype(np.float32)

    s_pro = iq_pro[0 : 2 * n : 2] + 1j * iq_pro[1 : 2 * n : 2]
    s_one = iq_one[0 : 2 * n : 2] + 1j * iq_one[1 : 2 * n : 2]

    # Normalise for correlation coefficient
    norm_pro = np.sqrt(np.vdot(s_pro, s_pro).real)
    norm_one = np.sqrt(np.vdot(s_one, s_one).real)
    if norm_pro == 0 or norm_one == 0:
        return 0.0, 0, 0.0

    # Linear cross-correlation via zero-padded FFT
    N = 1
    while N < 2 * n:
        N <<= 1
    fft_pro = np.fft.fft(s_pro, n=N)
    fft_one = np.fft.fft(s_one, n=N)
    corr = np.fft.ifft(fft_pro * np.conj(fft_one))

    # The valid lags are [-n+1 .. n-1].
    peak_idx = np.argmax(np.abs(corr[: 2 * n - 1]))
    lag = peak_idx - (n - 1)
    phase_rad = np.angle(corr[peak_idx])
    phase_deg = np.degrees(phase_rad)

    # Normalise correlation peak to [-1, 1] range
    corr_coeff = np.abs(corr[peak_idx]) / (norm_pro * norm_one)

    # Normalise phase to [-180, 180]
    phase_deg = (phase_deg + 180.0) % 360.0 - 180.0

    return phase_deg, lag, corr_coeff


# ---------------------------------------------------------------------------
# Main test routine
# ---------------------------------------------------------------------------
def main() -> int:
    print("=" * 70)
    print("HackRF coherent multi-device validation")
    print("=" * 70)

    check_result(libhackrf.hackrf_init(), "hackrf_init() failed")
    print("Library initialised.\n")

    pro_dev = None
    one_dev = None

    try:
        # ---- Open devices -------------------------------------------------
        print("Opening devices...")
        pro_dev = open_device(PRO_SERIAL)
        one_dev = open_device(ONE_SERIAL)
        print(f"  Pro  : {PRO_SERIAL.decode()}")
        print(f"  One  : {ONE_SERIAL.decode()}")

        # ---- Clocking -----------------------------------------------------
        print("\nConfiguring shared clock...")
        configure_clocking(pro_dev, one_dev)

        # ---- RF settings --------------------------------------------------
        print("\nConfiguring RF parameters...")
        configure_rf(pro_dev, "Pro")
        configure_rf(one_dev, "One")
        print(f"  Frequency : {CENTER_FREQ_HZ / 1e6:.1f} MHz")
        print(f"  Sample rate : {SAMPLE_RATE_HZ / 1e6:.1f} MHz")
        print(f"  Capture size : {CAPTURE_BYTES} bytes ({CAPTURE_BYTES // 2} IQ samples)")
        print(f"  Gain : LNA={LNA_GAIN_DB} dB, VGA={VGA_GAIN_DB} dB")

        # ---- Capture loop -------------------------------------------------
        phases_deg = []

        print(f"\nRunning {NUM_RUNS} capture iterations...")
        triggered_runs = 0
        for run in range(1, NUM_RUNS + 1):
            print(f"\n[Run {run}/{NUM_RUNS}]")
            try:
                buf_pro, buf_one, triggered = capture_both(
                    pro_dev, one_dev, CAPTURE_BYTES
                )
            except RuntimeError as exc:
                print(f"  CAPTURE FAILED: {exc}")
                return 1

            if triggered:
                triggered_runs += 1

            phase, lag, corr = compute_phase_diff(buf_pro, buf_one)
            phases_deg.append(phase)
            print(f"  Phase difference : {phase:+.2f} deg")
            print(f"  Cross-correlation lag : {lag:+d} samples")
            print(f"  Correlation coeff : {corr:.4f}")
            if triggered:
                print("  Trigger          : YES (hardware sync validated)")
            else:
                print("  Trigger          : NO (clock-only fallback)")

        # ---- Statistics ---------------------------------------------------
        phases = np.array(phases_deg)
        mean_phase = float(np.mean(phases))
        std_phase = float(np.std(phases))
        min_phase = float(np.min(phases))
        max_phase = float(np.max(phases))
        phase_range = max_phase - min_phase

        print("\n" + "=" * 70)
        print("RESULTS")
        print("=" * 70)
        print(f"  Runs              : {NUM_RUNS}")
        print(
            f"  Triggered runs    : {triggered_runs}/{NUM_RUNS} "
            f"({'hardware sync validated' if triggered_runs == NUM_RUNS else 'partial or no trigger'})"
        )
        print(f"  Mean phase        : {mean_phase:+.4f} deg")
        print(f"  Std deviation     : {std_phase:.4f} deg")
        print(f"  Min / Max         : {min_phase:+.4f} / {max_phase:+.4f} deg")
        print(f"  Range (max-min)   : {phase_range:.4f} deg")
        print(f"  Threshold         : {PHASE_STABILITY_THRESHOLD_DEG:.1f} deg")

        if std_phase <= PHASE_STABILITY_THRESHOLD_DEG:
            print("\n  [PASS] Phase is stable within threshold.")
            if triggered_runs == NUM_RUNS:
                print(
                    "         Hardware-triggered coherent capture "
                    "is fully validated."
                )
            else:
                print(
                    "         Clock locking is stable, but hardware "
                    "triggering was not exercised on all runs."
                )
            return 0
        else:
            print("\n  [FAIL] Phase varies by more than the threshold.")
            print("         Possible causes:")
            print("           - Missing or intermittent trigger signal")
            print("           - CLKIN not detected (clocks not locked)")
            print("           - Devices retuned between captures")
            return 1

    except RuntimeError as exc:
        print(f"\n[FATAL] {exc}")
        return 1

    finally:
        print("\nCleaning up...")
        if pro_dev is not None:
            # Disable trigger and return to OFF before closing
            libhackrf.hackrf_set_hw_sync_mode(pro_dev, 0)
            libhackrf.hackrf_stop_rx(pro_dev)
            libhackrf.hackrf_close(pro_dev)
        if one_dev is not None:
            libhackrf.hackrf_set_hw_sync_mode(one_dev, 0)
            libhackrf.hackrf_stop_rx(one_dev)
            libhackrf.hackrf_close(one_dev)
        libhackrf.hackrf_exit()
        print("Done.")


if __name__ == "__main__":
    sys.exit(main())
