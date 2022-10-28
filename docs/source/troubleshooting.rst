.. _troubleshooting:

===============
Troubleshooting
===============

Why isn't my HackRF One detectable after I plug it into my computer?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If your HackRF One isn't immediately detectable it is very possible that your Micro USB cable is not meeting HackRF One's requirements. HackRF One requires quite a bit of supply current and solid USB 2.0 high speed communications to operate. It is common for HackRF One to reveal cables with deficiencies such as carrying power but not data, carrying data but not enough power, etc. Please try multiple cables to resolve this issue. More than once people have gotten their HackRF One to work after trying their fifth cable. 


----


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
