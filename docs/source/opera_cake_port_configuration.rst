===================
Port Configurations
===================

Port connections may be configured manually. For example, to connect A0 to A2 and B0 to B3:

.. code-block:: sh

    hackrf_operacake -a A2 -b B3

To connect A0 to B2 and B0 to A4:

.. code-block:: sh

    hackrf_operacake -a B2 -b A4

If only one primary port is configured, the other primary port will be connected to the first secondary port on the opposite side. For example, after the next two commands B0 will be connected to A1:

.. code-block:: sh

	hackrf_operacake -a A2 -b B3
	hackrf_operacake -a B2
