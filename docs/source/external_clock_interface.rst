===========================================
External Clock Interface (CLKIN and CLKOUT)
===========================================

HackRF One produces a 10 MHz clock signal on CLKOUT. The signal is a 10 MHz square wave from 0 V to 3 V intended for a high impedance load.

The CLKIN port on HackRF One is a high impedance input that expects a 0 V to 3 V square wave at 10 MHz. Do not exceed 3.3 V or drop below 0 V on this input. Do not connect a clock signal at a frequency other than 10 MHz (unless you modify the firmware to support this). You may directly connect the CLKOUT port of one HackRF One to the CLKIN port of another HackRF One.

HackRF One uses CLKIN instead of the internal crystal when a clock signal is detected on CLKIN. The switch to or from CLKIN only happens when a transmit or receive operation begins.

To verify that a signal has been detected on CLKIN, use ``hackrf_debug --si5351c -n 0 -r``. The expected output with a clock detected is `[ 0] -> 0x01`. The expected output with no clock detected is `[ 0] -> 0x51`.
