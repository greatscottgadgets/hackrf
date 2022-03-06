## Python wrapper and examples to libhackrf ##

pyhackrf.py provides a python wrapper to libhackrf allowing direct tx, rx usage from python code. Not all API from libhackrf are implemented, but most usefull one allowing direct RX, TX and settings.  Multiple boards is supported providing argument to `open` (either a string with board serial number or int as board index).  

`example1.py` provides a minimal example displaying board and lib info in the same fashion as `hackrf_info`

`example2.py` provides a minimal example for the callback mechanism without actually sending or receiving any data. Callback will run as long as it return 0 and stop as soon as returning 1

`example3.py` provides a minimal TX example with the callback mecanism

`example4.py` provides a minimal RX example with the callback mecanism
