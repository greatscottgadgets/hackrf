================================================
HackRF Compatible Software
================================================

Software with HackRF Support
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is intended to be a list of software known to work with the HackRF. There are three sections, GNU Radio Based software, those that have direct support, and those that can work with data from the HackRF.



GNU Radio Based
~~~~~~~~~~~~~~~

GNU Radio Mode-S/ADS-B - `https://github.com/bistromath/gr-air-modes <https://github.com/bistromath/gr-air-modes>`__

GQRX - `http://gqrx.dk/ <http://gqrx.dk/>`__



Direct Support
~~~~~~~~~~~~~~

SDR# (Windows only) - `https://airspy.com/download/ <https://airspy.com/download/>`__

    * Only nightly builds currently support HackRF One - `http://sdrsharp.com/downloads/sdr-nightly.zip <http://sdrsharp.com/downloads/sdr-nightly.zip>`__

SDR_Radio.com V2 - `http://v2.sdr-radio.com/Radios/HackRF.aspx <http://v2.sdr-radio.com/Radios/HackRF.aspx>`__

Universal Radio Hacker (Windows/Linux) - `https://github.com/jopohl/urh <https://github.com/jopohl/urh>`__

QSpectrumAnalyzer - `https://github.com/xmikos/qspectrumanalyzer <https://github.com/xmikos/qspectrumanalyzer>`__

Spectrum Analyzer GUI for hackrf_sweep for Windows - `https://github.com/pavsa/hackrf-spectrum-analyzer <https://github.com/pavsa/hackrf-spectrum-analyzer>`__

Web-based APRS tracker `https://xakcop.com/aprs-sdr <https://xakcop.com/aprs-sdr/>`__


Can use HackRF data
~~~~~~~~~~~~~~~~~~~

Inspectrum `https://github.com/miek/inspectrum <https://github.com/miek/inspectrum>`__

    * Capture analysis tool with advanced features

Baudline `http://www.baudline.com/ <http://www.baudline.com/>`__ (Can view/process HackRF data, e.g. hackrf_transfer)



HackRF Tools
~~~~~~~~~~~~

In addition to third party tools that support HackRF, we provide some commandline tools for interacting with HackRF. For information on how to use each tool look at the help information provided (e.g. ``hackrf_transfer -h``) or the `manual pages <http://manpages.ubuntu.com/manpages/utopic/man1/hackrf_info.1.html>`__.

    * **hackrf_info** Read device information from HackRF such as serial number and firmware version.

    * **hackrf_transfer** Send and receive signals using HackRF. Input/output files are 8-bit signed quadrature samples.

    * **hackrf_sweep**, a command-line spectrum analyzer.

    * **hackrf_clock** Read and write clock input and output configuration.

    * **hackrf_operacake** Configure Opera Cake antenna switch connected to HackRF.

    * **hackrf_spiflash** A tool to write new firmware to HackRF. See: :ref:`Updating Firmware <updating_firmware>`.

    * **hackrf_debug** Read and write registers and other low-level configuration for debugging.


Handling HackRF data
~~~~~~~~~~~~~~~~~~~~

Matlab
^^^^^^

.. code-block :: sh

	fid = open('samples.bin',  'r');
	len = 1000; % 1000 samples
	y = fread(fid, 2*len, 'int8');
	y = y(1:2:end) + 1j*y(2:2:end);
	fclose(fid)
