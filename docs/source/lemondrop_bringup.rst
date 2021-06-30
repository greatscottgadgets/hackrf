================================================
Lemondrop Bring Up
================================================

Board draws approximately 24mA from +3V3 when power is applied. This seems a bit high, but may be expected if not all parts are capable of low-power mode, or aren't configured for low power at power-on. I need to review the schematic and datasheets and see what can be done.

When I put my finger on the MAX2837, current consumption goes up. This suggests there may be floating nodes in that region of the circuit.


Si5351 I2C
~~~~~~~~~~

Attached crystal is 25MHz. For now, I'm assuming 10pF "internal load capacitance" is good enough to get the crystal oscillating. The crystal datasheet should be reviewed and measurements made...

Be sure to reference Silicon Labs application note 619 (AN619). The datasheet is a terrible mess (typos and lack of some details). AN619 appears to be less of a mess, on the whole. And as a bonus, AN619 has PDF bookmarks for each register.



Connections
^^^^^^^^^^^

    * Bus Pirate GND to P7 pin 1
    * Bus Pirate +3V3 to P7 pin 2 (through multimeter set to 200mA range)
    * Bus Pirate CLK to P7 pin 3
    * Bus Pirate MOSI to P7 pin 5



Bus Pirate I2C Initialization
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: sh

	# set mode
	m
	# I2C mode
	4
	# ~100kHz speed
	3
	# power supplies ON
	W
	# macro 1: 7-bit address search
	(1)
	Searching I2C address space. Found devices at:
	0xC0(0x60 W) 0xC1(0x60 R) 

I2C A0 address configuration pin (not available on QFN20 package) is apparently forced to "0".



Reading registers
^^^^^^^^^^^^^^^^^

.. code-block:: sh

	# Read register 0
	I2C>[0xc0 0[0xc1 r]]
	...
	# Register 0: SYS_INIT=0, LOL_B=0, LOL_A=0, LOS=1, REVID=0
	READ: 0x10
	...
	# Read 16 registers, starting with register 0
	I2C>[0xc0 0[0xc1 r:16]]
	...
	READ: 0x10  ACK 0xF8  ACK 0x03  ACK 0x00  ACK 0x00  ACK 0x00  ACK 0x8F  ACK 0x01  ACK
	      0x00  ACK 0x00  ACK 0x00  ACK 0x00  ACK 0x00  ACK 0x00  ACK 0x90  ACK 0x00
	...
	# Read 16 registers, starting with register 16
	I2C>[0xc0 16[0xc1 r:16]]
	...
	READ: 0x00  ACK 0x00  ACK 0x00  ACK 0x00  ACK 0x00  ACK 0x00  ACK 0x00  ACK 0x00  ACK
	      0x00  ACK 0x00  ACK 0x00  ACK 0x00  ACK 0x00  ACK 0x00  ACK 0x00  ACK 0x00 
	...



Writing registers
^^^^^^^^^^^^^^^^^


Simple XTAL passthrough to CLK0
+++++++++++++++++++++++++++++++

.. code-block:: sh

	# Disable all CLKx outputs.
	[0xC0 3 0xFF]

	# Turn off OEB pin control for all CLKx
	[0xC0 9 0xFF]

	# Power down all CLKx
	[0xC0 16 0x80 0x80 0x80 0x80 0x80 0x80 0x80 0x80]

	# Register 183: Crystal Internal Load Capacitance
	# Reads as 0xE4 on power-up
	# Set to 10pF (until I find out what loading the crystal/PCB likes best)
	[0xC0 183 0xE4]

	# Register 187: Fanout Enable
	# Turn on XO fanout only.
	[0xC0 187 0x40]

	# Register 15: PLL Input Source
	# CLKIN_DIV=0 (Divide by 1)
	# PLLB_SRC=0 (XTAL input)
	# PLLA_SRC=0 (XTAL input)
	[0xC0 15 0x00]

	# Registers 16 through 23: CLKx Control
	# CLK0:
	#   CLK0_PDN=0 (powered up)
	#   MS0_INT=1 (integer mode)
	#   MS0_SRC=0 (PLLA as source for MultiSynth 0)
	#   CLK0_INV=0 (not inverted)
	#   CLK0_SRC=0 (XTAL as clock source for CLK0)
	#   CLK0_IDRV=3 (8mA)
	[0xC0 16 0x43 0x80 0x80 0x80 0x80 0x80 0x80 0x80]

	# Enable CLK0 output only.
	[0xC0 3 0xFE]



Clocking Scheme (Work In Progress)
++++++++++++++++++++++++++++++++++

From AN619: If Fxtal=25MHz, Fvco = Fxtal * (a + (b / c)). If we want Fvco = 800MHz, a = 32, b = 0, c = don't care.

