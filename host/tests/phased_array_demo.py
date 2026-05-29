#!/usr/bin/env python3
"""
2-Element Phased Array Direction-Finding Demo

Uses two HackRF devices with shared clock to estimate the bearing
to a strong FM broadcast station via phase interferometry.

Hardware Setup:
- HackRF Pro r1.2 (serial: 645061de252d6613) — primary, provides CLKOUT
- HackRF One r9 (serial: 922c63dc21748847) — secondary, receives CLKIN
- Antennas separated by distance d (default: 0.5 m) along a known baseline
- Baseline should be oriented perpendicular to the expected signal direction

Physics:
    Δφ = (2πd/λ) × sin(θ)
    θ  = arcsin(Δφ × λ / (2πd))

Where:
    Δφ = phase difference between the two channels (radians)
    d  = antenna separation (meters)
    λ  = wavelength = c/f (meters)
    θ  = bearing angle relative to array normal (degrees)

Front-back ambiguity: A 2-element array cannot distinguish between
θ and 180°-θ. A 3rd element or directional antenna resolves this.

Usage:
    python3 phased_array_demo.py --freq 94000000 --baseline 0.5
"""

import argparse
import numpy as np
import subprocess
import sys
import tempfile

HACKRF_TRANSFER = "/Users/ivo/hackrf/host/build/hackrf-tools/src/hackrf_transfer"
HACKRF_CLOCK = "/Users/ivo/hackrf/host/build/hackrf-tools/src/hackrf_clock"

PRO_SERIAL = "645061de252d6613"
ONE_SERIAL = "922c63dc21748847"

C = 299_792_458  # speed of light, m/s


def parse_args():
    parser = argparse.ArgumentParser(description="2-element phased array DF demo")
    parser.add_argument(
        "--freq",
        type=int,
        default=94_000_000,
        help="Target frequency in Hz (default: 94 MHz)",
    )
    parser.add_argument(
        "--baseline",
        type=float,
        default=0.5,
        help="Antenna separation in meters (default: 0.5)",
    )
    parser.add_argument(
        "--duration",
        type=float,
        default=0.5,
        help="Capture duration in seconds (default: 0.5)",
    )
    parser.add_argument(
        "--sample-rate",
        type=int,
        default=2_000_000,
        help="Sample rate in Hz (default: 2 MHz)",
    )
    parser.add_argument(
        "--calibrate",
        action="store_true",
        help="Run calibration: both antennas at same position",
    )
    return parser.parse_args()


def run_cmd(cmd: list) -> tuple:
    p = subprocess.run(cmd, capture_output=True, text=True)
    return p.returncode, p.stdout, p.stderr


def enable_shared_clock() -> bool:
    """Enable Pro CLKOUT and verify One CLKIN detects it."""
    print("Setting up shared clock...")
    rc, _, _ = run_cmd([HACKRF_CLOCK, "-o", "1", "-d", PRO_SERIAL])
    if rc != 0:
        print("  FAIL: Could not enable Pro CLKOUT")
        return False

    rc, out, _ = run_cmd([HACKRF_CLOCK, "-i", "-d", ONE_SERIAL])
    if "clock signal detected" not in out:
        print(f"  FAIL: One does not detect clock ({out.strip()})")
        return False

    print("  Shared clock active (Pro CLKOUT → One CLKIN)")
    return True


def capture_both(freq: int, sample_rate: int, duration: float, path1: str, path2: str) -> bool:
    """Simultaneously capture from both devices."""
    n_samples = int(sample_rate * duration)
    base = [
        "-f", str(freq), "-s", str(sample_rate),
        "-n", str(n_samples), "-a", "0", "-l", "16", "-g", "20",
    ]

    p1 = subprocess.Popen(
        [HACKRF_TRANSFER, "-d", PRO_SERIAL, "-r", path1] + base,
        stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
    )
    p2 = subprocess.Popen(
        [HACKRF_TRANSFER, "-d", ONE_SERIAL, "-r", path2] + base,
        stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
    )

    try:
        _, e1 = p1.communicate(timeout=30)
        _, e2 = p2.communicate(timeout=30)
    except subprocess.TimeoutExpired:
        p1.kill()
        p2.kill()
        print("  CAPTURE TIMEOUT")
        return False

    if p1.returncode != 0:
        print(f"  Pro capture failed: {e1.decode().strip()[-80:]}")
        return False
    if p2.returncode != 0:
        print(f"  One capture failed: {e2.decode().strip()[-80:]}")
        return False

    return True


def read_iq(path: str) -> np.ndarray:
    raw = np.fromfile(path, dtype=np.int8)
    if len(raw) % 2:
        raw = raw[:-1]
    n = len(raw) // 2
    iq = np.zeros(n, dtype=np.complex64)
    iq.real = raw[0::2].astype(np.float32)
    iq.imag = raw[1::2].astype(np.float32)
    return iq


