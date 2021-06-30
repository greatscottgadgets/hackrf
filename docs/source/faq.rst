.. _faq:

================================================
FAQ
================================================

----

I can't seem to access my HackRF under Linux
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Question:** When running ``hackrf_info`` or any other command which tries to communicate with the HackRF, I get the following error message, although the board seems to be enumerated properly by the Linux kernel:

.. code-block :: sh

	hackrf_open() failed: HACKRF_ERROR_NOT_FOUND (-5)

or:

.. code-block :: sh

	hackrf_open() failed: HACKRF_ERROR_LIBUSB (-1000)

**Answer:**

Using the latest version
^^^^^^^^^^^^^^^^^^^^^^^^

First make sure that you are running the latest version of libhackrf and hackrf-tools. HackRF One, for example, is only supported by release 2014.04.1 or newer. Then check to see if ``hackrf_info`` is successful when running as root. If it is, then your other user is lacking permission.

Permission Problem
^^^^^^^^^^^^^^^^^^

A normal user under Linux doesn't have the permissions to access arbitrary USB devices because of security reasons. The first solution would be to run every command which tries to access the HackRF as root which is not recommended for daily usage, but at least shows you if your HackRF really works.

To fix this issue, you can write a udev rule to instruct udev to set permissions for the device in a way that it can be accessed by any user on the system who is a member of a specific group.

(The following things have been tested on Ubuntu and Gentoo and may need to be adapted to other Linux distributions. In particular, your distro may have a group named something other than plugdev for this purpose.)
To do that, you need to create a new rules file in the ``/etc/udev/rules.d`` folder. I called mine ``52-hackrf.rules``. Here is the content:

.. code-block :: sh

	ATTR{idVendor}=="1d50", ATTR{idProduct}=="604b", SYMLINK+="hackrf-jawbreaker-%k", MODE="660", 	GROUP="plugdev"
	ATTR{idVendor}=="1d50", ATTR{idProduct}=="6089", SYMLINK+="hackrf-one-%k", MODE="660", GROUP="plugdev"
	ATTR{idVendor}=="1fc9", ATTR{idProduct}=="000c", SYMLINK+="hackrf-dfu-%k", MODE="660", GROUP="plugdev"

The content of the file instructs udev to look out for devices with Vendor ID and Product ID matching HackRF devices. It then sets the UNIX permissions to ``660`` and the group to ``plugdev`` and creates a symlink in ``/dev`` to the device.

After creating the rules file you can either reboot or run the command ``udevadm control --reload-rules`` as root to instruct udev to reload all rule files. After replugging your HackRF board, you should be able to access the device with all utilities as a normal user. If you still can't access the device, make sure that you are a member of the plugdev group.

PyBOMBs
^^^^^^^

If you are using PyBOMBS, note that the HackRF recipe intentionally `does not install the udev rules <https://github.com/gnuradio/recipes_legacy/commit/a031078a52c038fc083dd3c107ef87df91803479#diff-8f163181a125b81f4d01e17217e471f3>`__ to `avoid installation failures <https://github.com/mossmann/hackrf/issues/190>`__ when run as non-root.

Power saving and USB autosuspend
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A common problem for laptop users could power management enabling USB autosuspend, which is likely if ``hackrf_info`` returns an error of ``hackrf_open() failed: Input/Output Error (-1000)`` on the first execution, and works if you run it a second time directly afterwards. This can be confirmed by running ``LIBUSB_DEBUG=3 hackrf_info`` and checking that the error is a ``broken pipe``.

To fix this, you need to disable USB autosuspend for HackRF. If you use the TLP power manager you can add the HackRF USB VID/PIDs to the ``USB_BLACKLIST`` line in ``/etc/default/tlp`` (under Archlinux create a file ``/etc/tlp.d/10-usb-blacklist.conf``, under Ubuntu the config file can be found at ``/etc/tlp.conf``):

.. code-block:: sh

	USB_BLACKLIST="1d50:604b 1d50:6089 1d50:cc15 1fc9:000c"

