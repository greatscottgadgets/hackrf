.. _synchronization_checklist:

================================================
Synchronization Checklist
================================================

There are many scenarios where you may want to use multiple HackRF One devices
synchronized with each other. For instance, multiple devices can be used with a
phased antenna array to implement direction finding or beamforming.

If you're having trouble achieving fully synchronized operation, the following
checklist may be useful for troubleshooting:

* **Are you running the latest firmware and host software versions?**

    There have been many bug fixes, so please use the latest releases.

* **Are you applying settings to the right target devices?**

    With more than one HackRF device connected, you need to take care to specify
    which device should be used where. When using our command line tools, use the
    ``-d`` option with a serial number on the command line to specify which
    device each command should target.

* **Are all HackRFs sharing a clock?**

    If the HackRFs are not sharing a clock, frequencies and sample rates will
    not match exactly. See the
    :ref:`external clock interface <external_clock_interface>`
    section for how to connect the clock signals.

* **Is the clock input being detected?**

    Use ``hackrf_clock`` with the ``-i`` option to check for clock input.
    Use the ``-d`` option to specify the serial number.

    This requires ``2022.09.1`` or later host software.

    The older way of checking using ``hackrf_debug`` will not work correctly on
    some hardware revisions.

* **If the clock source is CLKOUT of another HackRF, has it been enabled?**

    Use ``hackrf_clock`` with the ``-o`` option to enable the clock output.
    Use the ``-d`` option to specify the serial number.

* **Is the CLKIN waveform correct?**

    * It should be a 10 MHz square wave between 0 V and 3.0 to 3.3 V.
    * A sine wave is OK, but may give greater phase noise.
    * An unbuffered TCXO output may not have sufficient voltage.
    * Some hardware revisions are less tolerant of out-of-spec
      clock input than others.

* **Is your hardware faulty?**

    Some HackRF clones were sold with a non-functional CLKIN port.

* **Are all HackRFs being started together using hardware triggering?**

    If hardware triggering is not used, start times will not match exactly.
    See the :ref:`hardware triggering <hardware_triggering>` section for how
    to connect the necessary trigger signals.

    Use ``hackrf_transfer`` with the ``-H`` option to wait for trigger input.

    If launching multiple ``hackrf_transfer`` instances, make sure that those
    running with ``-H`` have had time to reach the ``Waiting for trigger...``
    state before the trigger signal is sent.

    .. note:: There is currently no support for hardware triggering via the
              Osmocom or Soapy blocks in GNU Radio. The Osmocom block may look
              like it supports this but the options have no effect. 

* **Are any samples being lost due to USB throughput problems?**

    If RX samples are dropped or TX samples delayed, signals will become out
    of sync.

    Use ``hackrf_debug -S`` on each HackRF after running your software to check
    if there were throughput problems. If the shortfall count reported is
    non-zero, some samples were dropped on RX or late for TX, and signals will
    be out of sync as a result.

    This requires ``2022.09.1`` or later host software.

    To force shortfalls to stop the device at runtime, use
    ``hackrf_debug -T 0 -R 0`` to set zero tolerance for shortfalls.

* **Are your HackRFs sharing a USB bus?**

    A single HackRF One at 20 MHz sample rate uses practically all the bandwidth
    of a single USB 2.0 bus. Unless using very low sample rates, each HackRF
    should be connected to its own bus.
