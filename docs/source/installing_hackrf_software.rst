.. _operating_system_tips:

================================================
Installing HackRF Software
================================================

Install Using Package Managers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Unless developing or testing new features for HackRF, we highly recommend that most users use build systems or package managers provided for their operating system. **Our suggested operating system for use with HackRF is Ubuntu**.

FreeBSD
+++++++

You can use the binary package: ``# pkg install hackrf``

You can also build and install from ports:

.. code-block :: sh

	# cd /usr/ports/comms/hackrf
	# make install

Linux: Arch
+++++++++++

.. code-block :: sh

	pacman -S hackrf

Linux: Fedora / Red Hat
+++++++++++++++++++++++

``sudo dnf install hackrf -y``

Linux: Gentoo
+++++++++++++

.. code-block :: sh

	emerge -a net-wireless/hackrf-tools

Linux: Ubuntu / Debian
++++++++++++++++++++++

``sudo apt-get install hackrf``

OS X (10.5+): Homebrew
++++++++++++++++++++++

``brew install hackrf``

OS X (10.5+): MacPorts
++++++++++++++++++++++

``sudo port install hackrf``

Windows: Binaries
+++++++++++++++++

Binaries are provided as part of the PothosSDR project, they can be downloaded `here <http://downloads.myriadrf.org/builds/PothosSDR/?C=M;O=D>`__.

-----------

Installing From Source
~~~~~~~~~~~~~~~~~~~~~~

Linux / OS X / \*BSD: Building HackRF Software From Source
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

If you have HackRF hardware, you may need to :ref:`update the firmware <updating_firmware>` to match the host tools versions.



Windows: Prerequisites for Cygwin, MinGW, or Visual Studio
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    * cmake-2.8.12.1 or later from http://www.cmake.org/cmake/resources/software.html
    * libusbx-1.0.18 or later from http://sourceforge.net/projects/libusbx/files/latest/download?source=files
    * fftw-3.3.5 or later from http://www.fftw.org/install/windows.html
    * Install Windows driver for HackRF hardware or use Zadig see http://sourceforge.net/projects/libwdi/files/zadig
        * If you want to use Zadig select HackRF USB device and just install/replace it with WinUSB driver.

Note for Windows build: You shall always execute hackrf-tools from Windows command shell and not from Cygwin or MinGW shell because on Cygwin/MinGW Ctrl+C is not managed correctly and especially for hackrf_transfer the Ctrl+C (abort) will not stop correctly and will corrupt the file.



Windows: Installing HackRF Software via Cygwin
++++++++++++++++++++++++++++++++++++++++++++++

.. code-block :: sh

	mkdir host/build
	cd host/build
	cmake ../ -G "Unix Makefiles" -DCMAKE_LEGACY_CYGWIN_WIN32=1 -DLIBUSB_INCLUDE_DIR=/usr/local/include/libusb-1.0/
	make
	make install



Windows: Installing HackRF Software via MinGW
+++++++++++++++++++++++++++++++++++++++++++++

.. code-block :: sh

	mkdir host/build
	cd host/build
	cmake ../ -G "MSYS Makefiles" -DLIBUSB_INCLUDE_DIR=/usr/local/include/libusb-1.0/
	make
	make install



Windows: Installing HackRF Software via Visual Studio 2015 x64
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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
