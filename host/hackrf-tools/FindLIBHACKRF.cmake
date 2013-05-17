# - Try to find the libhackrf library
# Once done this defines
#
#  LIBHACKRF_FOUND - system has libhackrf
#  LIBHACKRF_INCLUDE_DIR - the libhackrf include directory
#  LIBHACKRF_LIBRARIES - Link these to use libhackrf

# Copyright (c) 2013  Benjamin Vernoux
#


if (LIBHACKRF_INCLUDE_DIR AND LIBHACKRF_LIBRARIES)

  # in cache already
  set(LIBHACKRF_FOUND TRUE)

else (LIBHACKRF_INCLUDE_DIR AND LIBHACKRF_LIBRARIES)
  IF (NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    find_package(PkgConfig)
    pkg_check_modules(PC_LIBHACKRF libhackrf)
  ENDIF(NOT WIN32)

  FIND_PATH(LIBHACKRF_INCLUDE_DIR hackrf.h
    PATHS ${PC_LIBHACKRF_INCLUDEDIR} ${PC_LIBHACKRF_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/../libhackrf/src)

  FIND_LIBRARY(LIBHACKRF_LIBRARIES NAMES libhackrf
    PATHS ${PC_LIBHACKRF_LIBDIR} ${PC_LIBHACKRF_LIBRARY_DIRS} ${CMAKE_SOURCE_DIR}/../libhackrf/src)

  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBHACKRF DEFAULT_MSG LIBHACKRF_LIBRARIES LIBHACKRF_INCLUDE_DIR)

  MARK_AS_ADVANCED(LIBHACKRF_INCLUDE_DIR LIBHACKRF_LIBRARIES)

endif (LIBHACKRF_INCLUDE_DIR AND LIBHACKRF_LIBRARIES)