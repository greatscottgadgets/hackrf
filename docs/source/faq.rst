.. _faq:

================================================
FAQ
================================================

.. _bigspike:

What is the big spike in the center of my received spectrum?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you see a large spike in the center of your FFT display regardless of the frequenecy you are tuned to, you are seeing a DC offset (or component or bias). The term "DC" comes from "Direct Current" in electronics. It is the unchanging aspect of a signal as opposed to the "alternating" part of the signal (AC) that changes over time. Take, for example, the signal represented by the digital sequence:

.. code-block:: sh

	-2, -1, 1, 6, 8, 9, 8, 6, 1, -1, -2, -1, 1, 6, 8, 9, 8, 6, 1, -1, -2, -1, 1, 6, 8, 9, 8, 6, 1, -1

This periodic signal contains a strong sinusoidal component spanning from -2 to 9. If you were to plot the spectrum of this signal, you would see one spike at the frequency of this sinusoid and a second spike at 0 Hz (DC). If the signal spanned from values -2 to 2 (centered around zero), there would be no DC offset. Since it is centered around 3.5 (the number midway between -2 and 9), there is a DC component.

Samples produced by HackRF are measurements of radio waveforms, but the measurement method is prone to a DC bias introduced by HackRF. It's an artifact of the measurement system, not an indication of a received radio signal. DC offset is not unique to HackRF; it is common to all quadrature sampling systems.

There was a bug in the HackRF firmware (through release 2013.06.1) that made the DC offset worse than it should have been. In the worst cases, certain Jawbreakers experienced a DC offset that drifted to a great extreme over several seconds of operation. This bug has been fixed. The fix reduces DC offset but does not do away with it entirely. It is something you have to live with when using any quadrature sampling system like HackRF.

A high DC offset is also one of a few symptoms that can be caused by a software version mismatch. A common problem is that people run an old version of gr-osmosdr with newer firmware.



----



How do I deal with the big spike in the middle of my spectrum?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Start by reading :ref:`our FAQ Response on the DC Spike <bigspike>`. After that, there are a few options:

    #. Ignore it. For many applications it isn't a problem. You'll learn to ignore it.

    #. Avoid it. The best way to handle DC offset for most applications is to use offset tuning; instead of tuning to your exact frequency of interest, tune to a nearby frequency so that the entire signal you are interested in is shifted away from 0 Hz but still within the received bandwidth. If your algorithm works best with your signal centered at 0 Hz (many do), you can shift the frequency in the digital domain, moving your signal of interest to 0 Hz and your DC offset away from 0 Hz. HackRF's high maximum sampling rate can be a big help as it allows you to use offset tuning even for relatively wideband signals.

    #. Correct it. There are various ways of removing the DC offset in software. However, these techniques may degrade parts of the signal that are close to 0 Hz. It may look better, but that doesn't necessarily mean that it is better from the standpoint of a demodulator algorithm, for example. Still, correcting the DC offset is often a good choice.



---



What gain controls are provided by HackRF?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

HackRF (both Jawbreaker and One) provides three different analog gain controls on RX and two on TX.

The three RX gain controls are at these stages:

- RF ("amp", 0 or ~11 dB)
- IF ("lna", 0 to 40 dB in 8 dB steps)
- baseband ("vga", 0 to 62 dB in 2 dB steps)
 
The two TX gain controls are at these stages:

- RF (0 or ~11 dB)
- IF (0 to 47 dB in 1 dB steps)

Note: in some documents, the RF gain was erroneously quoted to be 14 dB. The confusion was based on the fact that the MGA-81563 amplifier is advertised as a "14 dBm" amplifier, but that specifies its output power, not its amplification. See `Martin Ling's comment on issue #1059 <https://github.com/greatscottgadgets/hackrf/issues/1059#issuecomment-1060038293>`__ for some details!

----


Why is the RF gain setting restricted to two values?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

HackRF has two RF amplifiers close to the antenna port, one for TX and one for RX. These amplifiers have two settings: on or off. In the off state, the amps are completely bypassed. They nominally provide around 11 dB of gain when on, but the actual amount of gain varies by frequency. In general, expect less gain at higher frequencies. For fine control of gain, use the IF and/or baseband gain options.


----


Why are the LEDs on HackRF different colours?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Each LED is a single color. There are no multi-colored LEDs on HackRF One. Adjacent LEDs are different colors in order to make them easier to distinguish from one another. The colors do not mean anything.


----


Where can I purchase HackRF?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

HackRF is designed and manufactured by Great Scott Gadgets. We do not sell low volumes of HackRFs to people individually; instead we have agreements with specific resellers. Please see our reseller list on the Great Scott Gadgets website for availability: `http://greatscottgadgets.com/hackrf/ <http://greatscottgadgets.com/hackrf/>`__. 

HackRF is open source hardware, so you can also build your own.
