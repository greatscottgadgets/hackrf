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
    parser.add_argument(
        "--runs",
        type=int,
        default=5,
        help="Number of capture runs for stability statistics (default: 5)",
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

    # 2. Run multiple captures for stability statistics
    tmpdir = tempfile.mkdtemp(prefix="hackrf_cal_")
    phase_offsets = []
    mag_ratios = []
    peak_freqs = []

    print(f"\n--- Running {args.runs} capture sets for stability ---")
    for run in range(args.runs):
        ref_file = os.path.join(tmpdir, f"ref_{run}.iq")
        arr_file = os.path.join(tmpdir, f"array_{run}.iq")

        print(f"\n[Run {run + 1}/{args.runs}]")
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

        ref_sig = read_iq_file(ref_file, args.samples)
        arr_sig = read_iq_file(arr_file, args.samples)

        if args.method == "corr":
            phase_offset, mag_ratio, _ = compute_phase_offset(
                ref_sig, arr_sig, args.sample_rate
            )
            phase_offsets.append(phase_offset)
            mag_ratios.append(mag_ratio)
        else:
            freq_hz, phase_offset, ref_peak, arr_peak = compute_fft_phase(
                ref_sig, arr_sig, args.sample_rate
            )
            phase_offsets.append(phase_offset)
            mag_ratios.append(arr_peak / ref_peak if ref_peak > 0 else 0)
            peak_freqs.append(freq_hz)

        print(f"  Phase offset: {np.degrees(phase_offset):.2f} deg")

    # 3. Compute statistics
    phase_offsets_deg = np.degrees(np.array(phase_offsets))
    phase_mean = np.mean(phase_offsets_deg)
    phase_std = np.std(phase_offsets_deg)
    phase_min = np.min(phase_offsets_deg)
    phase_max = np.max(phase_offsets_deg)
    phase_range = phase_max - phase_min

    mag_mean = np.mean(mag_ratios)
    mag_std = np.std(mag_ratios)

    print("\n" + "=" * 50)
    print("--- STABILITY STATISTICS ---")
    print("=" * 50)
    print(f"Runs:             {args.runs}")
    if args.method == "fft" and peak_freqs:
        freq_mean = np.mean(peak_freqs) / 1e6
        freq_std = np.std(peak_freqs) / 1e6
        print(f"Peak frequency:   {freq_mean:.6f} ± {freq_std:.6f} MHz")
    print(f"\nPhase offset:")
    print(f"  Mean:           {phase_mean:.4f} deg")
    print(f"  Std dev:        {phase_std:.4f} deg")
    print(f"  Min:            {phase_min:.4f} deg")
    print(f"  Max:            {phase_max:.4f} deg")
    print(f"  Range:          {phase_range:.4f} deg")
    print(f"\nMagnitude ratio:")
    print(f"  Mean:           {mag_mean:.4f}")
    print(f"  Std dev:        {mag_std:.4f}")

    # Stability assessment
    if phase_std < 5.0:
        stability = "EXCELLENT"
        advice = "Calibration is rock-solid."
    elif phase_std < 15.0:
        stability = "GOOD"
        advice = "Usable for beamforming/direction finding."
    elif phase_std < 30.0:
        stability = "FAIR"
        advice = "OK for coarse direction finding. Consider a stronger signal."
    else:
        stability = "POOR"
        advice = "Too noisy for array processing. Need stronger/common signal or same antenna."

    print(f"\nStability:        {stability}")
    print(f"Advice:           {advice}")

    # Final calibration constant using mean phase
    mean_phase_rad = np.radians(phase_mean)
    cal_const = np.exp(-1j * mean_phase_rad)
    print(f"\n--- Final calibration constant (mean of {args.runs} runs) ---")
    print(f"  Real:  {cal_const.real:.6f}")
    print(f"  Imag:  {cal_const.imag:.6f}")
    print(f"  Mag:   {np.abs(cal_const):.6f}")
    print(f"  Phase: {np.angle(cal_const):.6f} rad ({phase_mean:.4f} deg)")

    # 4. Cleanup
    if not args.keep_iq:
        for f in os.listdir(tmpdir):
            os.remove(os.path.join(tmpdir, f))
        os.rmdir(tmpdir)
    else:
        print(f"\nIQ files kept in: {tmpdir}")


if __name__ == "__main__":
    main()
