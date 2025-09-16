# top dir cmake project for libhackrf + tools

cmake_minimum_required(VERSION 3.16)
project(
  HackRF
  LANGUAGES C
  HOMEPAGE_URL https://greatscottgadgets.com/hackrf/)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_FLAGS
    "$ENV{CFLAGS}"
    CACHE STRING "C Flags")

add_subdirectory(libhackrf)
add_subdirectory(hackrf-tools)

# ##############################################################################
# Create uninstall target
# ##############################################################################

configure_file(${PROJECT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake @ONLY)

add_custom_target(
  uninstall
  COMMENT "Provide uninstall target"
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