.. code-block:: sh

	MSNA_P1[17:0] = 128 * a + floor(128 * b / c) - 512
	              = 128 * a + floor(0) - 512
	              = 128 * 32 + 0 - 512
	              = 3584 = 0xE00
	MSNA_P1[17:16] (register 28) = 0x00
	MSNA_P1[15: 8] (register 29) = 0x0E
	MSNA_P1[ 7: 0] (register 30) = 0x00
	MSNA_P2[19:0] = 128 * b - c * floor(128 * b / c)
	              = 128 * 0 - 0 * floor(128 * 0 / X)
	              = 0
	MSNA_P3[19:0] = 0

MultiSynth0 should output 40MHz (800MHz VCO divided by 20):

.. code-block:: sh

	a = 20, b = 0, c = X
	MS0_P1[17: 0] = 128 * a + floor(128 * b / c) - 512
	              = 2048 = 0x800
	MS0_P1[17:16] (register 44) = 0x00
	MS0_P1[15: 8] (register 45) = 0x08
	MS0_P1[ 7: 0] (register 46) = 0x00
	MS0_P2[19:0]  = 0
	MS0_P3[19:0]  = 0

MultiSynth1 should output 20MHz (800MHz VCO divided by 40) or some smaller integer fraction of the VCO:

.. code-block:: sh

	a = 40, b = 0, c = X
	MS1_P1[17: 0] = 128 * a + floor(128 * b / c) - 512
	              = 4608 = 0x1200
	MS1_P1[17:16] (register 52) = 0x00
	MS1_P1[15: 8] (register 53) = 0x12
	MS1_P1[ 7: 0] (register 54) = 0x00
	MS1_P2[19:0]  = 0
	MS1_P3[19:0]  = 0

Initialization:

.. code-block:: sh

	# Disable all CLKx outputs.
	[0xC0 3 0xFF]

	# Turn off OEB pin control for all CLKx
	[0xC0 9 0xFF]

	# Power down all CLKx
	[0xC0 16 0x80 0x80 0x80 0x80 0x80 0x80 0x80 0x80]

	# Register 183: Crystal Internal Load Capacitance
	# Reads as 0xE4 on power-up
	# Set to 10pF (until I find out what loading the crystal/PCB likes best)
	[0xC0 183 0xE4]

	# Register 187: Fanout Enable
	# Turn on XO and MultiSynth fanout only.
	[0xC0 187 0x50]

	# Register 15: PLL Input Source
	# CLKIN_DIV=0 (Divide by 1)
	# PLLB_SRC=0 (XTAL input)
	# PLLA_SRC=0 (XTAL input)
	[0xC0 15 0x00]

	# MultiSynth NA (PLL1)
	[0xC0 26 0x00 0x00 0x00 0x0E 0x00 0x00 0x00 0x00]

	# MultiSynth NB (PLL2)
	...

	# MultiSynth 0
	[0xC0 42 0x00 0x00 0x00 0x08 0x00 0x00 0x00 0x00]

	# MultiSynth 1
	[0xC0 50 0x00 0x00 0x00 0x12 0x00 0x00 0x00 0x00]

	# Registers 16 through 23: CLKx Control
	# CLK0:
	#   CLK0_PDN=0 (powered up)
	#   MS0_INT=1 (integer mode)
	#   MS0_SRC=0 (PLLA as source for MultiSynth 0)
	#   CLK0_INV=0 (not inverted)
	#   CLK0_SRC=3 (MS0 as input source)
	#   CLK0_IDRV=3 (8mA)
	# CLK1:
	#   CLK1_PDN=0 (powered up)
	#   MS1_INT=1 (integer mode)
	#   MS1_SRC=0 (PLLA as source for MultiSynth 1)
	#   CLK1_INV=0 (not inverted)
	#   CLK1_SRC=3 (MS1 as input source)
	#   CLK1_IDRV=3 (8mA)
	[0xC0 16 0x4F 0x4F 0x80 0x80 0x80 0x80 0x80 0x80]

	# Enable CLK0 output only.
	[0xC0 3 0xFC]



Si5351 output phase relationships
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Tested CLK4 and CLK5 (integer division only):

With CLK4 set to MS4 and CLK5 set to MS5, even with both multisynths configured identically, there was no consistent phase between the two. Once started, the clocks maintained relative phase with each other, but when stopped and restarted the initial phase offset was unpredictable.

With CLK4 and CLK5 both set to MS4, the phase of both outputs was identical when no output (R) divider was selected. When an output divider was selected on both MS4 and MS5, the relative phase became predictable only within the constraints of the divider (e.g. with R=2 the relative phase was always either 0 or half a cycle, with R=4 the relative phase was always either 0, a quarter, a half, or three quarters of a cycle). With R=1 on MS4 and R=2 on MS5, the two outputs were consistently in phase with each other. The output (R) dividers supposedly tied to the multisynths are actually tied to the outputs.