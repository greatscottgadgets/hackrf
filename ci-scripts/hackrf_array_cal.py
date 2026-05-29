#!/usr/bin/env python3
"""
HackRF dual-device array calibration tool.

Calibrates phase offset between two HackRF receivers that share a
common reference clock (e.g. Pro P2 -> One CLKIN).

Usage:
    python3 hackrf_array_cal.py \\
        --ref-serial 645061de252d6613 \\
        --array-serial 922c63dc21748847 \\
        --freq 915000000 \\
        --sample-rate 20000000 \\
        --samples 262144
"""

import argparse
import os
import subprocess
import struct
import sys
import tempfile
import numpy as np


def run_cmd(cmd, check=True):
    """Run a shell command and return stdout."""
    print(f"$ {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if check and result.returncode != 0:
        print(f"Error: {' '.join(cmd)} failed with code {result.returncode}")
        print(result.stderr)
        sys.exit(1)
    return result.stdout + result.stderr


def configure_clock_source(pro_serial, tools_dir):
    """Configure HackRF Pro P2 to output reference clock."""
    hackrf_clock = os.path.join(tools_dir, "hackrf_clock")
    # Route P2 to CLK3 and enable the clock output
    run_cmd([hackrf_clock, "-2", "clkout", "-d", pro_serial])
    run_cmd([hackrf_clock, "-o", "1", "-d", pro_serial])
    print("Configured Pro P2 for CLKOUT (10 MHz).")


def check_clkin_status(one_serial, tools_dir):
    """Check if HackRF One is locked to external CLKIN."""
    hackrf_clock = os.path.join(tools_dir, "hackrf_clock")
    output = run_cmd([hackrf_clock, "-i", "-d", one_serial])
    print("CLKIN status:")
    print(output)
    if "1" in output or "detected" in output.lower():
        print("External clock detected on One CLKIN.")
    else:
        print("WARNING: External clock NOT detected. Check wiring.")


def capture_samples(serial, freq, sample_rate, num_samples, output_file, tools_dir):
    """Capture raw IQ samples from a HackRF using hackrf_transfer."""
    hackrf_transfer = os.path.join(tools_dir, "hackrf_transfer")
    # hackrf_transfer -r <file> -f <freq> -s <rate> -n <num_bytes> -d <serial>
    # hackrf_transfer -n specifies SAMPLES, not bytes
    cmd = [
        hackrf_transfer,
        "-r", output_file,
        "-f", str(freq),
        "-s", str(sample_rate),
        "-n", str(num_samples),
        "-d", serial,
    ]
    run_cmd(cmd)
    print(f"Captured {num_samples} samples from {serial} -> {output_file}")


def read_iq_file(path, num_samples):
    """Read raw int8 IQ file and return complex64 array."""
    expected_bytes = num_samples * 2
    with open(path, "rb") as f:
        data = f.read()
    if len(data) < expected_bytes:
        print(
            f"Warning: expected {expected_bytes} bytes, got {len(data)}"
        )
        num_samples = len(data) // 2
    elif len(data) > expected_bytes:
        # hackrf_transfer may round up to buffer boundary; truncate
        num_samples = expected_bytes // 2
    # Interpret as signed int8 interleaved I,Q
    iq = np.frombuffer(data[: num_samples * 2], dtype=np.int8).astype(np.float32)
    i_samples = iq[0::2]
    q_samples = iq[1::2]
    return i_samples + 1j * q_samples


def compute_phase_offset(ref_sig, arr_sig, sample_rate):
    """
    Compute relative phase offset between two complex signals.

    Returns the complex calibration constant that, when multiplied by
    the array signal, aligns its phase with the reference signal.
    """
    # Use cross-correlation to find best alignment (in case of sample offset)
    # For clock-locked devices, offset should be minimal, but we handle it.
    corr = np.correlate(ref_sig, arr_sig, mode="full")
    lag = np.argmax(np.abs(corr)) - (len(arr_sig) - 1)

    # Align signals
    if lag > 0:
        aligned_ref = ref_sig[lag:]
        aligned_arr = arr_sig[: len(aligned_ref)]
    elif lag < 0:
        aligned_arr = arr_sig[-lag:]
        aligned_ref = ref_sig[: len(aligned_arr)]
    else:
        aligned_ref = ref_sig
        aligned_arr = arr_sig

    # Compute complex correlation coefficient
    ref_norm = np.vdot(aligned_ref, aligned_ref)
    arr_norm = np.vdot(aligned_arr, aligned_arr)
    if ref_norm == 0 or arr_norm == 0:
        return 0.0, 0.0, 0

    # Cross-correlation at zero lag (after alignment)
    cross = np.vdot(aligned_ref, aligned_arr)
    # Phase offset = angle of cross-correlation
    phase_offset = np.angle(cross)
    # Magnitude ratio
    mag_ratio = np.abs(cross) / ref_norm

    return phase_offset, mag_ratio, lag


def compute_fft_phase(ref_sig, arr_sig, sample_rate):
    """
    Alternative: compute phase offset at dominant frequency via FFT.
    Useful when receiving a CW tone.
    """
    n = min(len(ref_sig), len(arr_sig))
    ref_sig = ref_sig[:n]
    arr_sig = arr_sig[:n]

    # Hann window to reduce spectral leakage
    window = np.hanning(n)
    ref_fft = np.fft.fft(ref_sig * window)
    arr_fft = np.fft.fft(arr_sig * window)

    # Find peak in reference spectrum
    peak_bin = np.argmax(np.abs(ref_fft[: n // 2]))

    ref_phase = np.angle(ref_fft[peak_bin])
    arr_phase = np.angle(arr_fft[peak_bin])
    phase_offset = ref_phase - arr_phase

    # Normalize to [-pi, pi]
    phase_offset = (phase_offset + np.pi) % (2 * np.pi) - np.pi

    freq_hz = peak_bin * sample_rate / n
    return freq_hz, phase_offset, np.abs(ref_fft[peak_bin]), np.abs(arr_fft[peak_bin])


def main():
    parser = argparse.ArgumentParser(
        description="Calibrate phase offset between two clock-locked HackRFs"
    )
    parser.add_argument(
        "--ref-serial", required=True, help="Serial number of reference device (clock source)"
    )
    parser.add_argument(
        "--array-serial", required=True, help="Serial number of array element device"
    )
    parser.add_argument(
        "--freq", type=int, default=915000000, help="Tuning frequency in Hz (default: 915 MHz)"
    )
    parser.add_argument(
        "--sample-rate", type=int, default=20000000, help="Sample rate in Hz (default: 20 MHz)"
    )
    parser.add_argument(
        "--samples", type=int, default=262144, help="Number of complex samples to capture"
    )
    parser.add_argument(
        "--tools-dir",
        default="../host/build/hackrf-tools/src",
        help="Path to hackrf_transfer and hackrf_debug binaries",
    )
    parser.add_argument(
        "--no-clock-config",
        action="store_true",
        help="Skip clock source configuration (assume already configured)",
    )
    parser.add_argument(
        "--method",
        choices=["corr", "fft"],
        default="fft",
        help="Phase estimation method: corr=cross-correlation, fft=FFT peak (default: fft)",
    )
    parser.add_argument(
        "--keep-iq",
        action="store_true",
        help="Keep captured IQ files after calibration",
    )
    args = parser.parse_args()

    tools_dir = os.path.abspath(args.tools_dir)
    if not os.path.isfile(os.path.join(tools_dir, "hackrf_transfer")):
        print(f"Error: hackrf_transfer not found in {tools_dir}")
        sys.exit(1)

    # 1. Configure clock source on Pro
    if not args.no_clock_config:
        configure_clock_source(args.ref_serial, tools_dir)

    check_clkin_status(args.array_serial, tools_dir)

    # 2. Capture from both devices
    tmpdir = tempfile.mkdtemp(prefix="hackrf_cal_")
    ref_file = os.path.join(tmpdir, "ref.iq")
    arr_file = os.path.join(tmpdir, "array.iq")

    print("\n--- Starting capture from both devices ---")
    # We capture sequentially since hackrf_transfer uses stdout/stderr
    # and two concurrent instances would interleave output messily.
    # For phase calibration, a few seconds of sequential capture is fine
    # as long as the signal source is stable.
    capture_samples(
        args.ref_serial,
        args.freq,
        args.sample_rate,
        args.samples,
        ref_file,
        tools_dir,
    )
    capture_samples(
        args.array_serial,
        args.freq,
        args.sample_rate,
        args.samples,
        arr_file,
        tools_dir,
    )

    # 3. Read IQ data
    print("\n--- Reading captured samples ---")
    ref_sig = read_iq_file(ref_file, args.samples)
    arr_sig = read_iq_file(arr_file, args.samples)
    print(f"Reference signal: {len(ref_sig)} samples")
    print(f"Array signal:     {len(arr_sig)} samples")

    # 4. Compute phase offset
    print("\n--- Calibration results ---")
    if args.method == "corr":
        phase_offset, mag_ratio, lag = compute_phase_offset(
            ref_sig, arr_sig, args.sample_rate
        )
        print(f"Method:           Cross-correlation")
        print(f"Sample lag:       {lag}")
        print(f"Magnitude ratio:  {mag_ratio:.6f}")
        print(f"Phase offset:     {np.degrees(phase_offset):.4f} deg ({phase_offset:.6f} rad)")
    else:
        freq_hz, phase_offset, ref_peak, arr_peak = compute_fft_phase(
            ref_sig, arr_sig, args.sample_rate
        )
        print(f"Method:           FFT peak")
        print(f"Peak frequency:   {freq_hz / 1e6:.6f} MHz")
        print(f"Ref peak mag:     {ref_peak:.2f}")
        print(f"Array peak mag:   {arr_peak:.2f}")
        print(f"Phase offset:     {np.degrees(phase_offset):.4f} deg ({phase_offset:.6f} rad)")

    # Calibration constant: multiply array signal by exp(-j*phase_offset) to align with ref
    cal_const = np.exp(-1j * phase_offset)
    print(f"\nCalibration constant (array * const -> aligned with ref):")
    print(f"  Real: {cal_const.real:.6f}")
    print(f"  Imag: {cal_const.imag:.6f}")
    print(f"  Mag:  {np.abs(cal_const):.6f}")
    print(f"  Phase: {np.angle(cal_const):.6f} rad")

    # 5. Cleanup
    if not args.keep_iq:
        os.remove(ref_file)
        os.remove(arr_file)
        os.rmdir(tmpdir)
    else:
        print(f"\nIQ files kept in: {tmpdir}")


if __name__ == "__main__":
    main()
