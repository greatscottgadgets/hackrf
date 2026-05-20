# HTime

HTime is a firmware extension for the HackRF One, the open-hardware/open-source
SDR by [Great Scott Gadgets](https://www.greatscottgadgets.com/hackrf/one/).
HTime provides the HackRF SDR with the
capability to perform timed commands and a precise phase synchronization
of internal clocks (sampling, MCU, output) for the best timing performance.
HTime is developed by Fabrizio Pollastri.

## Features

* Unix-like time scale, with a resolution of 5 ns
* RX/TX sampling synchronized with the time scale
* Fine time scale/frequency adjust for synchronization with remote reference
  radio signals with a resolution of about 0.25 Hz @ 10 MHz
* No external hardware required
* Host time tool for access to time extension API

For now, HTime works only with a 10 MHz sample rate.

## Build and Install

HTime is developed into a fork of the original HackRF repository, so all
build and install rules for the original HackRF software apply also to
HTime. Please, see the documentation at
<https://github.com/greatscottgadgets/hackrf>

## Documentation

* [HTime — Working Principle](docs_htime/htime.md)
* [HTime USB API](docs_htime/htime_api.md)
* [HTime Tool](docs_htime/htime_tool.md)

All other documentation about the HackRF can be found
[here](https://hackrf.readthedocs.io/en/latest/).

## License

HTime © 2025 documentation by Fabrizio Pollastri is licensed under CC BY-SA 4.0

HTime © 2025 software by Fabrizio Pollastri is licensed under GNU GPL v2.0


