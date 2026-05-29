#!/usr/bin/env python3
"""
Live beamforming with two clock-locked HackRFs.

Captures repeated snapshots from a reference and array device,
applies calibration, and scans beam steering angles to find
the dominant arrival direction.

Usage:
    python3 hackrf_beamform.py \\
        --ref-serial 645061de252d6613 \\
        --array-serial 922c63dc21748847 \\
        --freq 2437000000 \\
        --cal-phase -113.53 \\
        --interval 2 \\
        --snaps 50
"""

import argparse
import os
import subprocess
import sys
import tempfile
import time
import numpy as np


def run_cmd(cmd, check=True):
    """Run a shell command and return stdout."""
    result = subprocess.run(cmd, capture_output=True, text=True)
    if check and result.returncode != 0:
        print(f"Error: {' '.join(cmd)} failed with code {result.returncode}")
        print(result.stderr)
        return None
    return result.stdout + result.stderr


def configure_clock_source(pro_serial, tools_dir):
    """Configure HackRF Pro P2 to output reference clock."""
    hackrf_clock = os.path.join(tools_dir, "hackrf_clock")
    run_cmd([hackrf_clock, "-2", "clkout", "-d", pro_serial], check=False)
    run_cmd([hackrf_clock, "-o", "1", "-d", pro_serial], check=False)
    print("Clock source configured.")


def capture_snapshot(serial, freq, sample_rate, num_samples, tmpdir, prefix, tools_dir):
    """Capture one buffer from a device, return complex array or None."""
    path = os.path.join(tmpdir, f"{prefix}.iq")
    hackrf_transfer = os.path.join(tools_dir, "hackrf_transfer")
    cmd = [
        hackrf_transfer,
        "-r", path,
        "-f", str(freq),
        "-s", str(sample_rate),
        "-n", str(num_samples),
        "-d", serial,
    ]
    out = run_cmd(cmd, check=False)
    if out is None:
        return None

    expected = num_samples * 2
    with open(path, "rb") as f:
        data = f.read()
    if len(data) < expected:
        return None

    iq = np.frombuffer(data[:expected], dtype=np.int8).astype(np.float32)
    i_samp = iq[0::2]
    q_samp = iq[1::2]
    return i_samp + 1j * q_samp


def ascii_bar(value, width=40, vmax=1.0):
    """Return an ASCII bar string."""
    n = int(round((value / vmax) * width))
    n = max(0, min(width, n))
    return "█" * n + "░" * (width - n)


def compute_beam_power(ref_sig, arr_sig, cal_phase_deg, scan_resolution=1.0):
    """
    Scan steering phases and compute beam power.

    Returns:
        steering_angles: array of steering angles in degrees
        powers: normalized power at each angle
        peak_angle: angle with maximum power
        peak_power: power at peak
    """
    n = min(len(ref_sig), len(arr_sig))
    ref = ref_sig[:n]
    arr = arr_sig[:n]

    # Apply calibration: rotate array to align with reference
    cal = np.exp(-1j * np.radians(cal_phase_deg))
    arr_cal = arr * cal

    # Scan steering angles (phase shifts from -180 to +180)
    angles = np.arange(-180, 180, scan_resolution)
    powers = np.zeros_like(angles, dtype=np.float64)

    for i, angle in enumerate(angles):
        steer = np.exp(-1j * np.radians(angle))
        beam = ref + arr_cal * steer
        # Power = average squared magnitude
        powers[i] = np.mean(np.abs(beam) ** 2)

    # Normalize
    powers = powers / np.max(powers)
    peak_idx = np.argmax(powers)
    peak_angle = angles[peak_idx]
    peak_power = powers[peak_idx]

    return angles, powers, peak_angle, peak_power


