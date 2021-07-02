================================================
Opera Cake
================================================

Opera Cake is an antenna switching add on board for HackRF.



Using Opera Cake
~~~~~~~~~~~~~~~~

Board Address
^^^^^^^^^^^^^

As communication with Opera Cake is based on I2C, each board has an address. The default address is ``24``, but this can be changed by setting jumpers on P1. To list the address of one or more opera cake boards:

.. code-block :: sh

	hackrf_operacake -l



Ports
^^^^^

Opera cake has two input ports, PA0 and PB0, which can be switched to any of the eight output ports, PA1-PA4 and PB1-PB4. Each input is always connected to an output, PA0 is connected to PA1 by default, and PB0 to PB1. It is not possible to connect both inputs to ports in the same bank.

When configuring the opera cake board you must always specify the connection for both ports. For example:

.. code-block :: sh

	hackrf_operacake -o 24 -a 3 -b 4

will connect PA0 to PA4 and PB0 to PB1.

.. code-block :: sh

	hackrf_operacake -o 24 -a 5 -b 2

will connect PA0 to PB2 and PB0 to PA3.

.. code-block :: sh

	hackrf_operacake -o 24 -a 1 -b 2

is invalid as it attempts to connect both inputs to bank A.

The available ports are:

.. list-table :: 
  :header-rows: 1
  :widths: 1 1 

  * - Port 	
    - Number
  * - PA1 	
    - 0
  * - PA2 	
    - 1
  * - PA3 	
    - 2
  * - PA4 	
    - 3
  * - PB1 	
    - 4
  * - PB2 	
    - 5
  * - PB3 	
    - 6
  * - PB4 	
    - 7



Opera Glasses
~~~~~~~~~~~~~

As no other software is opera cake aware, it is possible to pre-configure HackRF to support frequency bands and have opera cake automatically switch antenna when the radio retunes. The bands are specified in priority order, the final band specified will be used for frequencies not covered by the other bands specified.

To configure ports to frequency bands you must use the ``-f min:max:port`` argument for each band, with the frequencies specified in MHz. For example:

.. code-block :: sh

	hackrf_operacake -f 100:600:0 -f 600:1200:2 -f 0:4000:5

will use PA1 for 100MHz to 600MHz, PA3 for 600MHz to 1200MHz, and PB2 for 0MHz to 4GHz. If tuning to precisely 600MHz PA1 will be used as it is listed first. Tuning to anything over 4GHz will use PB2 as it is the last listed, and therefore the default antenna.