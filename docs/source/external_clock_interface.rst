========================
External Clock Interface
========================

.. _external_clock_interface:

HackRF Pro
~~~~~~~~~~

HackRF Pro has two configurable SMA ports, P1 and P2. By default, P1 is configured as CLKIN and P2 as CLKOUT. The default behaviour of these signals is as described for HackRF One below.

HackRF One
~~~~~~~~~~

HackRF One produces a 10 MHz clock signal on CLKOUT. The signal is a 3.3 V, 10 MHz square wave intended for a high impedance load.

The CLKIN port on HackRF One is a high impedance input that expects 3.3 V square wave at 10 MHz. Do not exceed 3.3 V or drop below 0 V on this input. Do not connect a clock signal at a frequency other than 10 MHz (unless you modify the firmware to support this). You may directly connect the CLKOUT port of one HackRF One to the CLKIN port of another HackRF One.

HackRF One uses CLKIN instead of the internal crystal when a clock signal is detected on CLKIN. The switch to or from CLKIN only happens when a transmit or receive operation begins.

To verify that a signal has been detected on CLKIN, use ``hackrf_clock -i``. The expected output with a clock detected is `CLKIN status: clock signal detected`. The expected output with no clock detected is `CLKIN status: no clock signal detected`.

To activate CLKOUT, use ``hackrf_clock -o 1``. To switch it off, use ``hackrf_clock -o 0``.
