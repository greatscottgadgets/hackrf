.. _troubleshooting:

===============
Troubleshooting
===============


How do I deal with the big spike in the middle of my spectrum?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Start by reading :ref:`our FAQ Response on the DC Spike <bigspike>`. After that, there are a few options:

    #. Ignore it. For many applications it isn't a problem. You'll learn to ignore it.

    #. Avoid it. The best way to handle DC offset for most applications is to use offset tuning; instead of tuning to your exact frequency of interest, tune to a nearby frequency so that the entire signal you are interested in is shifted away from 0 Hz but still within the received bandwidth. If your algorithm works best with your signal centered at 0 Hz (many do), you can shift the frequency in the digital domain, moving your signal of interest to 0 Hz and your DC offset away from 0 Hz. HackRF's high maximum sampling rate can be a big help as it allows you to use offset tuning even for relatively wideband signals.

    #. Correct it. There are various ways of removing the DC offset in software. However, these techniques may degrade parts of the signal that are close to 0 Hz. It may look better, but that doesn't necessarily mean that it is better from the standpoint of a demodulator algorithm, for example. Still, correcting the DC offset is often a good choice.


----


How should I set the gain controls for RX?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A good default setting to start with is RF=0 (off), IF=16, baseband=16. Increase or decrease the IF and baseband gain controls roughly equally to find the best settings for your situation. Turn on the RF amp if you need help picking up weak signals. If your gain settings are too low, your signal may be buried in the noise. If one or more of your gain settings is too high, you may see distortion (look for unexpected frequencies that pop up when you increase the gain) or the noise floor may be amplified more than your signal is.


----


What are the minimum system requirements for using HackRF?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The most important requirement is that you supply 500 mA at 5 V DC to your HackRF via the USB port. If your host computer has difficulty meeting this requirement, you may need to use a powered USB hub.

Most users will want to stream data to or from the HackRF at high speeds. This requires that the host computer supports Hi-Speed USB. Some Hi-Speed USB hosts are better than others, and you may have multiple host controllers on your computer. If you have difficulty operating your HackRF at high sample rates (10 Msps to 20 Msps), try using a different USB port on your computer. If possible, arrange things so that the HackRF is the only device on the bus.

There is no specific minimum CPU requirement for the host computer, but SDR is generally a CPU-intensive application. If you have a slower CPU, you may be unable to run certain SDR software or you may only be able to operate at lower sample rates.


----


Why isn't HackRF working with my virtual machine (VM)?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

HackRF requires the ability to stream data at very high rates over USB. Unfortunately VM software typically has problems with continuous high speed USB transfers.

There are some known bugs with the HackRF firmware's USB implementation. It is possible that fixing these bugs will improve the ability to operate HackRF with a VM, but there is a very good chance that operation at higher sample rates will still be limited.


----


What LEDs should be illuminated on the HackRF?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When HackRF One is plugged in to a USB host, four LEDs should turn on: 3V3, 1V8, RF, and USB. The 3V3 LED indicates that the primary internal power supply is working properly. The 1V8 and RF LEDs indicate that firmware is running and has switched on additional internal power supplies. The USB LED indicates that the HackRF One is communicating with the host over USB.

The RX and TX LEDs indicate that a receive or transmit operation is currently in progress.


----


.. _faq_hackrf_under_linux:

I can't seem to access my HackRF under Linux
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


If you run ``hackrf_info`` or any other command which tries to communicate with the HackRF and get one of the following error messages 

.. code-block :: sh

	hackrf_open() failed: HACKRF_ERROR_NOT_FOUND (-5)

or:

.. code-block :: sh

	hackrf_open() failed: HACKRF_ERROR_LIBUSB (-1000)

there are a few steps you can try:

#. Make sure that you are running the latest version of libhackrf and hackrf-tools. HackRF One, for example, is only supported by release 2014.04.1 or newer. Try running ``hackrf_info`` again to see if the updates have addressed your issue.

#. Write a udev rule to instruct udev to set permissions for the device in a way that it can be accessed by any user on the system who is a member of a specific group.

	A normal user under Linux doesn't have the permissions to access arbitrary USB devices because of security reasons. The first solution would be to run every command which tries to access the HackRF as root which is not recommended for daily usage, but at least shows you if your HackRF really works.

	To write a udev rule, you need to create a new rules file in the ``/etc/udev/rules.d`` folder. I called mine ``52-hackrf.rules``. Here is the content:

	.. code-block :: sh

		ATTR{idVendor}=="1d50", ATTR{idProduct}=="604b", SYMLINK+="hackrf-jawbreaker-%k", MODE="660", 	GROUP="plugdev"
		ATTR{idVendor}=="1d50", ATTR{idProduct}=="6089", SYMLINK+="hackrf-one-%k", MODE="660", GROUP="plugdev"
		ATTR{idVendor}=="1fc9", ATTR{idProduct}=="000c", SYMLINK+="hackrf-dfu-%k", MODE="660", GROUP="plugdev"

	The content of the file instructs udev to look out for devices with Vendor ID and Product ID matching HackRF devices. It then sets the UNIX permissions to ``660`` and the group to ``plugdev`` and creates a symlink in ``/dev`` to the device.

	After creating the rules file you can either reboot or run the command ``udevadm control --reload-rules`` as root to instruct udev to reload all rule files. After replugging your HackRF board, you should be able to access the device with all utilities as a normal user. If you still can't access the device, make sure that you are a member of the plugdev group.

	(These instructions have been tested on Ubuntu and Gentoo and may need to be adapted to other Linux distributions. In particular, your distro may have a group named something other than plugdev for this purpose.)

#. Disable USB autosuspend for HackRF. A common problem for laptop users could power management enabling USB autosuspend, which is likely if ``hackrf_info`` returns an error of ``hackrf_open() failed: Input/Output Error (-1000)`` on the first execution, and works if you run it a second time directly afterwards. This can be confirmed by running ``LIBUSB_DEBUG=3 hackrf_info`` and checking that the error is a ``broken pipe``. \

	If you use the TLP power manager you can add the HackRF USB VID/PIDs to the ``USB_BLACKLIST`` line in ``/etc/default/tlp`` (under Archlinux create a file ``/etc/tlp.d/10-usb-blacklist.conf``, under Ubuntu the config file can be found at ``/etc/tlp.conf``):

	.. code-block:: sh

		USB_BLACKLIST="1d50:604b 1d50:6089 1d50:cc15 1fc9:000c"

	and restart TLP using ``tlp restart`` or ``systemctl restart tlp``.


----


The command hackrf_info failed with "hackrf_open() .. HACKRF_ERROR_NOT_FOUND"
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This could be a problem of a kernel driver. Some ubuntu versions, like Ubuntu 15.04 with installed gnuradio has a kernel driver pre-installed. In this case you probably will get some syslog kernel messages like:

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
