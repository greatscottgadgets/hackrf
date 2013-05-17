This repository contains hardware designs and software for HackRF, a project to
produce a low cost, open source software radio platform.

![Jawbreaker](https://raw.github.com/mossmann/hackrf/master/doc/jawbreaker.jpeg)

How to build host software on Windows:
prerequisite for cygwin or mingw:
* cmake-2.8.10.2 or more see http://www.cmake.org/cmake/resources/software.html
* libusbx-1.0.14 or more see http://sourceforge.net/projects/libusbx/files/latest/download?source=files
* Install Windows driver for HackRF hardware or use Zadig see http://sourceforge.net/projects/libwdi/files/zadig
  - If you want to use Zadig  select HackRF USB device and just install/replace it with WinUSB driver.
* Build libhackrf before to build this library, see host/libhackrf/Readme.md.
  
For Cygwin:
cmake -G "Unix Makefiles" -DCMAKE_LEGACY_CYGWIN_WIN32=1 -DLIBUSB_INCLUDE_DIR=/usr/local/include/libusb-1.0/
make
make install

For Mingw:
#normal version
cmake -G "MSYS Makefiles" -DLIBUSB_INCLUDE_DIR=/usr/local/include/libusb-1.0/
#debug version
cmake -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Debug -DLIBUSB_INCLUDE_DIR=/usr/local/include/libusb-1.0/
make
make install

How to build host software on Linux:
cmake ./
make
make install

principal author: Michael Ossmann <mike@ossmann.com>

http://greatscottgadgets.com/hackrf/
