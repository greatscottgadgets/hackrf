================================================
LPC43xx USB Implementation
================================================

The NXP LPC43xx has a fairly sophisticated USB peripheral. It can transmit and receive chains of large buffers (by microcontroller standards), completely independently of the processor. This is excellent, as we'll want to reserve the processor's computational power for doing interesting things with the data.



Initial Experimentation and Discovery
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

My first attempt at USB device software was to use the built-in, ROM-based USB API. I got the part to show up on my host, but quickly ran into problems with old headers that didn't match the API function signatures and data structures. I got a much better, more up-to-date set of code to work with from the `LPCware Web site's git repository <http://sw.lpcware.com/>`__. I then attempted to use the USB DFU support built in to the ROM, but quickly abandoned that in favor of doing some basic performance-testing of the USB peripheral.

To save time, I started with the nxpUSBLib USB library, which is a port of Dean Camera's LUFA. This port had LPC18xx support already, but no official LPC43xx support. I hacked together support by copying, renaming, and modifying the LPC18xx-specific parts. For USB descriptors, I emulated the FT2232H, a high-speed USB-to-FIFO IC that uses large bulk transfers to move data at a rate of 35MiB/second or more.

(more to come)