================================================
HackRF One
================================================

.. _hackrf_one:

.. image:: ../images/HackRF-One-fd0-0009.jpeg
  :alt: HackRF One

HackRF One was the first production hardware platform for the HackRF project. It is a Software Defined Radio peripheral capable of transmission or reception of radio signals from 1 MHz to 6 GHz. Designed to enable test and development of modern and next generation radio technologies, HackRF One is an open source hardware platform that can be used as a USB peripheral or programmed for stand-alone operation.

| `Product page <https://greatscottgadgets.com/hackrf/one/>`_
| `Where to buy <https://greatscottgadgets.com/hackrf/one/#purchasing>`_

Features
~~~~~~~~

    * half-duplex transceiver
    * operating freq: 1 MHz to 6 GHz
    * supported sample rates: 2 Msps to 20 Msps (quadrature)
    * resolution: 8 bits
    * interface: High Speed USB (with USB Micro-B connector)
    * power supply: USB bus power
    * software-controlled antenna port power (max 50 mA at 3.0 to 3.3 V)
    * SMA female antenna connector (50 ohms)
    * SMA female clock input and output for synchronization
    * convenient buttons for programming
    * pin headers for expansion
    * portable
    * open source


Maximum input power
~~~~~~~~~~~~~~~~~~~

The maximum input power of HackRF One is -5 dBm. Exceeding -5 dBm can result in permanent damage!

In theory, HackRF One can safely accept up to 10 dBm with the front-end RX amplifier disabled. However, a simple software or user error could enable the amplifier, resulting in permanent damage. It is better to use an external attenuator than to risk damage.


Minimum detectable input power
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This isn't a question that can be answered for a general purpose SDR platform such as HackRF. Any answer would be very specific to a particular application. For example, an answerable question might be: What is the minimum power level in dBm of modulation M at frequency F that can be detected by HackRF One with software S under configuration C at a bit error rate of no more than E%? Changing any of those variables (M, F, S, C, or E) would change the answer to the question. Even a seemingly minor software update might result in a significantly different answer. To learn the exact answer for a specific application, you would have to measure it yourself.

HackRF's concrete specifications include operating frequency range, maximum sample rate, and dynamic range in bits. These specifications can be used to roughly determine the suitability of HackRF for a given application. Testing is required to finely measure performance in an application. Performance can typically be enhanced significantly by selecting an appropriate antenna, external amplifier, and/or external filter for the application.


Typical maximum transmit power
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

HackRF One's maximum TX power varies by operating frequency:

    * 1 MHz to 10 MHz: 5 dBm to 15 dBm, generally increasing as frequency increases (see this `blog post <https://greatscottgadgets.com/2015/05-15-hackrf-one-at-1-mhz/>`__)
    * 10 MHz to 2170 MHz: 5 dBm to 15 dBm, generally decreasing as frequency increases
    * 2170 MHz to 2740 MHz: 13 dBm to 15 dBm
    * 2740 MHz to 4000 MHz: 0 dBm to 5 dBm, decreasing as frequency increases
    * 4000 MHz to 6000 MHz: -10 dBm to 0 dBm, generally decreasing as frequency increases

Through most of the frequency range up to 4 GHz, the maximum TX power is between 0 and 10 dBm. The frequency range with best performance is 2170 MHz to 2740 MHz.

Overall, the output power is enough to perform over-the-air experiments at close range or to drive an external amplifier. If you connect an external amplifier, you should also use an external bandpass filter for your operating frequency.

Before you transmit, know your laws. HackRF One has not been tested for compliance with regulations governing transmission of radio signals. You are responsible for using your HackRF One legally.
