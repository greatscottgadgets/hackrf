================================================
Design Goals
================================================

Eventually, the HackRF project may result in multiple hardware designs, but the initial goal is to build a single wideband transceiver peripheral that can be attached to a general purpose computer for software radio functions.

Primary goals:

    * half-duplex transceiver
    * operating freq: 100 MHz to 6 GHz
    * maximum sample rate: 20 Msps
    * resolution: 8 bits
    * interface: High Speed USB
    * power supply: USB bus power
    * portable
    * open source

Wish list:

    * full-duplex (at reduced max sample rate)
    * external clock reference
    * dithering
    * parallel interface for external FPGA, etc.

If there is a primary goal we miss, it will probably be the operating frequency range. The wideband front end is the part of the design furthest from completion. At an absolute minimum, the board should do 900 MHz and 2.4 GHz.

The design is FPGA-less. There will be a tiny bit of DSP capability (ARM Cortex-M4), but mostly we're just trying to get samples to and from a host computer.

We are trading resolution and DSP capability for cost, portability, and frequency range. Considering that we'll be able to support oversampling for many applications and that we should be able to implement AGC, it should be a pretty good trade.