and restart TLP using ``tlp restart`` or ``systemctl restart tlp``.


hackrf kernel module
^^^^^^^^^^^^^^^^^^^^

If the command ``hackrf_info`` failed with "hackrf_open() .. HACKRF_ERROR_NOT_FOUND" error message, this could be also a problem of a kernel driver. Some ubuntu versions, like Ubuntu 15.04 with installed gnuradio has a kernel driver pre-installed. In this case you probably will get some syslog kernel messages like:

    * kernel: [ 8932.297074] hackrf 1-9.4:1.0: Board ID: 02
    * kernel: [ 8932.297076] hackrf 1-9.4:1.0: Firmware version: 2014.08.1
    * kernel: [ 8932.297261] hackrf 1-9.4:1.0: Registered as swradio0
    * kernel: [ 8932.297262] hackrf 1-9.4:1.0: SDR API is still slightly experimental and functionality changes may follow

when you plug in the the HackRF module. Use the command ``dmesg`` to check the last system log entries. If you try to start ``hackrf_info`` it will terminate with the error message and the system log will show a message like:

    * kernel: [ 8967.263268] usb 1-9.4: usbfs: interface 0 claimed by hackrf while 'hackrf_info' sets config #1

To solve this issue check under root account if is there is a kernel module ``hackrf`` loaded: ``lsmod | grep hackrf``. If there is a hackrf kernel module, try to unload it with ``rmmod hackrf``. You must do this command as root, too. After this the command ``hackrf_info`` (and all other hackrf related stuff) should work and the syslog usbfs massage should vanish.

After a reset or USB unplug/plug this kernel module will load again and block the access again. To solve this you have to blacklist the hackrf kernel module in /etc/modprobe.d/blacklist(.conf) The current filename of the blacklist file may differ, it depends on the current ubuntu version. In ubuntu 15.04 it is located in /etc/modprobe.d/blacklist.conf. Open this file under root account with a text editor an add the following line at the end:

.. code-block:: sh

	blacklist hackrf

After a system-restart, to get the updated modprobe working, the hackrf worked under ubuntu 15.04 with the upstream packages (Firmware version: 2014.08.1) out-of-the-box.

----

hackrf_set_sample_rate fails
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Question:** I'm trying to run ``hackrf_transfer`` and hackrf_set_sample_rate fails. The libusb_control_transfer call in hackrf_set_sample_rate_manual is returning with LIBUSB_ERROR_PIPE.

**Answer:** Follow the instructions to update your firmware.

----

What is the big spike in the center of my received spectrum?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Question:** I see a large spike in the center of my FFT display regardless of the frequency my HackRF is tuned to. Is there something wrong with my HackRF?

**Answer:** You are seeing a DC offset (or component or bias). The term "DC" comes from "Direct Current" in electronics. It is the unchanging aspect of a signal as opposed to the "alternating" part of the signal (AC) that changes over time. Take, for example, the signal represented by the digital sequence:

.. code-block:: sh

	-2, -1, 1, 6, 8, 9, 8, 6, 1, -1, -2, -1, 1, 6, 8, 9, 8, 6, 1, -1, -2, -1, 1, 6, 8, 9, 8, 6, 1, -1

This periodic signal contains a strong sinusoidal component spanning from -2 to 9. If you were to plot the spectrum of this signal, you would see one spike at the frequency of this sinusoid and a second spike at 0 Hz (DC). If the signal spanned from values -2 to 2 (centered around zero), there would be no DC offset. Since it is centered around 3.5 (the number midway between -2 and 9), there is a DC component.

Samples produced by HackRF are measurements of radio waveforms, but the measurement method is prone to a DC bias introduced by HackRF. It's an artifact of the measurement system, not an indication of a received radio signal. DC offset is not unique to HackRF; it is common to all quadrature sampling systems.

There was a bug in the HackRF firmware (through release 2013.06.1) that made the DC offset worse than it should have been. In the worst cases, certain Jawbreakers experienced a DC offset that drifted to a great extreme over several seconds of operation. This bug has been fixed. The fix reduces DC offset but does not do away with it entirely. It is something you have to live with when using any quadrature sampling system like HackRF.

