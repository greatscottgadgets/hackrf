===========================================
Third-Party Software Compatible With HackRF
===========================================

Software That Has Direct Support For HackRF
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* GQRX

    * `http://gqrx.dk/ <http://gqrx.dk/>`__

* GNU Radio

    * https://www.gnuradio.org/

* GNU Radio Mode-S/ADS-B

    * `https://github.com/bistromath/gr-air-modes <https://github.com/bistromath/gr-air-modes>`__

* QSpectrumAnalyzer

    * `https://github.com/xmikos/qspectrumanalyzer <https://github.com/xmikos/qspectrumanalyzer>`__

* SDR# 

    * `https://airspy.com/download/ <https://airspy.com/download/>`__
    * Windows OS only
    * Only nightly builds currently support HackRF One  

* SDR Console

    * https://www.sdr-radio.com/Console

* Spectrum Analyzer GUI for hackrf_sweep for Windows 

    * `https://github.com/pavsa/hackrf-spectrum-analyzer <https://github.com/pavsa/hackrf-spectrum-analyzer>`__

* Universal Radio Hacker (Windows/Linux) 

    * `https://github.com/jopohl/urh <https://github.com/jopohl/urh>`__

* Web-based APRS tracker 

    * `https://xakcop.com/aprs-sdr <https://xakcop.com/aprs-sdr/>`__



Software That Can Use Data From HackRF
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* Baudline 

    * `http://www.baudline.com/ <http://www.baudline.com/>`__ 
    * Can view/process HackRF data, e.g. hackrf_transfer

* Inspectrum 

    * `https://github.com/miek/inspectrum <https://github.com/miek/inspectrum>`__
    * Capture analysis tool with advanced features

* Matlab

    .. code-block :: sh

        fid = open('samples.bin',  'r');
        len = 1000; % 1000 samples
        y = fread(fid, 2*len, 'int8');
        y = y(1:2:end) + 1j*y(2:2:end);
        fclose(fid)



Troubleshooting Recommendations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Many of these tools require libhackrf and at times HackRF Tools. It may help you to have updated libhackrf and HackRF Tools when troubleshooting these applications. 

It is also strongly suggested, and usually required, that your HackRF Tools and HackRF firmware match. 