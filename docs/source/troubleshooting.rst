.. _troubleshooting:

================================================
Troubleshooting
================================================

.. _bigspike:

There is a big spike in the center of the received spectrum
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you see a large spike in the center of your FFT display regardless of the frequenecy you are tuned to, you are seeing a DC offset (or component or bias). The term "DC" comes from "Direct Current" in electronics. It is the unchanging aspect of a signal as opposed to the "alternating" part of the signal (AC) that changes over time.

.. figure:: ../images/dc_spike_grc.png
   :align: center

   DC spike


Take, for example, the signal represented by the digital sequence:

.. code-block:: sh

	-2, -1, 1, 6, 8, 9, 8, 6, 1, -1, -2, -1, 1, 6, 8, 9, 8, 6, 1, -1, -2, -1, 1, 6, 8, 9, 8, 6, 1, -1

.. figure:: ../images/dc_spike_example_plot.png
   :align: center

   Example signal

This periodic signal contains a strong sinusoidal component spanning from -2 to 9. If we plot the spectrum of this signal, you can see one spike at the frequency of this sinusoid and a second spike at 0 Hz (DC).


.. figure:: ../images/dc_spike_example_spectrum.png
   :align: center

   Spectrum of example signal

If the signal spanned from values -2 to 2 (centered around zero), there would be no DC offset. Since it is centered around 3.5 (the number midway between -2 and 9), there is a DC component.

Samples produced by HackRF are measurements of radio waveforms, but the measurement method is prone to a DC bias introduced by HackRF. It's an artifact of the measurement system, not an indication of a received radio signal. DC offset is not unique to HackRF; it is common to all quadrature sampling systems.

There was a bug in the HackRF firmware (through release 2013.06.1) that made the DC offset worse than it should have been. In the worst cases, certain Jawbreakers experienced a DC offset that drifted to a great extreme over several seconds of operation. This bug has been fixed. The fix reduces DC offset but does not do away with it entirely. It is something you have to live with when using any quadrature sampling system like HackRF.

A high DC offset is also one of a few symptoms that can be caused by a software version mismatch. A common problem is that people run an old version of gr-osmosdr with newer firmware.

Solution
--------

There are a few options:

    #. Ignore it. For many applications it isn't a problem. You'll learn to ignore it.

    #. Avoid it. The best way to handle DC offset for most applications is to use offset tuning; instead of tuning to your exact frequency of interest, tune to a nearby frequency so that the entire signal you are interested in is shifted away from 0 Hz but still within the received bandwidth. If your algorithm works best with your signal centered at 0 Hz (many do), you can shift the frequency in the digital domain, moving your signal of interest to 0 Hz and your DC offset away from 0 Hz. HackRF's high maximum sampling rate can be a big help as it allows you to use offset tuning even for relatively wideband signals.

    #. Correct it. There are various ways of removing the DC offset in software. However, these techniques may degrade parts of the signal that are close to 0 Hz. It may look better, but that doesn't necessarily mean that it is better from the standpoint of a demodulator algorithm, for example. Still, correcting the DC offset is often a good choice.
