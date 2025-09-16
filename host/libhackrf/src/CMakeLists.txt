# Copyright (c) 2012, Jared Boone <jared@sharebrained.com>
# Copyright (c) 2013, Benjamin Vernoux <titanmkd@gmail.com>
# Copyright (c) 2013, Michael Ossmann <mike@ossmann.com>
# Copyright (c) 2025, A. Maitland Bottoms <bottoms@debian.org>
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer. Redistributions in binary
# form must reproduce the above copyright notice, this list of conditions and
# the following disclaimer in the documentation and/or other materials provided
# with the distribution. Neither the name of Great Scott Gadgets nor the names
# of its contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

option(ENABLE_STATIC_LIB "Build and Install libhackrf.a static library" ON)
option(ENABLE_SHARED_LIB "Build and Install libhackrf.so shared library" ON)
option(
  DISABLE_USB_DEVICE_DISCOVERY
  "Prevent libusb from trying to enumerate devices. Useful on non-root android"
  ANDROID)

# For cygwin just force UNIX OFF and WIN32 ON
if(${CYGWIN})
  set(UNIX OFF)
  set(WIN32 ON)
endif(${CYGWIN})

# Settings common for shared and static libraries
function(libhackrf_common_settings libtarget)
  target_compile_features(${libtarget} PRIVATE c_std_90)
  set_target_properties(hackrf PROPERTIES CLEAN_DIRECT_OUTPUT 1)

  target_compile_definitions(
    ${libtarget} PRIVATE -DLIBRARY_VERSION="${PROJECT_VERSION}"
                         -DLIBRARY_RELEASE="${RELEASE}")
  if(DISABLE_USB_DEVICE_DISCOVERY)
    target_compile_definitions(${libtarget}
                               PRIVATE -DDISABLE_USB_DEVICE_DISCOVERY)
  endif()

  target_include_directories(
    ${libtarget}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/libhackrf>)

  target_link_libraries(${libtarget} PRIVATE LIBUSB::LIBUSB)
  if(TARGET PThreads4W::PThreads4W)
    target_link_libraries(${libtarget} PRIVATE PThreads4W::PThreads4W)
  else()
    target_link_libraries(${libtarget} PRIVATE Threads::Threads)
  endif()

  if(${UNIX})
    install(
      TARGETS ${libtarget}
      EXPORT HackRFTargets
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT sharedlibs)
  endif(${UNIX})

  if(${WIN32})
    install(
      TARGETS ${libtarget}
      EXPORT HackRFTargets
      DESTINATION bin
      COMPONENT sharedlibs)
  endif(${WIN32})
endfunction()

# Dynamic library
if(ENABLE_SHARED_LIB)
  add_library(hackrf SHARED hackrf.c)
  set_target_properties(${libtarget} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
  libhackrf_common_settings(hackrf)
  generate_export_header(hackrf)
  add_library(HackRF::hackrf ALIAS hackrf)
endif()

# Static library
if(ENABLE_STATIC_LIB)
  add_library(hackrf_static STATIC hackrf.c)
  if(MSVC)
    set_target_properties(hackrf_static PROPERTIES OUTPUT_NAME "hackrf_static")
  else()
    set_target_properties(hackrf_static PROPERTIES OUTPUT_NAME "hackrf")
  endif()
  libhackrf_common_settings(hackrf_static)
endif()

if(${WIN32})
  install(
    DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/"
    DESTINATION bin
    FILES_MATCHING
    PATTERN "libusb*.dll"
    PATTERN "pthread*.dll")
endif(${WIN32})

install(
  FILES hackrf.h
  # "${CMAKE_CURRENT_BINARY_DIR}/hackrf_export.h"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
  COMPONENT headers)
