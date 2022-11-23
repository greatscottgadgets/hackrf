#!/usr/bin/python3
import sys
import subprocess

PASS, FAIL  = range(2)
EUT         = "RunningFromRAM"


def check_debug(target, register, reg_val):
    hackrf_debug = subprocess.run(["hackrf_debug",
                                   f"--{target}", "--register", register,
                                   "--read", "--device", EUT],
                                  capture_output=True, encoding="UTF-8")

    if reg_val in hackrf_debug.stdout:
        print(f"hackrf_debug --{target} passed.")
        return PASS
    else:
        print(f"hackrf_debug --{target} failed.")
        return FAIL


def main():
    results = [
        check_debug("si5351c", "2", "0x03"),
        check_debug("max2837", "3", "0x1b9"),
        check_debug("rffc5072", "2", "0x9055"),
    ]

    if FAIL not in results:
        sys.exit(PASS)
    else:
        sys.exit(FAIL)


if __name__ == "__main__":
    main()
