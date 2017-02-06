#top dir cmake project for libhackrf + tools

cmake_minimum_required(VERSION 2.8)
project (hackrf_all)

#set(RELEASE "")

if(NOT DEFINED RELEASE)
	execute_process(
	        COMMAND git log -n 1 --format=%h
	        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	        RESULT_VARIABLE GIT_EXIT_VALUE
	        ERROR_QUIET
	        OUTPUT_VARIABLE GIT_VERSION
	        OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if (GIT_EXIT_VALUE)
	        set(RELEASE "unknown")
	else (GIT_EXIT_VALUE)
		execute_process(
			COMMAND git status -s --untracked-files=no
			OUTPUT_VARIABLE DIRTY
		)
		if ( NOT "${DIRTY}" STREQUAL "" )
			set(DIRTY_FLAG "*")
		else()
			set(DIRTY_FLAG "")
		endif()
	        set(RELEASE "git-${GIT_VERSION}${DIRTY_FLAG}")
	endif (GIT_EXIT_VALUE)
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
