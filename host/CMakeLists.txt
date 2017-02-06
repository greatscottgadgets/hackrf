#top dir cmake project for libhackrf + tools

cmake_minimum_required(VERSION 2.8)
project (hackrf_all)

#set(RELEASE "")

if(NOT DEFINED RELEASE)
	execute_process(
	        COMMAND git log -n 1 --format=%h
	        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	        RESULT_VARIABLE GIT_VERSION_FOUND
	        ERROR_QUIET
	        OUTPUT_VARIABLE GIT_VERSION
	        OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if (GIT_VERSION_FOUND)
	        set(RELEASE "unknown")
	else (GIT_VERSION_FOUND)
	        set(RELEASE "git-${GIT_VERSION}")
	endif (GIT_VERSION_FOUND)
endif()

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
