.. _updating_firmware:

================================================
Updating Firmware
================================================

HackRF devices ship with firmware on the SPI flash memory. The firmware can be updated with a USB cable and host computer.

These instructions allow you to upgrade the firmware in order to take advantage of new features or bug fixes.



Updating the SPI Flash Firmware
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To update the firmware on a working HackRF One, use the hackrf_spiflash program:

.. code-block :: sh

	hackrf_spiflash -w hackrf_one_usb.bin

You can find the firmware binary (hackrf_one_usb.bin) in the firmware-bin directory of the latest `release package <https://github.com/greatscottgadgets/hackrf/releases/latest>`__ or you can compile your own from the `source <https://github.com/greatscottgadgets/hackrf/tree/master/firmware>`__. For Jawbreaker, use hackrf_jawbreaker_usb.bin. If you compile from source, the file will be called hackrf_usb.bin.

The hackrf_spiflash program is part of hackrf-tools.

When writing a firmware image to SPI flash, be sure to select firmware with a filename ending in ".bin".

After writing the firmware to SPI flash, you may need to reset the HackRF device by pressing the RESET button or by unplugging it and plugging it back in.

If you get an error that mentions HACKRF_ERROR_NOT_FOUND, it is often a permissions problem on your OS.



Only if Necessary: Recovering the SPI Flash Firmware
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If the firmware installed in SPI flash has been damaged or if you are programming a home-made HackRF for the first time, you will not be able to immediately use the hackrf_spiflash program as listed in the above procedure. Follow these steps instead:

    #. Follow the DFU Boot instructions to start the HackRF in DFU boot mode.
    #. Type ``dfu-util --device 1fc9:000c --alt 0 --download hackrf_one_usb.dfu`` to load firmware from a release package into RAM. If you have a Jawbreaker, use hackrf_jawbreaker_usb.dfu instead. Alternatively, use ``make -e BOARD=HACKRF_ONE RUN_FROM=RAM program`` to load the firmware into RAM and start it.
    #. Follow the SPI flash firmware update procedure above to write the ".bin" firmware image to SPI flash. 



Only if Necessary: DFU Boot
~~~~~~~~~~~~~~~~~~~~~~~~~~~

DFU boot mode is normally only needed if the firmware is not working properly or has never been installed.

The LPC4330 microcontroller on HackRF is capable of booting from several different code sources. By default, HackRF boots from SPI flash memory (SPIFI). It can also boot HackRF in DFU (USB) boot mode. In DFU boot mode, HackRF will enumerate over USB, wait for code to be delivered using the DFU (Device Firmware Update) standard over USB, and then execute that code from RAM. The SPIFI is normally unused and unaltered in DFU mode.

To start up HackRF One in DFU mode, hold down the DFU button while powering it on or while pressing and releasing the RESET button. Release the DFU button after the 3V3 LED illuminates. The 1V8 LED should remain off. At this point HackRF One is ready to receive firmware over USB.

To start up Jawbreaker in DFU mode, short two pins on one of the "BOOT" headers while power is first supplied. The pins that must be shorted are pins 1 and 2 of header P32 on Jawbreaker. Header P32 is labeled "P2_8" on most Jawbreakers but may be labeled "2" on prototype units. Pin 1 is labeled "VCC". Pin 2 is the center pin. After DFU boot, you should see VCCLED illuminate and note that 1V8LED does not illuminate. At this point Jawbreaker is ready to receive firmware over USB.

You should only use a firmware image with a filename ending in ".dfu" over DFU, not firmware ending in ".bin".



Obtaining DFU-Util
~~~~~~~~~~~~~~~~~~

On fresh installs of your OS, you may need obtain a copy of DFU-Util. For most Linux distributions it should be available as a package, for example on Debian/Ubuntu

.. code-block :: sh

	sudo apt-get install dfu-util

If you are using a platform without a dfu-util package, build instruction can be found `here on the dfu-util source forge build page <http://dfu-util.sourceforge.net/build.html>`__.

.. code-block :: sh

	cd ~
	sudo apt-get build-dep dfu-util
	sudo apt-get install libusb-1.0-0-dev
	git clone git://git.code.sf.net/p/dfu-util/dfu-util
	cd dfu-util
	./autogen.sh
	./configure
	make
	sudo make install

Now you will have the current version of DFU Util installed on your system.



Updating the CPLD
~~~~~~~~~~~~~~~~~

Older versions of HackRF firmware (prior to release 2021.03.1) require an additional step to program a bitstream into the CPLD.

To update the CPLD image, first update the SPI flash firmware, libhackrf, and hackrf-tools to the version you are installing. Then:

.. code-block :: sh

	hackrf_cpldjtag -x firmware/cpld/sgpio_if/default.xsvf

After a few seconds, three LEDs should start blinking. This indicates that the CPLD has been programmed successfully. Reset the HackRF device by pressing the RESET button or by unplugging it and plugging it back in.