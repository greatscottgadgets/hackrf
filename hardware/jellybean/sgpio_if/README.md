CPLD interface between LPC43xx microcontroller SGPIO peripheral and MAX5864
RF codec.

Requirements
============

To build this VHDL project and produce an SVF file for flashing the CPLD:

* Xilinx WebPACK 13.4 for Windows or Linux.

To program the SVF file into the CPLD:

* Dangerous Prototypes Bus Blaster v2:
  * Configured with JTAGKey buffers.
  * Connected to CPLD JTAG signals on Jellybean.

* urJTAG built with libftdi support.

* BSDL model files for Xilinx CoolRunner-II XC264A, available at xilinx.com,
  in the "Device Models" Support Resources section of the CoolRunner-II
  Product Support & Documentation page. Only one file from the BSDL package is
  required, and the "program" script below expects it to be at the relative
  path "bsdl/xc2c/xc2c64.bsd".

To Program
==========

./program

...which connects to the Bus Blaster interface 0, sets the BSDL directory,
detects devices on the JTAG chain, and writes the sgpio_if.svf file to the
CPLD.
