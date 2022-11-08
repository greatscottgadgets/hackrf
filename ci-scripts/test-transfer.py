#!/usr/bin/python3
import subprocess
import time


def write_bytes():
    tx_bytes = b'\x7f\x00\x59\x59\x00\x7f\xa7\x59\x81\x00\xa7\xa7\x00\x81\x59\xa7'

    with open("/tmp/binary100", "wb") as bin_file:
        # 1MB file size
        for i in range(62500):
            bin_file.write(tx_bytes)


def capture_signal(sweep_range, freq, tx_gain, rx_lna_gain, rx_vga_gain,
                   if_freq=None, lo_freq=None, image_reject=0):
    EUT     = "0000000000000000325866e629a25623"
    TESTER  = "0000000000000000325866e629822923"

    if if_freq == None:
        print("calling transmit without if_freq")
        transmit = subprocess.Popen(["hackrf_transfer", "-d", EUT, "-R", "-t", "/tmp/binary100",
                                     "-a", "0", "-x", tx_gain, "-f", freq],
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    else:
        print("calling transmit with if_freq")
        transmit = subprocess.Popen(["hackrf_transfer", "-d", EUT, "-R", "-t", "/tmp/binary100",
                                     "-a", "0", "-x", tx_gain, "-i", if_freq,
                                     "-o", lo_freq, "-m", image_reject],
                                     stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    time.sleep(1)
    print("sweeping")
    sweep = subprocess.Popen(["hackrf_sweep", "-d", TESTER, "-1", "-w", "333333",
                              "-f", sweep_range, "-a", "0", "-l", rx_lna_gain,
                              "-g", rx_vga_gain],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    data, stderr = sweep.communicate()
    data = data.decode("utf-8")
    print("data:\n", data)

    transmit.terminate()
    transmit.wait()


# for convenience
# hackrf_transfer -d EUT -R -t /tmp/binary100 -f 2664250000 -a 0 -x 26
# hackrf_sweep -d TESTER -1 -w 333333 -f 2665:2685 -a 0 -l 16 -g 16

# hackrf_transfer -d EUT -R -t /tmp/binary100 -i 2540750000 -o 3455000000 -m 1 -a 0 -x 38
# hackrf_sweep -d TESTER -1 -w 333333 -f 915:935 -a 0 -l 16 -g 16

# hackrf_transfer -d EUT -R -t /tmp/binary100 -i 2628250000 -o 2620000000 -m 1 -a 0 -x 38
# hackrf_sweep -d TESTER -1 -w 333333 -f 9:29 -a 0 -l 16 -g 16

# hackrf_transfer -d EUT -R -t /tmp/binary100 -i 2540750000 -o 3460000000 -m 2 -a 0 -x 37
# hackrf_sweep -d TESTER -1 -w 333333 -f 5999:6019 -a 0 -l 32 -g 40
def main():
    write_bytes()
    capture_signal("2665:2685", "2664250000", "26", "16", "16")


if __name__ == "__main__":
    main()

