#!/usr/bin/python3
import subprocess
import time
import sys

EUT     = "RunningFromRAM"
TESTER  = "0000000000000000325866e629a25623"
PASS, FAIL = range(2)


def write_bytes():
    tx_bytes = b'\x7f\x00\x59\x59\x00\x7f\xa7\x59\x81\x00\xa7\xa7\x00\x81\x59\xa7'
    with open("/tmp/binary100", "wb") as bin_file:
        for i in range(62500):  # 1MB file size
            bin_file.write(tx_bytes)


def capture_signal(sweep_range, tx_gain, rx_lna_gain, rx_vga_gain, freq=None,
                   if_freq=None, lo_freq=None, image_reject=0):

    test_type = sys.argv[1]
    if test_type == "tx":
        transmitter = EUT
        receiver = TESTER
    elif test_type == "rx":
        transmitter = TESTER
        receiver = EUT
    else:
        print(f"Invalid command-line argument: {test_type}. Use tx or rx")
        sys.exit(1)

    if if_freq == None:
        transmit = subprocess.Popen(["host/build/hackrf-tools/src/hackrf_transfer",
                                     "-d", transmitter, "-R", "-t", "/tmp/binary100",
                                     "-a", "0", "-x", tx_gain, "-f", freq],
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    else:
        transmit = subprocess.Popen(["host/build/hackrf-tools/src/hackrf_transfer",
                                     "-d", transmitter, "-R", "-t", "/tmp/binary100",
                                     "-a", "0", "-x", tx_gain, "-i", if_freq,
                                     "-o", lo_freq, "-m", image_reject],
                                     stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    time.sleep(1)
    sweep = subprocess.Popen(["host/build/hackrf-tools/src/hackrf_sweep",
                              "-d", receiver, "-N", "2", "-w", "333333",
                              "-f", sweep_range, "-a", "0", "-l", rx_lna_gain,
                              "-g", rx_vga_gain],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    sweep.wait()
    transmit.terminate()
    transmit.wait()

    # parse the hackrf_sweep output
    data, stderr = sweep.communicate()
    data = data.decode("utf-8")
    data = data.split("\n")
    print(data[4])
    data = data[4]  # Note: using data from the 2nd sweep until issue #1230 is resolved.
    data = data.split(", ")
    data = data[6:21]
    bins = [float(bin) for bin in data]
    return bins


def check_signal(freq, bins):
    signal = bins.pop(1)
    signal_threshold = -25
    max_power = -10
    result = PASS

    if signal < signal_threshold:
        print(f"Signal not strong enough at {freq} MHz")
        result = FAIL
    elif signal > max_power:
        print(f"Received signal exceeded maximum power at {freq} MHz")
        result = 1

    for bin in bins:
        if bin > max_power or bin > signal:
            print(f"Non-target bin power exceeded max power threshold at {freq} MHz")
            result = FAIL
            break

    return result


def main():
    write_bytes()
    tester_hub_on = subprocess.Popen(["usbhub", "--disable-i2c", "--hub", "624C",
                                      "power", "state", "--port", "2", "--reset"])
    tester_hub_on.wait()
    time.sleep(1)
    eut_clkout_on       = subprocess.Popen(["host/build/hackrf-tools/src/hackrf_clock",
                                            "-o", "1", "-d", EUT])
    tester_clkout_off   = subprocess.Popen(["host/build/hackrf-tools/src/hackrf_clock",
                                            "-o", "0", "-d", TESTER])
    eut_clkout_on.wait()
    tester_clkout_off.wait()

    _9_5Mhz_data    = capture_signal(sweep_range="9:29", tx_gain="38", rx_lna_gain="16",
                                     rx_vga_gain="16", if_freq="2628250000",
                                     lo_freq="2620000000", image_reject="1")
    _915_5Mhz_data  = capture_signal(sweep_range="915:935", tx_gain="38", rx_lna_gain="16",
                                     rx_vga_gain="16", if_freq="2540750000",
                                     lo_freq="3455000000", image_reject="1")
    _2665_5Mhz_data = capture_signal(sweep_range="2665:2685", tx_gain="26", rx_lna_gain="16",
                                     rx_vga_gain="16", freq="2664250000")
    _5999_5Mhz_data = capture_signal(sweep_range="5999:6019", tx_gain="37", rx_lna_gain="32",
                                     rx_vga_gain="40", if_freq="2540750000",
                                     lo_freq="3460000000", image_reject="2")

    lp1_result  = check_signal(9.5, _9_5Mhz_data)
    lp2_result  = check_signal(915.5, _915_5Mhz_data)
    bp_result   = check_signal(2665.5, _2665_5Mhz_data)
    hp_result   = check_signal(5999.5, _5999_5Mhz_data)
    results     = [lp1_result, lp2_result, bp_result, hp_result]

    if FAIL in results:
        sys.exit(FAIL)
    else:
        sys.exit(PASS)


if __name__ == "__main__":
    main()

