# Try to find the libusb library
# Once done this defines
#
# LIBUSB_FOUND - system has libusb
# LIBUSB_INCLUDE_DIR - the libusb include directory
# LIBUSB_LIBRARIES - Link these to use libusb
# LIBUSB::LIBUSB - CMake imported library target

# Copyright (c) 2006, 2008  Laurent Montel, <montel@kde.org>
# Copyright (c) 2023 Jamie Smith <jsmith@crackofdawn.onmicrosoft.com>
# Copyright (c) 2025 A.  Maitland Bottoms <bottoms@debian.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(LIBUSB_INCLUDE_DIR AND LIBUSB_LIBRARIES)

  # in cache already
  set(LIBUSB_FOUND TRUE)

else(LIBUSB_INCLUDE_DIR AND LIBUSB_LIBRARIES)
  if(NOT WIN32)
    # use pkg-config to get the directories and then use these values in the
    # FIND_PATH() and FIND_LIBRARY() calls
    find_package(PkgConfig)
    pkg_check_modules(PC_LIBUSB libusb-1.0)
  endif(NOT WIN32)

  set(LIBUSB_LIBRARY_NAME usb-1.0)
  if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    set(LIBUSB_LIBRARY_NAME usb)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # vcpkg's libusb-1.0 has a "lib" prefix, but on Windows MVSC, CMake doesn't
    # search for static libraries with lib prefixes automatically.
    list(APPEND LIBUSB_LIBRARY_NAMES libusb-1.0)
  endif()

  find_path(LIBUSB_INCLUDE_DIR libusb.h PATHS ${PC_LIBUSB_INCLUDEDIR}
                                              ${PC_LIBUSB_INCLUDE_DIRS})

  find_library(
    LIBUSB_LIBRARIES
    NAMES ${LIBUSB_LIBRARY_NAME}
    PATHS ${PC_LIBUSB_LIBDIR} ${PC_LIBUSB_LIBRARY_DIRS})

endif(LIBUSB_INCLUDE_DIR AND LIBUSB_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBUSB DEFAULT_MSG LIBUSB_LIBRARIES
                                  LIBUSB_INCLUDE_DIR)
mark_as_advanced(LIBUSB_INCLUDE_DIR LIBUSB_LIBRARIES)

if(LIBUSB_FOUND AND NOT TARGET LIBUSB::LIBUSB)
  add_library(LIBUSB::LIBUSB INTERFACE IMPORTED)
  set_target_properties(
    LIBUSB::LIBUSB
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LIBUSB_INCLUDE_DIR}"
               INTERFACE_LINK_LIBRARIES "${LIBUSB_LIBRARIES}")
endif()
