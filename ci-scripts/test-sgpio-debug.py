#!/usr/bin/python3
import os
import sys
import subprocess
from pathlib import Path

FILENAME = f"/tmp/rx_100kB_{str(os.getpid())}"


def program_device():
    # build new firmware with SGPIO_DEBUG mode enabled
    print("Programming device...")
    fw_dir  = os.getcwd() + "/firmware/hackrf_usb/build"
    del_dir = subprocess.run(["rm", "-rf", "firmware/hackrf_usb/build"])
    mk_dir  = subprocess.run(["mkdir", "firmware/hackrf_usb/build"])
    cmake   = subprocess.run(["cmake", "-D", "SGPIO_DEBUG=1", ".."],
                             cwd=fw_dir, stdout=subprocess.DEVNULL)
    make    = subprocess.run(["make"],
                             cwd=fw_dir, stdout=subprocess.DEVNULL)
    program = subprocess.run(["./ci-scripts/test-firmware-program.sh"],
                             stdout=subprocess.DEVNULL)


def capture():
    print("Capturing data...")
    rx_100kB = subprocess.run(["host/build/hackrf-tools/src/hackrf_transfer",
                                "-r", FILENAME, "-d", "RunningFromRAM",
                                 "-n", "50000", "-s", "20000000"],
                                capture_output=True, encoding="UTF-8")
    print(rx_100kB.stdout)
    print(rx_100kB.stderr)
    print(f"Wrote capture data to file: {FILENAME}")


def check():
    print(f"Checking length of {FILENAME}")
    rx_data = Path(FILENAME).read_bytes()
    # file should be 100k bytes when using 50k samples
    if len(rx_data) != 100000:
        print(f"ERROR: Only {str(len(rx_data))} bytes found in file, expected 100k.")
        sys.exit(1)
    else:
        print("Correct file size found.")

    # check that each byte = prev_byte + 1 except at wraparound bounds
    print("Checking bytes...")
    for i in range(1, len(rx_data)):
        if rx_data[i-1] != rx_data[i] - 1:
            if not (rx_data[i] == 0 and rx_data[i-1] == 255):
                print(f"ERROR: Incorrect data value found at location {str(i)} in {FILENAME}:")
                # print up to 5 values starting from at most 1 value before error occurence
                j = -1
                while j < 4:
                    if i + j < len(rx_data) and i + j > -1:
                        print(f"{str(i+j)} : {str(rx_data[i+j])}")
                    j = j + 1
                sys.exit(1)
    print("Successfully validated all bytes in file.\nSGPIO debug test passed.")


def main():
    program_device()
    capture()
    check()


if __name__ == "__main__":
    main()

