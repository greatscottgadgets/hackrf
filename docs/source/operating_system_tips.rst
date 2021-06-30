================================================
Operating System Tips
================================================

Here are some software setup tips for particular Operating Systems and Linux distributions.



Package managers
~~~~~~~~~~~~~~~~

We highly recommend that, unless developing or testing new features of HackRF, most users use build systems or package management provided for their operating system.



Linux
^^^^^

Ubuntu / Debian
+++++++++++++++

``sudo apt install gqrx-sdr``

Fedora / Red Hat
++++++++++++++++

``sudo dnf install gnuradio gr-osmosdr hackrf gqrx -y``

Gentoo Linux
++++++++++++

.. code-block :: sh

	emerge -a net-wireless/hackrf-tools
	USE="hackrf" emerge -a net-wireless/gr-osmosdr


Arch Linux
++++++++++

.. code-block :: sh

	pacman -S gnuradio gnuradio-osmosdr
	pacman -S gnuradio-companion



OS X (10.5+)
^^^^^^^^^^^^

MacPorts
++++++++

``sudo port install gr-osmosdr``

Homebrew
++++++++

``brew install gr-osmosdr``



Windows
^^^^^^^

Binaries are provided as part of the PothosSDR project, they can be downloaded `here <http://downloads.myriadrf.org/builds/PothosSDR/?C=M;O=D>`__.



FreeBSD
^^^^^^^

You can use the binary package: ``# pkg install hackrf``

You can build and install from ports:

.. code-block :: sh

	# cd /usr/ports/comms/hackrf
	# make install



Building from source
~~~~~~~~~~~~~~~~~~~~

Linux / OS X / \*BSD
^^^^^^^^^^^^^^^^^^^^

Preparing Your System
+++++++++++++++++++++

First of all, make sure that your system is up to date using your operating system provided update method.

Installing using PyBOMBS
^^^^^^^^^^^^^^^^^^^^^^^^

The GNU Radio project has a `build system <https://www.gnuradio.org/blog/pybombs-the-what-the-how-and-the-why>`__ that covers the core libraries, drivers for SDR hardware, and many out of tree modules. PyBOMBs will take care of installing dependencies for you.


Building HackRF tools from source
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Acquire the source for the HackRF tools from either a `release archive <https://github.com/mossmann/hackrf/releases>`__ or git: ``git clone https://github.com/mossmann/hackrf.git``

Once you have the source downloaded, the host tools can be built as follows:

.. code-block :: sh

	cd hackrf/host
	mkdir build
	cd build
	cmake ..
	make
	sudo make install
	sudo ldconfig

If you have HackRF hardware, you may need to `update the firmware <https://github.com/mossmann/hackrf/wiki/Updating-Firmware>`__ to match the host tools versions.



Windows
^^^^^^^

Prerequisites for Cygwin, MinGW, or Visual Studio
+++++++++++++++++++++++++++++++++++++++++++++++++

    * cmake-2.8.12.1 or later from http://www.cmake.org/cmake/resources/software.html
    * libusbx-1.0.18 or later from http://sourceforge.net/projects/libusbx/files/latest/download?source=files
    * fftw-3.3.5 or later from http://www.fftw.org/install/windows.html
    * Install Windows driver for HackRF hardware or use Zadig see http://sourceforge.net/projects/libwdi/files/zadig
        * If you want to use Zadig select HackRF USB device and just install/replace it with WinUSB driver.

Note for Windows build: You shall always execute hackrf-tools from Windows command shell and not from Cygwin or MinGW shell because on Cygwin/MinGW Ctrl+C is not managed correctly and especially for hackrf_transfer the Ctrl+C (abort) will not stop correctly and will corrupt the file.



For Visual Studio 2015 x64
++++++++++++++++++++++++++

Create library definition for MSVC to link to ``C:\fftw-3.3.5-dll64> lib /machine:x64 /def:libfftw3f-3.def``

.. code-block :: sh

	c:\hackrf\host\build> cmake ../ -G "Visual Studio 14 2015 Win64" \
	-DLIBUSB_INCLUDE_DIR=c:\libusb-1.0.21\libusb \
	-DLIBUSB_LIBRARIES=c:\libusb-1.0.21\MS64\dll\lib\libusb-1.0.lib \
	-DTHREADS_PTHREADS_INCLUDE_DIR=c:\pthreads-w32-2-9-1-release\Pre-built.2\include \
	-DTHREADS_PTHREADS_WIN32_LIBRARY=c:\pthreads-w32-2-9-1-release\Pre-built.2\lib\x64\pthreadVC2.lib \
	-DFFTW_INCLUDES=C:\fftw-3.3.5-dll64 \
	-DFFTW_LIBRARIES=C:\fftw-3.3.5-dll64\libfftw3f-3.lib

CMake will produce a solution file named ``HackRF.sln`` and a series of project files which can be built with msbuild as follows: ``c:\hackrf\host\build> msbuild HackRF.sln``



Cygwin
++++++

.. code-block :: sh

	mkdir host/build
	cd host/build
	cmake ../ -G "Unix Makefiles" -DCMAKE_LEGACY_CYGWIN_WIN32=1 -DLIBUSB_INCLUDE_DIR=/usr/local/include/libusb-1.0/
	make
	make install



MinGW
+++++

.. code-block :: sh

	mkdir host/build
	cd host/build
	cmake ../ -G "MSYS Makefiles" -DLIBUSB_INCLUDE_DIR=/usr/local/include/libusb-1.0/
	make
	make install
