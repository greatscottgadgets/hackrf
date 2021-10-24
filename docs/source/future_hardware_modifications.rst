================================================
Future Hardware Modifications
================================================

Things to consider for post-Jawbreaker hardware designs:

----

Antenna
^^^^^^^

The PCB antenna on Jawbreaker was included to facilitate beta testing. Future designs likely will not include a PCB antenna.

SMA connectors will be PCB edge-mounted.

----

Baseband
^^^^^^^^

The interfaces between the MAX2837 and MAX5864 have some signals inverted. Theoretically, that's fine if compensated for in software. However, I'm theorizing that RX/ADC DC offset compensation assumes that both channels have the same DC polarity. I've fixed the inversion in the CPLD. However, a PCB experiment should be conducted to see if the DC offset is reduced by un-inverting the RX Q channel connections to the MAX5864.

----

CPLD
^^^^

The CPLD could be removed, but some sort of multiplexer would be needed to meet the MAX5864 i/o requirements. Depending on the particular LPC43xx part used, it might be possible to use the System Control Unit (SCU) for this.

----

Clocking
^^^^^^^^

The clock signal from the Si5351C to the LPC43xx's GP_CLKIN pin may need different passives, but the documentation on that clock input is thin (acceptable peak-to-peak voltage anyone?).

An unpopulated footprint for a 32.768 kHz RTC crystal would be nice. Also break out RTC battery pins to an expansion header.

----

USB
^^^

Would support for host mode on the second USB PHY be useful somehow? This is only possible with a larger LPC43xx package that exposes the second PHY's ULPI signals. Unless, of course, a mere full-speed PHY is acceptable.

----

Power Management
^^^^^^^^^^^^^^^^

The MAX5864 appears to come up in "Tx" or "Rcvr" mode -- I have observed that the part will pass DA bus data to ID/QD without any SPI configuration. If we're worried about USB power and minimizing current consumption, it might be good to have this device on a power regulator with an ENABLE pin, or have a FET power switch. Yes, let's add a high side switch for the whole RF section.

----

Regulators
^^^^^^^^^^

U21 (the TPS62410) FB1 pin is connected on the far side of jumper P8 (VCC), which puts the jumper inside the feedback path. If the jumper trace is cut, the regulator may go nuts because the FB pin is floating.

----

Buttons
^^^^^^^

Add a reset button (for the LPC43xx). Maybe add a DFU button too.

----

Shielding
^^^^^^^^^

Maybe add a can around the RF section.

----

Footprints
^^^^^^^^^^

Tighten up holes for USB connector support legs to improve placement consistency. Make some of the QFN pads bigger (especially on the RF switches) for better soldering.

----

Shield Support
^^^^^^^^^^^^^^

If support for add-on shields is considered valuable, here are some tweaks I'd suggest:

Any reason P28 (SD) pin 12 isn't grounded or doing something useful? Same goes for P25 (LPC_ISP) pin 3 -- maybe make it VCC, the signaling voltage for the ISP interface? The SPIFI connector could also use a reference voltage (GND?).

I'd like to see an I2C bus exposed somewhere, and perhaps an I2S0_RX_SDA signal, so I don't have to steal it from the CPLD interface. The I2S0 will function in "four-wire mode" with only one more pin (RX_SDA), so why not?

Provide a way to inject a supply voltage into the board? Having diodes managing multiple voltage sources would be lossy, so a more expensive solution would be necessary on the Jawbreaker board, adding cost.

If an LPC43xx package with a higher pin-count is used, it would be stellar to expose the LCD interface and quadrature encoder peripheral pins.

The RTC would be handy for stand-alone use. This would require a crystal (32.768kHz) between RTCX1 and RTCX2, and exposing VBAT to a shield for battery backup (disconnecting it from VCC) or providing a coin cell footprint on the HackRF PCB.

Coalesce separate headers into fewer, larger banks of headers, to reduce the number of unique, small header receptacles required for mating? Reducing the header count will also increase the amount of board space around the perimeter of a shield for components and connectors.