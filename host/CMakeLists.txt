#top dir cmake project for libhackrf + tools

cmake_minimum_required(VERSION 2.8)
project (hackrf_all)

add_subdirectory(libhackrf)
add_subdirectory(hackrf-tools)

########################################################################
# Create uninstall target
########################################################################

configure_file(
    ${PROJECT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
@ONLY)


add_custom_target(uninstall
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)
