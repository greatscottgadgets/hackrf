==================
Modes of Operation
==================

Opera Cake supports three modes of operation: ``manual``, ``frequency``, and ``time``. The operating mode can be set with the ``--mode`` or ``-m`` option, and the active operating mode is displayed with the ``--list`` or ``-l`` option.

Manual Mode
~~~~~~~~~~~

The default mode of operation is ``manual``. In manual mode, fixed port connections are configured with the ``-a`` and ``-b`` options as in the :ref:`port configuration examples <portconfiguration>`. If the operating mode has been changed, it can be changed back to manual mode with:

.. code-block:: sh

	hackrf_operacake -m manual

Frequency Mode
~~~~~~~~~~~~~~

In frequency mode, the A0 port connection switches automatically whenever the HackRF is tuned to a different frequency. This is useful when antennas for different frequency bands are connected to various ports.

The bands are specified in priority order. The final band specified will be used for frequencies not covered by the other bands specified.

To assign frequency bands to ports you must use the ``-f <port:min:max>`` option for each band, with the minimum and maximum frequencies specified in MHz. For example, to use port A1 for 100 MHz to 600 MHz, A3 for 600 MHz to 1200 MHz, and B2 for 0 MHz to 4 GHz:

.. code-block:: sh

	hackrf_operacake -m frequency -f A1:100:600 -f A3:600:1200 -f B2:0:4000

If tuning to precisely 600 MHz, A1 will be used as it is listed first. Tuning to any frequency over 4 GHz will use B2 as it is the last listed and therefore the default port.

Only the A0 port connection is specified in frequency mode. Whenever the A0 connection is switched, the B0 connection is also switched to the secondary port mirroring A0's secondary port. For example, when A0 switches to B2, B0 is switched to A2.

Once configured, an Opera Cake will remain in frequency mode until the mode is reconfigured or until the HackRF One is reset. You can pre-configure the Opera Cake in frequency mode, and the automatic switching will continue to work while using other software.

Although multiple Opera Cakes on a single HackRF One may be set to frequency mode at the same time, they share a single switching plan. This can be useful, for example, for a filter bank consisting of eight filters.

Time Mode
~~~~~~~~~

In time mode, the A0 port connection switches automatically over time, counted in units of the sample period. This is useful for experimentation with pseudo-doppler direction finding.

To cycle through four ports, one port every 1000 samples:

.. code-block:: none

	hackrf_operacake -m time -t A1:1000 -t A2:1000 -t A3:1000 -t A4:1000

When the duration on multiple ports is the same, the ``-w`` option can be used to set the default dwell time:

.. code-block:: none

	hackrf_operacake --mode time -w 1000 -t A1 -t A2 -t A3 -t A4

Only the A0 port connection is specified in time mode. Whenever the A0 connection is switched, the B0 connection is switched to the secondary port mirroring A0's secondary port. For example, when A0 switches to B2, B0 is switched to A2.

Once configured, an Opera Cake will remain in time mode until the mode is reconfigured or until the HackRF One is reset. You can pre-configure the Opera Cake in time mode, and the automatic switching will continue to work while using other software.

Although multiple Opera Cakes on a single HackRF One may be set to time mode at the same time, they share a single switching plan.
