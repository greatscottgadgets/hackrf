#!/usr/bin/env python3
"""
ADS-B decoder pipeline for HackRF using dump1090.

Captures raw IQ from a HackRF device, converts SC8 -> UC8 on the fly,
and pipes to dump1090 for real-time decoding.

Usage:
    python3 adsb_hackrf.py --serial <serial> --duration 60

Requirements:
    - hackrf_transfer (from hackrf-tools)
    - dump1090 (e.g. dump1090-fa from Homebrew)
"""

import argparse
import subprocess
import sys
import time
import threading


def parse_args():
    parser = argparse.ArgumentParser(description="ADS-B decoder for HackRF")
    parser.add_argument(
        "--serial",
        default="645061de252d6613",
        help="HackRF serial number (default: Pro r1.2)",
    )
    parser.add_argument(
        "--freq",
        type=int,
        default=1_090_000_000,
        help="Tuner frequency in Hz (default: 1090000000)",
    )
    parser.add_argument(
        "--rate",
        type=int,
        default=2_000_000,
        help="Sample rate in Hz (default: 2000000)",
    )
    parser.add_argument(
        "--lna-gain",
        type=int,
        default=32,
        help="RX LNA gain 0-40 dB, 8 dB steps (default: 32)",
    )
    parser.add_argument(
        "--vga-gain",
        type=int,
        default=40,
        help="RX VGA gain 0-62 dB, 2 dB steps (default: 40)",
    )
    parser.add_argument(
        "--duration",
        type=int,
        default=60,
        help="Capture duration in seconds (default: 60)",
    )
    parser.add_argument(
        "--fix",
        action="store_true",
        default=True,
        help="Enable CRC error correction (default: on)",
    )
    parser.add_argument(
        "--no-fix",
        action="store_true",
        help="Disable CRC error correction",
    )
    return parser.parse_args()


def build_hackrf_cmd(args: argparse.Namespace) -> list[str]:
    num_samples = args.rate * args.duration
    return [
        "hackrf_transfer",
        "-d", args.serial,
        "-r", "-",
        "-f", str(args.freq),
        "-s", str(args.rate),
        "-n", str(num_samples),
        "-l", str(args.lna_gain),
        "-g", str(args.vga_gain),
    ]


def build_dump1090_cmd(args: argparse.Namespace) -> list[str]:
    cmd = [
        "dump1090",
        "--ifile", "-",
        "--iformat", "UC8",
    ]
    if not args.no_fix:
        cmd.append("--fix")
    return cmd


def run_pipeline(args: argparse.Namespace) -> list[str]:
    hackrf_cmd = build_hackrf_cmd(args)
    dump1090_cmd = build_dump1090_cmd(args)

    print("=" * 70)
    print("ADS-B Live Capture Pipeline")
    print(f"Device serial: {args.serial}")
    print(f"Frequency:     {args.freq} Hz ({args.freq / 1e6:.3f} MHz)")
    print(f"Sample rate:   {args.rate / 1e6:.3f} MS/s")
    print(
        f"Gain:          LNA={args.lna_gain} dB, VGA={args.vga_gain} dB "
        f"(total={args.lna_gain + args.vga_gain} dB)"
    )
    print(f"Duration:      {args.duration} s")
    print("=" * 70)
    sys.stdout.flush()

    hackrf = subprocess.Popen(
        hackrf_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    dump1090 = subprocess.Popen(
        dump1090_cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )

    start = time.time()
    bytes_converted = 0
    dump1090_lines: list[str] = []

    def read_dump1090() -> None:
        while True:
            line = dump1090.stdout.readline()
            if not line:
                break
            line_str = line.decode("utf-8", errors="replace")
            dump1090_lines.append(line_str)
            sys.stdout.write(line_str)
            sys.stdout.flush()

    reader = threading.Thread(target=read_dump1090, daemon=True)
    reader.start()

    try:
        while hackrf.poll() is None and time.time() - start < args.duration + 10:
            chunk = hackrf.stdout.read(131072)
            if not chunk:
                break
            bytes_converted += len(chunk)
            uc8 = bytes((b + 128) & 0xff for b in chunk)
            try:
                dump1090.stdin.write(uc8)
                dump1090.stdin.flush()
            except BrokenPipeError:
                break
    except KeyboardInterrupt:
        pass
    finally:
        elapsed = time.time() - start

        hackrf.terminate()
        try:
            hackrf.wait(timeout=3)
        except subprocess.TimeoutExpired:
            hackrf.kill()

        dump1090.stdin.close()
        dump1090.terminate()
        try:
            dump1090.wait(timeout=3)
        except subprocess.TimeoutExpired:
            dump1090.kill()

        reader.join(timeout=2)

        messages = [l.strip() for l in dump1090_lines if l.startswith("*")]
        icaos = set()
        for m in messages:
            hex_data = m.lstrip("*").split(";")[0]
            if len(hex_data) >= 6:
                icaos.add(hex_data[2:8])

        print("\n" + "=" * 70)
        print("SUMMARY")
        print("=" * 70)
        print(f"Duration:              {elapsed:.1f} s")
        print(f"IQ samples:            {bytes_converted // 2:,}")
        print(f"Raw messages decoded:  {len(messages)}")
        print(f"Unique ICAO addresses: {len(icaos)}")
        if icaos:
            print(f"ICAO addresses:        {', '.join(sorted(icaos))}")
        print("=" * 70)

    return messages


def main() -> int:
    args = parse_args()
    run_pipeline(args)
    return 0


if __name__ == "__main__":
    sys.exit(main())
