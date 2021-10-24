==================================
LPC43xx USB DFU Notes
==================================

The LPC43xx contains USB DFU bootloader support in ROM. By selecting the appropriate boot mode (USB0), the device will come up on USB at power-up or reset, and implement the popular and well-documented `USB DFU protocol <http://www.usb.org/developers/docs/devclass_docs/DFU_1.1.pdf>`__.



Setup
~~~~~

Boot Mode Jumpers
^^^^^^^^^^^^^^^^^

Set the boot mode jumpers to USB0: BOOT[0:3] = "1010". Jumper setting takes effect at the next power-on or reset. (You can rig up a reset button to P2 "LPC_JTAG" or P14 "LPC_ISP".)



USB DFU Utility
^^^^^^^^^^^^^^^

`http://dfu-util.gnumonks.org/ <http://dfu-util.gnumonks.org/>`__

You need the latest source from git for some reason.



Usage Notes
~~~~~~~~~~~

Makefile
^^^^^^^^

The firmware Makefile contains a "%.dfu" and a "program" target. The program target will make the DFU file and then attempt to load the code to a device that's attached via USB and in DFU mode.

Manually
^^^^^^^^

DFU boot downloads to RAM then executes. Need a DFU suffix, but also a header

.. code-block :: sh

	cp blinky.bin blinky.dfu

Add DFU suffix and address prefix:

.. code-block :: sh

	/usr/local/bin/dfu-suffix --pid=0x000c --vid=0x1fc9 --did=0x0 -s 0 -a blinky.dfu

Create header (based on UM10503 section 5.3.3 "Boot image header format"):

.. code-block :: sh

	echo "0000000: da ff {blocksL} {blocksH} {hash0} {hash1} {hash2} {hash3}" | xxd -g1 -r > header.bin

where {blocksL} and {blocksH} are the low and high bytes of the length of the .bin file + 16 bytes, measured in 512-byte frames. "02 00" means 2 x 512 + 16 bytes. (this should be automated.) Assume rounded up.

{hash0}..{hash3} = 0xFF. The value is not used, based on the HASH_ACTIVE field in the first two bytes.

Add header

.. code-block :: sh

	cat header.bin blinky.dfu > load.dfu 

Check for device:

.. code-block :: sh

	/usr/local/bin/dfu-util -l

Download (I used the latest dfu-util from git)

.. code-block :: sh

	/usr/local/bin/dfu-util -d 1fc9:000c -a 0 -D load.dfu

Also make sure that the binary is linked for execution at address 0x00000000. This is the default for our linker scripts.



Jellybean Notes
~~~~~~~~~~~~~~~

A 12 MHz clock signal must be supplied to the LPC4330 at boot time.

The April 16, 2012 Jellybean design has a voltage divider for VBUS that is fed to the LPC43xx to detect that a USB host is present. Measuring the divided voltage, the USB0_VBUS only reaches 0.4V, which fails to trip the USB bootloader code. It looks like NXP's intent is to directly connect VBUS to the USB0_VBUS. Indeed, when I shorted R10 with tweezers, the USB bootloader sprung into action. I would recommend changing R10 to 0 Ohms and R11 to DNP, or removing R10 and R11 altogether.