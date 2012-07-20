
CPLD interface to expose LPC43xx microcontroller SGPIO peripheral, either
as all inputs or all outputs.

Requirements
============

To build this VHDL project and produce an SVF file for flashing the CPLD:

* Xilinx WebPACK 13.4 for Windows or Linux.

* BSDL model files for Xilinx CoolRunner-II XC264A, available at xilinx.com,
  in the "Device Models" Support Resources section of the CoolRunner-II
  Product Support & Documentation page. Only one file from the BSDL package is
  required, and the "program" script below expects it to be at the relative
  path "bsdl/xc2c/xc2c64.bsd".

To program the SVF file into the CPLD:

* Dangerous Prototypes Bus Blaster v2:
  * Configured with JTAGKey buffers.
  * Connected to CPLD JTAG signals on Jellybean.

* urJTAG built with libftdi support.

To Program
==========

./program

...which connects to the Bus Blaster interface 0, sets the BSDL directory,
detects devices on the JTAG chain, and writes the sgpio_if_passthrough.svf
file to the CPLD.

Usage:
B1AUX9=0 control SGPIO0 to 15 as Input. B2AUX[1-16] => SGPIO[0-15]
B1AUX9=1 control SGPIO0 to 15 as Output. SGPIO[0-15] => B2AUX[1-16]
