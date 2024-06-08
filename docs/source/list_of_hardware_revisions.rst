================================================
Hardware Revisions
================================================

HackRF One hardware revisions exist mainly to deal with changes in component availability. Each of the revisions meet the same performance specifications that are measured in the factory. 


List of Hardware Revisions
~~~~~~~~~~~~~~~~~~~~~~~~~~

HackRF One r1–r4
^^^^^^^^^^^^^^^^

The first revision of HackRF One shipped by Great Scott Gadgets starting in 2014 was labeled r1. Subsequent manufacturing runs incremented the revision number up to r4 without modification to the hardware design. Manufacturing years: 2014–2020

HackRF One r5
^^^^^^^^^^^^^

This experimental revision has not been manufactured.

HackRF One r6
^^^^^^^^^^^^^

SKY13350 RF switches were replaced by SKY13453. Although the SKY13453 uses simplified control logic, it did not require a firmware modification. Hardware revision detection pin straps were added. Manufacturing year: 2020

HackRF One r7
^^^^^^^^^^^^^

SKY13453 RF switches were reverted to SKY13350. USB VBUS detection resistor values were updated. Manufacturing year: 2021

HackRF One r8
^^^^^^^^^^^^^

SKY13350 RF switches were replaced by SKY13453. Manufacturing years: 2021–2022

HackRF One r9
^^^^^^^^^^^^^

MAX2837 was replaced by MAX2839. Si5351C was replaced by Si5351A with additional clock distribution. A series diode was added to the antenna port power supply. Manufacturing year: 2023

HackRF One r10
^^^^^^^^^^^^^^

This revision is based on r8, reverting most of the changes made in r9. A series diode was added to the antenna port power supply. Manufacturing year: 2024

Hardware Revision Identification
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

HackRF Ones manufactured by Great Scott Gadgets have the revision number printed on the PCB top silkscreen layer near the MAX5864 (U18).

Starting with HackRF One r6, hardware revisions are detected by firmware and reported by ``hackrf_info``.
