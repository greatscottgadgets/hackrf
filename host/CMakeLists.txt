# top dir cmake project for libhackrf + tools

cmake_minimum_required(VERSION 3.10.0)
project(HackRF C)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED FLASH_MB_SIZE)
    set(FLASH_MB_SIZE 1) # Default to 1MB if not provided. For PortaRf this should be 2MB passed to cmake with -DFLASH_MB_SIZE=2
endif()
message("Configuring for FLASH_MB_SIZE=${FLASH_MB_SIZE}MB")
#This is the flash memory size we build. This can be less, so the FW size will be less, so can be flashed with older utils.
if(NOT DEFINED FLASH_MB_LIMIT_SIZE)
	set(FLASH_MB_LIMIT_SIZE 1) #TODO: should be ${FLASH_MB_SIZE} remove once we got a stable build for all devices we support with bigger than 1 mb flash
endif()
message("Configuring for FLASH_MB_LIMIT_SIZE=${FLASH_MB_LIMIT_SIZE}MB")

if(FLASH_MB_SIZE LESS FLASH_MB_LIMIT_SIZE)
    message(FATAL_ERROR "Error: The supported flash size is lower than the generated flash file. Build halted.")
endif()

if(NOT DEFINED FLASH_BYTES_SIZE)
    math(EXPR FLASH_BYTES_SIZE "${FLASH_MB_SIZE} * 1024 * 1024")
endif()
if(NOT DEFINED FLASH_BYTES_LIMIT_SIZE)
    math(EXPR FLASH_BYTES_LIMIT_SIZE "${FLASH_MB_LIMIT_SIZE} * 1024 * 1024")
endif()

#generate h file
configure_file(../flashsize.h.in ${CMAKE_CURRENT_SOURCE_DIR}/../flashsize.h)


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
