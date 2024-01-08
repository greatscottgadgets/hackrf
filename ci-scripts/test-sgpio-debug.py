#!/usr/bin/python3
import os
import sys
import subprocess
from pathlib import Path

FILENAME = "/tmp/rx_100kB_" + str(os.getpid())


def program_device():
    # build new firmware with SGPIO_DEBUG mode enabled
    print("Programming device..")
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
    print("Capturing data..")
    rx_100kB = subprocess.run(["host/build/hackrf-tools/src/hackrf_transfer",
                                "-r", FILENAME, "-d", "RunningFromRAM",
                                 "-n", "50000", "-s", "20000000"],
                                capture_output=True, encoding="UTF-8")
    print(rx_100kB.stdout)
    print(rx_100kB.stderr)
    print("Wrote data to file: " + FILENAME)


def check():
    rx_data = Path(FILENAME).read_bytes()
    # file should be 100k bytes when using 50k samples
    if len(rx_data) != 100000:
        print("SGPIO debug test failed.")
        sys.exit(1)

    # check that each byte = prev_byte + 1 except at wraparound bounds
    for i in range(1, len(rx_data)):
        if rx_data[i-1] != rx_data[i] - 1:
            if not (rx_data[i] == 0 and rx_data[i-1] == 255):
                print("SGPIO debug test failed.")
                sys.exit(1)
    print("SGPIO debug test passed.")


def main():
    program_device()
    capture()
    check()


if __name__ == "__main__":
    main()

