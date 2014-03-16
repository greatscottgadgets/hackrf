This repository contains host software (Linux/Windows) for HackRF, a project to
produce a low cost, open source software radio platform.

##How to build host software on Windows:

###Prerequisites for cygwin or mingw:

* cmake-2.8.12.1 or more see http://www.cmake.org/cmake/resources/software.html
* libusbx-1.0.18 or more see http://sourceforge.net/projects/libusbx/files/latest/download?source=files
* Install Windows driver for HackRF hardware or use Zadig see http://sourceforge.net/projects/libwdi/files/zadig
  - If you want to use Zadig  select HackRF USB device and just install/replace it with WinUSB driver.

>**Note for Windows build:**
 You shall always execute hackrf-tools from Windows command shell and not from Cygwin or Mingw shell because on Cygwin/Mingw
 Ctrl C is not managed correctly and especially for hackrf_transfer the Ctrl C(abort) will not stop correctly and will corrupt the file.

###For Cygwin:

`cd host`

`mkdir build`

`cd build`

`cmake ../ -G "Unix Makefiles" -DCMAKE_LEGACY_CYGWIN_WIN32=1 -DLIBUSB_INCLUDE_DIR=/usr/local/include/libusb-1.0/`

`make`

`make install`


###For MinGW:

`cd host`

`mkdir build`

`cd build`

Normal version:

* 
`cmake ../ -G "MSYS Makefiles" -DLIBUSB_INCLUDE_DIR=/usr/local/include/libusb-1.0/`

Debug version:

* 
`cmake ../ -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Debug -DLIBUSB_INCLUDE_DIR=/usr/local/include/libusb-1.0/`

`make`

`make install`


##How to build the host software on Linux:

###Prerequisites for Linux (Debian/Ubuntu):


`sudo apt-get install build-essential cmake libusb-1.0-0-dev`


###Build host software on Linux:

`cd host`

`mkdir build`

`cd build`

`cmake ../ -DINSTALL_UDEV_RULES=ON`

`make`

`sudo make install`

`sudo ldconfig`

##Clean CMake temporary files/dirs:

`cd host/build`

`rm -rf *`


principal author: Michael Ossmann <mike@ossmann.com>

http://greatscottgadgets.com/hackrf/
