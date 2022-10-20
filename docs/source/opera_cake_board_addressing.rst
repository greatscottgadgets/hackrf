================
Board Addressing
================

Each Opera Cake has a numeric address set by optional jumpers installed on header P1. The default address (without jumpers) is 0. The ``--list`` or ``-l`` option can be used to list the address(es) of one or more Opera Cakes installed on a HackRF One:

.. code-block:: sh

	hackrf_operacake -l

The address may be set to any number from 0 to 7 by installing jumpers across the A0, A1, and/or A2 pins of header P1.

.. list-table::
  :header-rows: 1
  :widths: 1 1 1 1

  * - Address
    - A2 Jumper
    - A1 Jumper
    - A0 Jumper
  * - 0
    - No
    - No
    - No
  * - 1
    - No
    - No
    - Yes
  * - 2
    - No
    - Yes
    - No
  * - 3
    - No
    - Yes
    - Yes
  * - 4
    - Yes
    - No
    - No
  * - 5
    - Yes
    - No
    - Yes
  * - 6
    - Yes
    - Yes
    - No
  * - 7
    - Yes
    - Yes
    - Yes

When configuring an Opera Cake, the address may be specified with the ``--address`` or ``-o`` option:

.. code-block:: sh

	hackrf_operacake -o 1 -a A1 -b B2

If the address is unspecified, 0 is assumed. It is only necessary to specify the address if the address has been changed with the addition of jumpers, typically required only if multiple Opera Cakes are stacked onto a single HackRF One.
