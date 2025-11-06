.. _operating_system_tips:

==========================
Installing HackRF Software
==========================

HackRF software includes HackRF Tools and libhackrf. HackRF Tools are the commandline utilities that let you interact with your HackRF. libhackrf is a low level library that enables software on your computer to operate with HackRF. 



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

Windows users can use `radioconda <https://github.com/ryanvolz/radioconda>`__ to get the required binaries installed.

Alternatively, binaries are available as build artifacts under the 'Actions'-tab on github `here <https://github.com/greatscottgadgets/hackrf/actions>`__ (GitHub Login needed).



-----------



Installing From Source
~~~~~~~~~~~~~~~~~~~~~~

Linux / OS X / \*BSD: Building HackRF Software From Source
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Acquire the source for the HackRF tools from either a `release archive <https://github.com/greatscottgadgets/hackrf/releases>`__ or git: ``git clone https://github.com/greatscottgadgets/hackrf.git``

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

Windows: Building HackRF Software From Source
+++++++++++++++++++++++++++++++++++++++++++++

Install `Visual Studio Community <https://visualstudio.microsoft.com/vs/community/>`__ (2015 or later) and `CMake <https://cmake.org/>`__ (at least version 3.21.4).

Install library dependencies using `vcpkg <https://vcpkg.io/en/>`__:

.. code-block :: winbatch

	git clone https://github.com/microsoft/vcpkg
	cd vcpkg
	bootstrap-vcpkg.bat
	vcpkg install libusb fftw3 pthreads pkgconf

Open the Visual Studio Developer Command Prompt, and change to the directory where you unpacked the HackRF source.

The following steps assume you installed vcpkg in ``C:\vcpkg``.

Configure CMake and build the code:

.. code-block :: winbatch

	set PKG_CONFIG=C:\vcpkg\installed\x64-windows\tools\pkgconf\pkgconf.exe
	set PKG_CONFIG_PATH=C:\vcpkg\installed\x64-windows\lib\pkgconfig
	set CMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
	cmake -B host\build host
	cmake --build host\build

CMake will generate a ``HackRF.sln`` project file which you can open in Visual Studio for editing and development.
