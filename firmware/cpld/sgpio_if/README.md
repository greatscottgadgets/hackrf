CPLD interface between LPC43xx microcontroller SGPIO peripheral and MAX5864
RF codec.

Requirements
============

To build this VHDL project and produce an SVF file for flashing the CPLD:

* Xilinx WebPACK 13.4 for Windows or Linux.

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

$ hackrf_cpldjtag -x default.xsvf
