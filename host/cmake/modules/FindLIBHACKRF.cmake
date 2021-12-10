# - Try to find the libhackrf library
# Once done this defines
#
#  LIBHACKRF_FOUND - system has libhackrf
#  LIBHACKRF_INCLUDE_DIR - the libhackrf include directory
#  LIBHACKRF_LIBRARIES - Link these to use libhackrf
#  LIBHACKRF_COMPILE_OPTIONS - Add these compile options when using libhackrf
#
# If libhackrf is found, the following imported target is created:
#  libhackrf::libhackrf

# Copyright (c) 2013  Benjamin Vernoux
#


if (LIBHACKRF_INCLUDE_DIR AND LIBHACKRF_LIBRARIES)

  # in cache already
  set(LIBHACKRF_FIND_QUIETLY TRUE)
endif()

IF (NOT WIN32)
  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  find_package(PkgConfig)
  pkg_check_modules(PC_LIBHACKRF QUIET libhackrf)
ENDIF(NOT WIN32)

FIND_PATH(LIBHACKRF_INCLUDE_DIR
  NAMES hackrf.h
  HINTS $ENV{LIBHACKRF_DIR}/include ${PC_LIBHACKRF_INCLUDEDIR}
  PATH_SUFFIXES libhackrf
)

set(libhackrf_library_names hackrf hackrf_static)

FIND_LIBRARY(LIBHACKRF_LIBRARY
  NAMES ${libhackrf_library_names}
  HINTS $ENV{LIBHACKRF_DIR}/lib ${PC_LIBHACKRF_LIBDIR}
  PATHS /usr/local/lib /usr/lib /opt/local/lib ${PC_LIBHACKRF_LIBRARY_DIRS}
)

# set up shared vs static compile flags (need to check type of library)
set(LIBHACKRF_COMPILE_OPTIONS "")
if(EXISTS "${LIBHACKRF_LIBRARY}")
  get_lib_type(${LIBHACKRF_LIBRARY} HACKRF_LIB_TYPE)

  if(HACKRF_LIB_TYPE STREQUAL "STATIC")
    list(APPEND LIBHACKRF_COMPILE_OPTIONS -Dhackrf_STATIC)
  endif()
endif()

# If we found a static libhackrf, we need to also link in libusb and pthread that were found earlier in order for it to be able to link.
set(LIBHACKRF_EXTRA_LIBS ${LIBUSB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
set(LIBHACKRF_LIBRARIES ${LIBHACKRF_LIBRARY} ${LIBHACKRF_EXTRA_LIBS}) 

try_link_library(LIBHACKRF_WORKS
    LANGUAGE C
    FUNCTION hackrf_init
    LIBRARIES ${LIBHACKRF_LIBRARIES})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBHACKRF DEFAULT_MSG LIBHACKRF_LIBRARY LIBHACKRF_INCLUDE_DIR LIBHACKRF_WORKS)

MARK_AS_ADVANCED(LIBHACKRF_INCLUDE_DIR LIBHACKRF_LIBRARY)

# Create imported target if hackrf was found
if(LIBHACKRF_FOUND)
    add_library(libhackrf::libhackrf UNKNOWN IMPORTED)
    set_property(TARGET libhackrf::libhackrf PROPERTY IMPORTED_LOCATION ${LIBHACKRF_LIBRARY})
    set_property(TARGET libhackrf::libhackrf PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${LIBHACKRF_INCLUDE_DIR})
    set_property(TARGET libhackrf::libhackrf PROPERTY INTERFACE_LINK_LIBRARIES ${LIBHACKRF_EXTRA_LIBS})
    set_property(TARGET libhackrf::libhackrf PROPERTY INTERFACE_COMPILE_OPTIONS ${LIBHACKRF_COMPILE_OPTIONS})

endif()
