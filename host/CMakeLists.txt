#top dir cmake project for libhackrf + tools

cmake_minimum_required(VERSION 3.8)
project(HackRF LANGUAGES C)

# standard policies for CMake 3.8
cmake_policy(VERSION 3.8)

# Run common init code
# -----------------------------------------------------
include(cmake/hackrf-host.cmake)

# Subdirectories
# -----------------------------------------------------
add_subdirectory(libhackrf)
add_subdirectory(hackrf-tools)

