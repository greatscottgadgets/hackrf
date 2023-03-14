================================================
HackRF One
================================================

.. image:: ../images/HackRF-One-fd0-0009.jpeg
  :alt: HackRF One

HackRF One is the current hardware platform for the HackRF project. It is a Software Defined Radio peripheral capable of transmission or reception of radio signals from 1 MHz to 6 GHz. Designed to enable test and development of modern and next generation radio technologies, HackRF One is an open source hardware platform that can be used as a USB peripheral or programmed for stand-alone operation.



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



Minimum System Requirements for using HackRF
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

HackRF requires you to supply 500 mA at 5 V DC to your HackRF via the USB port. If your host computer has difficulty meeting this requirement, you may need to use a powered USB hub.

There is no specific minimum CPU requirement for the host computer when using a HackRF, but SDR is generally a CPU-intensive application. If you have a slower CPU, you may be unable to run certain SDR software or you may only be able to operate at lower sample rates.

Most users will want to stream data to or from the HackRF at high speeds. This requires that the host computer supports Hi-Speed USB. Some Hi-Speed USB hosts are better than others, and you may have multiple host controllers on your computer. If you have difficulty operating your HackRF at high sample rates (10 Msps to 20 Msps), try using a different USB port on your computer. If possible, arrange things so that the HackRF is the only device on the bus.


