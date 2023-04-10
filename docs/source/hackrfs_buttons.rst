=======
Buttons
=======

The **RESET button** resets the microcontroller. This is a reboot that should result in a USB re-enumeration.

The **DFU button** invokes a USB DFU bootloader located in the microcontroller's ROM. This bootloader makes it possible to unbrick a HackRF One with damaged firmware because the ROM cannot be overwritten.

The DFU button only invokes the bootloader during reset. This means that it can be used for other functions by custom firmware.

To invoke DFU mode: Press and hold the DFU button. While holding the DFU button, reset the HackRF One either by pressing and releasing the RESET button or by powering on the HackRF One. Release the DFU button.