A high DC offset is also one of a few symptoms that can be caused by a software version mismatch. A common problem is that people run an old version of gr-osmosdr with newer firmware.

----

How do I deal with the DC offset?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Question:** Okay, now that I understand what that big spike in the middle of my spectrum is, how do I handle it?

**Answer:** There are a few options:

    #. Ignore it. For many applications it isn't a problem. You'll learn to ignore it.

    #. Avoid it. The best way to handle DC offset for most applications is to use offset tuning; instead of tuning to your exact frequency of interest, tune to a nearby frequency so that the entire signal you are interested in is shifted away from 0 Hz but still within the received bandwidth. If your algorithm works best with your signal centered at 0 Hz (many do), you can shift the frequency in the digital domain, moving your signal of interest to 0 Hz and your DC offset away from 0 Hz. HackRF's high maximum sampling rate can be a big help as it allows you to use offset tuning even for relatively wideband signals.

    #. Correct it. There are various ways of removing the DC offset in software. However, these techniques may degrade parts of the signal that are close to 0 Hz. It may look better, but that doesn't necessarily mean that it is better from the standpoint of a demodulator algorithm, for example. Still, correcting the DC offset is often a good choice.

----

Purchasing HackRF
~~~~~~~~~~~~~~~~~

**Question:** Where can I buy HackRF?

**Answer:** HackRF is designed and manufactured by Great Scott Gadgets. Please see `http://greatscottgadgets.com/hackrf/ <http://greatscottgadgets.com/hackrf/>`__ for availability. HackRF is open source hardware, so you could also build your own.

----

Making sense of gain settings
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Question:** What gain controls are provided by HackRF?

**Answer:** HackRF (both Jawbreaker and One) provides three different analog gain controls on RX and two on TX. The three RX gain controls are at the RF ("amp", 0 or 14 dB), IF ("lna", 0 to 40 dB in 8 dB steps), and baseband ("vga", 0 to 62 dB in 2 dB steps) stages. The two TX gain controls are at the RF (0 or 14 dB) and IF (0 to 47 dB in 1 dB steps) stages.

**Question:** Why is the RF gain setting restricted to two values?

**Answer:** HackRF has two RF amplifiers close to the antenna port, one for TX and one for RX. These amplifiers have two settings: on or off. In the off state, the amps are completely bypassed. They nominally provide 14 dB of gain when on, but the actual amount of gain varies by frequency. In general, expect less gain at higher frequencies. For fine control of gain, use the IF and/or baseband gain options.

**Question:** How should I set the gain controls for RX?

**Answer:** A good default setting to start with is RF=0 (off), IF=16, baseband=16. Increase or decrease the IF and baseband gain controls roughly equally to find the best settings for your situation. Turn on the RF amp if you need help picking up weak signals. If your gain settings are too low, your signal may be buried in the noise. If one or more of your gain settings is too high, you may see distortion (look for unexpected frequencies that pop up when you increase the gain) or the noise floor may be amplified more than your signal is.

----

System Requirements
~~~~~~~~~~~~~~~~~~~


**Question:** What are the minimum system requirements for using HackRF?

**Answer:** The most important requirement is that you supply 500 mA at 5 V DC to your HackRF via the USB port. If your host computer has difficulty meeting this requirement, you may need to use a powered USB hub.

Most users will want to stream data to or from the HackRF at high speeds. This requires that the host computer supports Hi-Speed USB. Some Hi-Speed USB hosts are better than others, and you may have multiple host controllers on your computer. If you have difficulty operating your HackRF at high sample rates (10 Msps to 20 Msps), try using a different USB port on your computer. If possible, arrange things so that the HackRF is the only device on the bus.

There is no specific minimum CPU requirement for the host computer, but SDR is generally a CPU-intensive application. If you have a slower CPU, you may be unable to run certain SDR software or you may only be able to operate at lower sample rates.