def compute_phase_diff(ref: np.ndarray, sig: np.ndarray, freq: int, sr: int) -> float:
    """
    Compute phase difference by cross-correlating the strongest narrowband
    component. This is more robust than full-band correlation for FM signals.
    """
    n = min(len(ref), len(sig))
    ref = ref[:n]
    sig = sig[:n]

    # Find strongest narrowband component via FFT
    spectrum = np.fft.fft(ref)
    # Look in the middle 80% of spectrum to avoid DC and edges
    margin = n // 10
    peak_bin = margin + np.argmax(np.abs(spectrum[margin : n - margin]))

    # Extract phase of that tone from both signals
    tone_ref = spectrum[peak_bin]
    tone_sig = np.fft.fft(sig)[peak_bin]

    phase = np.angle(tone_sig * np.conj(tone_ref))
    return phase


def bearing_from_phase(phase_rad: float, freq: int, baseline: float) -> tuple:
    """
    Convert phase difference to bearing angle.
    Returns (bearing_deg, ambiguity_note).
    """
    wavelength = C / freq
    k = 2 * np.pi / wavelength

    # Δφ = k * d * sin(θ)  →  sin(θ) = Δφ / (k * d)
    sin_theta = phase_rad / (k * baseline)

    # Clamp to valid range [-1, 1]
    if sin_theta > 1.0:
        sin_theta = 1.0
    if sin_theta < -1.0:
        sin_theta = -1.0

    theta = np.degrees(np.arcsin(sin_theta))

    # Front-back ambiguity
    if theta >= 0:
        ambiguity = f"{theta:.1f}° or {180 - theta:.1f}°"
    else:
        ambiguity = f"{theta:.1f}° or {-180 - theta:.1f}°"

    return theta, ambiguity


def run_calibration(freq: int, sample_rate: int, duration: float) -> float:
    """
    Run calibration: both antennas at same position.
    Measures the inherent phase offset between the two receiver chains.
    Returns the calibration offset in radians.
    """
    print("\n" + "=" * 70)
    print("CALIBRATION: Place both antennas at SAME position")
    print("Press Enter when ready...")
    input()

    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
        path1 = f.name
    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
        path2 = f.name

    print("  Capturing calibration data...")
    if not capture_both(freq, sample_rate, duration, path1, path2):
        return 0.0

    iq1 = read_iq(path1)
    iq2 = read_iq(path2)

    phase = compute_phase_diff(iq1, iq2, freq, sample_rate)
    print(f"  Calibration phase offset: {np.degrees(phase):+.2f}°")
    print("  This offset will be subtracted from all future measurements.")
    return phase


def main() -> int:
    args = parse_args()

    print("=" * 70)
    print("2-Element Phased Array Direction-Finding Demo")
    print("=" * 70)
    print(f"Frequency:     {args.freq / 1e6:.1f} MHz")
    print(f"Wavelength:    {C / args.freq * 100:.1f} cm")
    print(f"Baseline:      {args.baseline:.2f} m")
    print(f"Sample rate:   {args.sample_rate / 1e6:.1f} MHz")
    print(f"Capture:       {args.duration:.1f} s")
    print()

    # Validate baseline
    wavelength = C / args.freq
    if args.baseline > wavelength / 2:
        print(f"WARNING: Baseline ({args.baseline} m) > λ/2 ({wavelength / 2:.2f} m)")
        print("This causes grating lobes (ambiguity). Consider shorter spacing.")
        print()

    # Setup shared clock
    if not enable_shared_clock():
        return 1

    # Calibration
    cal_offset = 0.0
    if args.calibrate:
        cal_offset = run_calibration(args.freq, args.sample_rate, args.duration)

    # Main measurement loop
    print("\n" + "=" * 70)
    print("MEASUREMENT: Place antennas at baseline separation")
    print("Point array broadside toward expected signal direction")
    print("=" * 70)
    print("Press Enter to start capture (Ctrl-C to stop)...")
    input()

    print("\nCapturing...")
    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
        path1 = f.name
    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
        path2 = f.name

    if not capture_both(args.freq, args.sample_rate, args.duration, path1, path2):
        return 1

    iq1 = read_iq(path1)
    iq2 = read_iq(path2)

    phase = compute_phase_diff(iq1, iq2, args.freq, args.sample_rate)
    phase_calibrated = phase - cal_offset

    print(f"\nRaw phase difference:     {np.degrees(phase):+.2f}°")
    if args.calibrate:
        print(f"Calibrated phase diff:    {np.degrees(phase_calibrated):+.2f}°")

    bearing, ambiguity = bearing_from_phase(phase_calibrated, args.freq, args.baseline)

    print(f"\n{'=' * 70}")
    print(f"BEARING ESTIMATE: {bearing:.1f}° relative to array normal")
    print(f"Front/back ambiguity: {ambiguity}")
    print(f"{'=' * 70}")

    print("\nNote: A 2-element array has ±90° ambiguity.")
    print("      Add a 3rd element or directional antenna to resolve.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
