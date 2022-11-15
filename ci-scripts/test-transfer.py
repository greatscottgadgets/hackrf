#!/usr/bin/python3
import subprocess
import time
import sys


def write_bytes():
    tx_bytes = b'\x7f\x00\x59\x59\x00\x7f\xa7\x59\x81\x00\xa7\xa7\x00\x81\x59\xa7'

    with open("/tmp/binary100", "wb") as bin_file:
        # 1MB file size
        for i in range(62500):
            bin_file.write(tx_bytes)


def capture_signal(sweep_range, tx_gain, rx_lna_gain, rx_vga_gain, freq=None,
                   if_freq=None, lo_freq=None, image_reject=0):
    EUT     = "RunningFromRAM"
    TESTER  = "0000000000000000325866e629a25623"
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
        transmit = subprocess.Popen(["hackrf_transfer", "-d", transmitter, "-R", "-t", "/tmp/binary100",
                                     "-a", "0", "-x", tx_gain, "-f", freq],
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    else:
        transmit = subprocess.Popen(["hackrf_transfer", "-d", transmitter, "-R", "-t", "/tmp/binary100",
                                     "-a", "0", "-x", tx_gain, "-i", if_freq,
                                     "-o", lo_freq, "-m", image_reject],
                                     stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    time.sleep(0.5)
    sweep = subprocess.Popen(["hackrf_sweep", "-d", receiver, "-1", "-w", "333333",
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
    data = data[0]
    print(data)
    data = data.split(", ")
    data = data[6:21]
    bins = [float(bin) for bin in data]
    return bins


def check_signal(freq, bins):
    signal = bins.pop(1)
    signal_threshold = -25
    noise_threshold = signal - 3
    max_power = -10
    result = 0

    for bin in bins:
        if bin > max_power:
            print(f"Exceeded maximum power at {freq} MHz")
            result = 1
        elif bin > noise_threshold:
            print(f"Non-target bin power too high at {freq} MHz")
            result = 1

    if signal < signal_threshold:
        print(f"Signal not strong enough at {freq} MHz")
        result = 1
    elif signal > max_power:
        print(f"Received signal exceeded maximum power at {freq} MHz")
        result = 1

    return result


def main():
    write_bytes()
    _2665_5Mhz_data = capture_signal(sweep_range="2665:2685", tx_gain="26", rx_lna_gain="16",
                                     rx_vga_gain="16", freq="2664250000")
    _915_5Mhz_data  = capture_signal(sweep_range="915:935", tx_gain="38", rx_lna_gain="16",
                                     rx_vga_gain="16", if_freq="2540750000",
                                     lo_freq="3455000000", image_reject="1")
    _9_5Mhz_data    = capture_signal(sweep_range="9:29", tx_gain="38", rx_lna_gain="16",
                                     rx_vga_gain="16", if_freq="2628250000",
                                     lo_freq="2620000000", image_reject="1")
    _5999_5Mhz_data = capture_signal(sweep_range="5999:6019", tx_gain="37", rx_lna_gain="32",
                                     rx_vga_gain="40", if_freq="2540750000",
                                     lo_freq="3460000000", image_reject="2")

    lp1_result  = check_signal(915.5, _915_5Mhz_data)
    lp2_result  = check_signal(9.5, _9_5Mhz_data)
    bp_result   = check_signal(2665.5, _2665_5Mhz_data)
    hp_result   = check_signal(5999.5, _5999_5Mhz_data)
    results     = [lp1_result, lp2_result, bp_result, hp_result]

    if 1 in results:
        sys.exit(1)
    else:
        sys.exit(0)


if __name__ == "__main__":
    main()

