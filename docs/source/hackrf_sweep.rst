================================================
hackrf_sweep
================================================

Usage
~~~~~

.. code-block:: sh 

    [-h] # this help
    [-d serial_number] # Serial number of desired HackRF
    [-a amp_enable] # RX RF amplifier 1=Enable, 0=Disable
    [-f freq_min:freq_max] # minimum and maximum frequencies in MHz
    [-p antenna_enable] # Antenna port power, 1=Enable, 0=Disable
    [-l gain_db] # RX LNA (IF) gain, 0-40dB, 8dB steps
    [-g gain_db] # RX VGA (baseband) gain, 0-62dB, 2dB steps
    [-n num_samples] # Number of samples per frequency, 8192-4294967296
    [-w bin_width] # FFT bin width (frequency resolution) in Hz
    [-1] # one shot mode
    [-B] # binary output
    [-I] # binary inverse FFT output
    -r filename # output file



Output fields
~~~~~~~~~~~~~

``date, time, hz_low, hz_high, hz_bin_width, num_samples, dB, dB, ...``

Running ``hackrf_sweep -f 2400:2490`` gives the following example results:

.. list-table :: 
  :header-rows: 1
  :widths: 1 1 1 1 1 1 1 1 1 1 1

  * - Date 	
    - Time 	
    - Hz Low 	
    - Hz High 	
    - Hz bin width 	
    - Num Samples 	
    - dB 	
    - dB 	
    - dB 	
    - dB 	
    - dB
  * - 2019-01-03 	
    - 11:57:34.967805 	
    - 2400000000 	
    - 2405000000 	
    - 1000000.00 	
    - 20 	
    - -64.72 	
    - -63.36 	
    - -60.91 	
    - -61.74 	
    - -58.58
  * - 2019-01-03 	
    - 11:57:34.967805 	
    - 2410000000 	
    - 2415000000 	
    - 1000000.00 	
    - 20 	
    - -69.22 	
    - -60.67 	
    - -59.50 	
    - -61.81 	
    - -58.16
  * - 2019-01-03 	
    - 11:57:34.967805 	
    - 2405000000 	
    - 2410000000 	
    - 1000000.00 	
    - 20 	
    - -61.19 	
    - -70.14 	
    - -60.10 	
    - -57.91 	
    - -61.97
  * - 2019-01-03 	
    - 11:57:34.967805 	
    - 2415000000 	
    - 2420000000 	
    - 1000000.00 	
    - 20 	
    - -72.93 	
    - -79.14 	
    - -68.79 	
    - -70.71 	
    - -82.78
  * - 2019-01-03 	
    - 11:57:34.967805 	
    - 2420000000 	
    - 2425000000 	
    - 1000000.00 	
    - 20 	
    - -67.57 	
    - -61.61 	
    - -57.29 	
    - -61.90 	
    - -70.19
  * - 2019-01-03 	
    - 11:57:34.967805 	
    - 2430000000 	
    - 2435000000 	
    - 1000000.00 	
    - 20 	
    - -56.04 	
    - -59.58 	
    - -66.24 	
    - -66.02 	
    - -62.12

Two ranges of 5 MHz are analyzed at once from the same set of samples, so a single timestamp applies to the whole range.

The fifth column tells you the width in Hz (1 MHz in this case) of each frequency bin, which you can set with ``-w``. The sixth column is the number of samples analyzed to produce that row of data.

Each of the remaining columns shows the power detected in each of several frequency bins. In this case there are five bins, the first from 2400 to 2401 MHz, the second from 2401 to 2402 MHz, and so forth.