================================================
LPC43xx Debugging
================================================

Various debugger options for the LPC43xx exist.

Black Magic Probe
~~~~~~~~~~~~~~~~~

`https://github.com/blacksphere/blackmagic <https://github.com/blacksphere/blackmagic>`__

An example of using gdb with the Black Magic Probe:

.. code-block :: sh

	arm-none-eabi-gdb -n blinky.elf
	target extended-remote /dev/ttyACM0
	monitor swdp_scan
	attach 1
	set {int}0x40043100 = 0x10000000
	load
	cont

It is possible to attach to the M0 instead of the M4 if you use jtag_scan instead of swdp_scan, but the Black Magic Probe had some bugs when trying to work with the M0 the last time I tried it.  



LPC-Link
~~~~~~~~

(included with LPCXpresso boards)

TitanMKD has had some success. See the tutorial in hackrf/doc/LPCXPresso_Flash_Debug_Tutorial.pdf or .odt (PDF and OpenOffice document) Doc Link [https://github.com/mossmann/hackrf/tree/master/doc)



ST-LINK/V2
~~~~~~~~~~

Hardware Configuration
^^^^^^^^^^^^^^^^^^^^^^

Start with an STM32F4-Discovery board. Remove the jumpers from CN3. Connect the target's SWD interface to CN2 "SWD" connector.



Software Configuration
^^^^^^^^^^^^^^^^^^^^^^

I'm using libusb-1.0.9.

Install OpenOCD-0.6.0 dev
+++++++++++++++++++++++++

.. code-block:: sh

	# Cloned at hash a21affa42906f55311ec047782a427fcbcb98994
	git clone git://openocd.git.sourceforge.net/gitroot/openocd/openocd
	cd openocd
	./bootstrap
	./configure --enable-stlink --enable-buspirate --enable-jlink --enable-maintainer-mode
	make
	sudo make install



OpenOCD configuration files
+++++++++++++++++++++++++++

openocd.cfg

.. code-block:: sh

	#debug_level 3
	source [find interface/stlink-v2.cfg]
	source ./lpc4350.cfg

lpc4350.cfg

.. code-block :: sh

	set _CHIPNAME lpc4350
	set _M0_CPUTAPID 0x4ba00477
	set _M4_SWDTAPID 0x2ba01477
	set _M0_TAPID 0x0BA01477
	set _TRANSPORT stlink_swd

	transport select $_TRANSPORT

	stlink newtap $_CHIPNAME m4 -expected-id $_M4_SWDTAPID
	stlink newtap $_CHIPNAME m0 -expected-id $_M0_TAPID

	target create $_CHIPNAME.m4 stm32_stlink -chain-position $_CHIPNAME.m4
	#target create $_CHIPNAME.m0 stm32_stlink -chain-position $_CHIPNAME.m0

target.xml, nabbed from an OpenOCD mailing list thread, to fix a communication problem between GDB and newer OpenOCD builds.

.. code-block:: sh

	<?xml version="1.0"?>
	<!DOCTYPE target SYSTEM "gdb-target.dtd">
	<target>
	  <feature name="org.gnu.gdb.arm.core">
	    <reg name="r0" bitsize="32" type="uint32"/>
	    <reg name="r1" bitsize="32" type="uint32"/>
	    <reg name="r2" bitsize="32" type="uint32"/>
	    <reg name="r3" bitsize="32" type="uint32"/>
	    <reg name="r4" bitsize="32" type="uint32"/>
	    <reg name="r5" bitsize="32" type="uint32"/>
	    <reg name="r6" bitsize="32" type="uint32"/>
	    <reg name="r7" bitsize="32" type="uint32"/>
	    <reg name="r8" bitsize="32" type="uint32"/>
	    <reg name="r9" bitsize="32" type="uint32"/>
	    <reg name="r10" bitsize="32" type="uint32"/>
	    <reg name="r11" bitsize="32" type="uint32"/>
	    <reg name="r12" bitsize="32" type="uint32"/>
	    <reg name="sp" bitsize="32" type="data_ptr"/>
	    <reg name="lr" bitsize="32"/>
	    <reg name="pc" bitsize="32" type="code_ptr"/>
	    <reg name="cpsr" bitsize="32" regnum="25"/>
	  </feature>
	  <feature name="org.gnu.gdb.arm.fpa">
	    <reg name="f0" bitsize="96" type="arm_fpa_ext" regnum="16"/>
	    <reg name="f1" bitsize="96" type="arm_fpa_ext"/>
	    <reg name="f2" bitsize="96" type="arm_fpa_ext"/>
	    <reg name="f3" bitsize="96" type="arm_fpa_ext"/>
	    <reg name="f4" bitsize="96" type="arm_fpa_ext"/>
	    <reg name="f5" bitsize="96" type="arm_fpa_ext"/>
	    <reg name="f6" bitsize="96" type="arm_fpa_ext"/>
	    <reg name="f7" bitsize="96" type="arm_fpa_ext"/>
	    <reg name="fps" bitsize="32"/>
	  </feature>
	</target>


Run ARM GDB
~~~~~~~~~~~

Soon, I should dump this stuff into a .gdbinit file.

.. code-block:: sh

	arm-none-eabi-gdb -n
	target extended-remote localhost:3333
	set tdesc filename target.xml
	monitor reset init
	monitor mww 0x40043100 0x10000000
	monitor mdw 0x40043100   # Verify 0x0 shadow register is set properly.
	file lpc4350-test.axf    # This is an ELF file.
	load                     # Place image into RAM.
	monitor reset init
	break main               # Set a breakpoint.
	continue                 # Run to breakpoint.
	continue                 # To continue from the breakpoint.
	step                     # Step-execute the next source line.
	stepi                    # Step-execute the next processor instruction.
	info reg                 # Show processor registers.

More GDB tips for the GDB-unfamiliar:

.. code-block:: sh

	# Write the variable "buffer" (an array) to file "buffer.u8".
	dump binary value buffer.u8 buffer

	# Display the first 32 values in buffer whenever you halt
	# execution.
	display/32xh buffer

	# Print the contents of a range of registers (in this case the
	# CGU peripheral, starting at 0x40050014, for 46 words):
	x/46 0x40050014

And still more, for debugging ARM Cortex-M4 Hard Faults:

.. code-block :: sh

	# Assuming you have a hard-fault handler wired in:
	display/8xw args

	# Examine fault-related registers:

	# Configurable Fault Status Register (CFSR) contains:
	# CFSR[15:8]: BusFault Status Register (BFSR)
	#   "Shows the status of bus errors resulting from instruction
	#   prefetches and data accesses."
	#   BFSR[7]: BFARVALID: BFSR contents valid.
	#   BFSR[5]: LSPERR: fault during FP lazy state preservation.
	#   BFSR[4]: STKERR: derived bus fault on exception entry.
	#   BFSR[3]: UNSTKERR: derived bus fault on exception return.
	#   BFSR[2]: IMPRECISERR: imprecise data access error.
	#   BFSR[1]: PRECISERR: precise data access error, faulting
	#            address in BFAR.
	#   BFSR[0]: IBUSERR: bus fault on instruction prefetch. Occurs
	#            only if instruction is issued.
	display/xw 0xE000ED28

	# BusFault Address Register (BFAR)
	# "Shows the address associated with a precise data access fault."
	# "This is the location addressed by an attempted data access that
	# was faulted. The BFSR shows the reason for the fault and whether
	# BFAR_ADDRESS is valid..."
	# "For unaligned access faults, the value returned is the address
	# requested by the instruction. This might not be the address that
	# faulted."
	display/xw 0xE000ED38