def print_beam_pattern(angles, powers, peak_angle, peak_power, width=50):
    """Print a compact ASCII beam pattern."""
    # Show every Nth point to fit in terminal
    step = max(1, len(angles) // width)
    print(f"\nBeam pattern (peak @ {peak_angle:+.1f}°):")
    for i in range(0, len(angles), step):
        a = angles[i]
        p = powers[i]
        marker = "*" if abs(a - peak_angle) < step else " "
        print(f"{a:+.0f}° {marker}|{ascii_bar(p, width=30)}")


def main():
    parser = argparse.ArgumentParser(description="Live beamforming with two HackRFs")
    parser.add_argument("--ref-serial", required=True, help="Reference device serial")
    parser.add_argument("--array-serial", required=True, help="Array device serial")
    parser.add_argument("--freq", type=int, default=2437000000, help="Tuning freq Hz")
    parser.add_argument("--sample-rate", type=int, default=20000000, help="Sample rate Hz")
    parser.add_argument("--samples", type=int, default=262144, help="Samples per snapshot")
    parser.add_argument("--cal-phase", type=float, default=0.0, help="Calibration phase offset in deg (array aligned to ref)")
    parser.add_argument("--interval", type=float, default=2.0, help="Seconds between snapshots")
    parser.add_argument("--snaps", type=int, default=20, help="Total number of snapshots")
    parser.add_argument("--tools-dir", default="../host/build/hackrf-tools/src")
    parser.add_argument("--no-clock-config", action="store_true")
    parser.add_argument("--resolution", type=float, default=5.0, help="Beam scan resolution in degrees")
    args = parser.parse_args()

    tools_dir = os.path.abspath(args.tools_dir)
    if not os.path.isfile(os.path.join(tools_dir, "hackrf_transfer")):
        print(f"Error: hackrf_transfer not found in {tools_dir}")
        sys.exit(1)

    if not args.no_clock_config:
        configure_clock_source(args.ref_serial, tools_dir)

    tmpdir = tempfile.mkdtemp(prefix="hackrf_beam_")

    print(f"\n{'='*60}")
    print(f"Live beamforming: {args.snaps} snapshots @ {args.freq/1e6:.0f} MHz")
    print(f"Calibration phase: {args.cal_phase:.2f}°")
    print(f"Interval: {args.interval:.1f}s | Resolution: {args.resolution:.1f}°")
    print(f"{'='*60}\n")

    peak_angles = []
    peak_powers = []

    try:
        for snap in range(args.snaps):
            t0 = time.time()

            ref_sig = capture_snapshot(
                args.ref_serial, args.freq, args.sample_rate,
                args.samples, tmpdir, "ref", tools_dir
            )
            arr_sig = capture_snapshot(
                args.array_serial, args.freq, args.sample_rate,
                args.samples, tmpdir, "array", tools_dir
            )

            if ref_sig is None or arr_sig is None:
                print(f"[Snap {snap+1}/{args.snaps}] Capture failed, skipping...")
                time.sleep(args.interval)
                continue

            angles, powers, peak_angle, peak_power = compute_beam_power(
                ref_sig, arr_sig, args.cal_phase, args.resolution
            )
            peak_angles.append(peak_angle)
            peak_powers.append(peak_power)

            # Running statistics
            running_mean = np.mean(peak_angles)
            running_std = np.std(peak_angles)

            print(f"\n[Snap {snap+1}/{args.snaps}] "
                  f"Peak: {peak_angle:+.1f}° | "
                  f"Running: {running_mean:+.1f}° ± {running_std:.1f}° | "
                  f"Power: {peak_power:.3f}")

            # Show mini ASCII pattern
            if snap % 5 == 0 or snap == args.snaps - 1:
                print_beam_pattern(angles, powers, peak_angle, peak_power)

            # Throttle to interval
            elapsed = time.time() - t0
            sleep_time = max(0, args.interval - elapsed)
            if sleep_time > 0:
                time.sleep(sleep_time)

    except KeyboardInterrupt:
        print("\n\nInterrupted by user.")

    # Final summary
    print(f"\n{'='*60}")
    print("FINAL SUMMARY")
    print(f"{'='*60}")
    if peak_angles:
        print(f"Estimated arrival angle: {np.mean(peak_angles):+.2f}°")
        print(f"Std dev:                 {np.std(peak_angles):.2f}°")
        print(f"Min:                     {np.min(peak_angles):+.2f}°")
        print(f"Max:                     {np.max(peak_angles):+.2f}°")
        print(f"Confidence:              {'HIGH' if np.std(peak_angles) < 15 else 'MEDIUM' if np.std(peak_angles) < 30 else 'LOW'}")
    else:
        print("No valid snapshots captured.")

    # Cleanup
    for f in os.listdir(tmpdir):
        os.remove(os.path.join(tmpdir, f))
    os.rmdir(tmpdir)


if __name__ == "__main__":
    main()