**Question:** Why doesn't HackRF work properly with a virtual machine (VM)?

**Answer:** HackRF requires the ability to stream data at very high rates over USB. Unfortunately VM software typically has problems with continuous high speed USB transfers.

There are some known bugs with the HackRF firmware's USB implementation. It is possible that fixing these bugs will improve the ability to operate HackRF with a VM, but there is a very good chance that operation at higher sample rates will still be limited.

----

LEDs
~~~~

**Question:** What LEDs should be illuminated?

**Answer:** When HackRF One is plugged in to a USB host, four LEDs should turn on: 3V3, 1V8, RF, and USB. The 3V3 LED indicates that the primary internal power supply is working properly. The 1V8 and RF LEDs indicate that firmware is running and has switched on additional internal power supplies. The USB LED indicates that the HackRF One is communicating with the host over USB.

The RX and TX LEDs indicate that a receive or transmit operation is currently in progress.

**Question:** Why are the LEDs different colors?

**Answer:** Each LED is a single color. There are no multi-colored LEDs on HackRF One. Adjacent LEDs are different colors in order to make them easier to distinguish from one another. The colors do not mean anything.

----

Half-Duplex, Full-Duplex
~~~~~~~~~~~~~~~~~~~~~~~~

**Question:** Is HackRF One half-duplex or full-duplex?

**Answer:** HackRF One is a half-duplex transceiver. This means that it can transmit or receive but not both at the same time.

**Question:** Why isn't HackRF One full-duplex?

**Answer:** HackRF One is designed to support the widest possible range of SDR applications in a single, low cost, portable device. Many applications do not require full-duplex operation. Full-duplex support would have made HackRF larger and more expensive, and it would have required an external power supply. Since full-duplex needs can be met by simply using a second HackRF One, it made sense to keep the device small, portable, and low cost for everyone who does not require full-duplex operation.

**Question:** How could the HackRF One design be changed to make it full-duplex?

**Answer:** The HackRF One hardware design is actually full-duplex (at lower sample rates) from the USB connection through the ADC/DAC. The RF section is the only part of the design that cannot support full-duplex operation. The easiest way to make HackRF One full-duplex would be to create an add-on board that duplicates the RF section and also provides an external power input (from a wall wart, for example) for the additional power required. This would also require software effort; the firmware, CPLD, libhackrf, and other host software would all need work to support full-duplex operation.

If you were to try to redesign the RF section on HackRF One to support full-duplex, the main thing to focus on would be the MAX2837 (intermediate frequency transceiver). This part is half-duplex, so you would either need two of them or you would have to redesign the RF section to use something other than the MAX2837, likely resulting in a radically different design. If you used two MAX2837s you might be able to use one RFFC5071 instead of two RFFC5072s.

----

What is the receive sensibility of HackRF?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Question:** What is the minimum signal power level that can be detected by HackRF?

**Answer:** This isn't a question that can be answered for a general purpose SDR platform such as HackRF. Any answer would be very specific to a particular application. For example, an answerable question might be: What is the minimum power level in dBm of modulation M at frequency F that can be detected by HackRF One with software S under configuration C at a bit error rate of no more than E%? Changing any of those variables (M, F, S, C, or E) would change the answer to the question. Even a seemingly minor software update might result in a significantly different answer. To learn the exact answer for a specific application, you would have to measure it yourself.

HackRF's concrete specifications include operating frequency range, maximum sample rate, and dynamic range in bits. These specifications can be used to roughly determine the suitability of HackRF for a given application. Testing is required to finely measure performance in an application. Performance can typically be enhanced significantly by selecting an appropriate antenna, external amplifier, and/or external filter for the application.

----

Troubleshooting
~~~~~~~~~~~~~~~


**Question:** Why is a known signal at an incorrect frequency which changes at a surprising rate when changing the center frequency?

**Answer:** You may have a version mismatch between the firmware and CPLD bitstream. [Update your firmware to the latest release](Updating Firmware) to solve this problem.