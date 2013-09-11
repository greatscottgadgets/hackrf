CPLD interface between LPC43xx microcontroller SGPIO peripheral and MAX5864
RF codec.

Requirements
============

To build this VHDL project and produce an SVF file for flashing the CPLD:

* Xilinx WebPACK 13.4 for Windows or Linux.

To program the SVF file into the CPLD:

* Dangerous Prototypes Bus Blaster v2:
  * Configured with [JTAGKey buffers](http://dangerousprototypes.com/docs/Bus_Blaster_v2_buffer_logic).
  * Connected to CPLD JTAG signals on Jellybean.

* urJTAG built with libftdi support.

* BSDL model files for Xilinx CoolRunner-II XC264A, available at xilinx.com,
  in the "Device Models" Support Resources section of the CoolRunner-II
  Product Support & Documentation page. Only one file from the BSDL package is
  required, and the "program" script below expects it to be at the relative
  path "bsdl/xc2c/xc2c64.bsd".

Generate an XSVF
================

After generating a programming file:

* In the ISE Project Navigator, "Processes: top - Behavioral" pane, double-click "Configure Target Device".
* Click "OK" to open iMPACT.
* Ctrl-N to create a "New Project".
* "Yes" to automatically create and save a project file.
* Select "Prepare a Boundary-Scan File", choose "XSVF".
* Select file name "default.xsvf".
* Click "OK" to start adding devices.
* Assign new configuration file: "top.jed".
* Right-click the "xc2c64a top.jed" icon and select "Erase". Accept defaults.
* Right-click the "xc2c64a top.jed" icon and select "Program".
* Right-click the "xc2c64a top.jed" icon and select "Verify".
* Choose menu "Output" -> "XSVF File" -> "Stop Writing to XSVF File".
* Close iMPACT.

To Program
==========

./program

...which connects to the Bus Blaster interface 0, sets the BSDL directory,
detects devices on the JTAG chain, and writes the sgpio_if.svf file to the
CPLD.
