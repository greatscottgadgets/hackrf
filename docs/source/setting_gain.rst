============================
Setting Gain Controls for RX
============================

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



A good default setting to start with is RF=0 (off), IF=16, baseband=16. Increase or decrease the IF and baseband gain controls roughly equally to find the best settings for your situation. Turn on the RF amp if you need help picking up weak signals. If your gain settings are too low, your signal may be buried in the noise. If one or more of your gain settings is too high, you may see distortion (look for unexpected frequencies that pop up when you increase the gain) or the noise floor may be amplified more than your signal is.
