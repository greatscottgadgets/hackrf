# HTime

HTime is a firmware extension for the HackRF One, the open-hardware/open-source
SDR by [Great Scott Gadgets](https://www.greatscottgadgets.com/hackrf/one/).
HTime provides the HackRF SDR with the
capability to perform timed commands and a precise phase synchronization
of internal clocks (sampling, MCU, output) for the best timing performance.

## Features

* Unix-like time scale, with a resolution of 5 ns
* RX/TX sampling synchronized with the time scale
* Fine time scale/frequency adjust for synchronization with remote reference radio signals with a resolution of about 0.25 Hz @ 10 MHz
* No external hardware required
* Host time tool for access to time extension API

## Documentation

HTime documentation HTML pages are on
[Read the Docs](https://htime.readthedocs.io/en/latest).

All other documentation about the HackRF can be found
[here](https://hackrf.readthedocs.io/en/latest/).

## License

HTime © 2025 documentation by Fabrizio Pollastri is licensed under CC BY-SA 4.0 

HTime © 2025 software by Fabrizio Pollastri is licensed under GNU GPL v2.0

