# Test harness for libhackrf
# Uses simple assert-based tests that link against hackrf_static.
# Mock libusb can be added later for hardware-free testing.

cmake_minimum_required(VERSION 3.10.0)

# For now, tests that don't need libusb (parse utilities) or that can
# run with a minimal mock will live here.

if(NOT TARGET hackrf_static)
    message(STATUS "hackrf_static not available, skipping tests")
    return()
endif()

add_executable(test_hackrf_init test_hackrf_init.c)
target_link_libraries(test_hackrf_init PRIVATE hackrf_static)
add_test(NAME test_hackrf_init COMMAND test_hackrf_init)

add_executable(test_cpld_checksum test_cpld_checksum.c mock_libusb.c)
target_link_libraries(test_cpld_checksum PRIVATE hackrf_static)
add_test(NAME test_cpld_checksum COMMAND test_cpld_checksum)

add_executable(test_fpga_bitstream test_fpga_bitstream.c mock_libusb.c)
target_link_libraries(test_fpga_bitstream PRIVATE hackrf_static)
add_test(NAME test_fpga_bitstream COMMAND test_fpga_bitstream)
