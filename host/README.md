This repository contains host software (Linux/Windows) for HackRF, a project to
produce a low cost, open source software radio platform.

## How to build the host software on Linux:

### Prerequisites for Linux (Debian/Ubuntu):
`sudo apt-get install build-essential cmake libusb-1.0-0-dev pkg-config libfftw3-dev`

### Build host software on Linux:
```
mkdir host/build
cd host/build
cmake ..
make
sudo make install
sudo ldconfig
```

By default this will attempt to install an udev rule to `/etc/udev/rules.d` to
provide the `usb` or `plugdev` group access to HackRF. If your setup requires
the udev rule to be installed elsewhere you can modify the path with
`-DUDEV_RULES_PATH=/path/to/udev`.

Note: The udev rule is not installed by default for PyBOMBS installs as
they do not ususally get installed with root privileges.

## Clean CMake temporary files/dirs:
```
cd host/build
rm -rf *
```

## How to build host software on Windows:
### Prerequisites for Cygwin, MinGW, or Visual Studio:

* cmake-2.8.12.1 or later from http://www.cmake.org/cmake/resources/software.html
* libusbx-1.0.18 or later from http://sourceforge.net/projects/libusbx/files/latest/download?source=files
* fftw-3.3.5 or later from http://www.fftw.org/install/windows.html
* Install Windows driver for HackRF hardware or use Zadig see http://sourceforge.net/projects/libwdi/files/zadig
  - If you want to use Zadig select HackRF USB device and just install/replace it with WinUSB driver.

>**Note for Windows build:**
 You shall always execute hackrf-tools from Windows command shell and not from Cygwin or MinGW shell because on Cygwin/MinGW
 Ctrl C is not managed correctly and especially for hackrf_transfer the Ctrl C(abort) will not stop correctly and will corrupt the file.

### For Cygwin:
```
mkdir host/build
cd host/build
cmake ../ -G "Unix Makefiles" -DCMAKE_LEGACY_CYGWIN_WIN32=1 -DLIBUSB_INCLUDE_DIR=/usr/local/include/libusb-1.0/
make
make install
```

### For MinGW:
```
mkdir host/build
cd host/build
cmake ../ -G "MSYS Makefiles" -DLIBUSB_INCLUDE_DIR=/usr/local/include/libusb-1.0/
make
make install
```

### For Visual Studio 2015 x64
Create library definition for MSVC to link to
`C:\fftw-3.3.5-dll64> lib /machine:x64 /def:libfftw3f-3.def`

```
c:\hackrf\host\build> cmake ../ -G "Visual Studio 14 2015 Win64" \
-DLIBUSB_INCLUDE_DIR=c:\libusb-1.0.21\libusb \
-DLIBUSB_LIBRARIES=c:\libusb-1.0.21\MS64\dll\lib\libusb-1.0.lib \
-DTHREADS_PTHREADS_INCLUDE_DIR=c:\pthreads-w32-2-9-1-release\Pre-built.2\include \
-DTHREADS_PTHREADS_WIN32_LIBRARY=c:\pthreads-w32-2-9-1-release\Pre-built.2\lib\x64\pthreadVC2.lib \
-DFFTW_INCLUDES=C:\fftw-3.3.5-dll64 \
-DFFTW_LIBRARIES=C:\fftw-3.3.5-dll64\libfftw3f-3.lib
```

CMake will produce a solution file named `HackRF.sln` and a series of
project files which can be built with msbuild as follows:
`c:\hackrf\host\build> msbuild HackRF.sln`

## How to build host the software on FreeBSD
You can use the binary package:
`# pkg install hackrf`

You can build and install from ports:
```
# cd /usr/ports/comms/hackrf
# make install
```

## How to build the host software on macOS:

### Install dependencies

Homebrew: `brew install cmake libusb pkg-config`

Install FFTW from [this guide](https://www.fftw.org/install/mac.html)

### Build it
```sh
mkdir host/build
cd host/build
cmake ..
make
sudo make install
sudo update_dyld_shared_cache # equivalent to ldconfig in linux
```

## Credits

principal author: Michael Ossmann <mike@ossmann.com>

http://greatscottgadgets.com/hackrf/
