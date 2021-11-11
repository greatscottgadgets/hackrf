#[=======================================================================[.rst:
FindCMath
-------

Finds the C Math library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``C::Math``
  The C math library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``CMath_FOUND``
  True if the system has the C math library.

``CMath_LIBRARIES``
  List of C math libraries. Might be empty.

#]=======================================================================]

include(CheckCSourceCompiles)

set(CHECK_SINE_C_SOURCE "#include <math.h>

int main(int argc, char** argv)
{
        return sin(argc - 1);
}")

set(CMath_WORKS FALSE)

check_c_source_compiles("${CHECK_SINE_C_SOURCE}" CMath_LINKS_WITHOUT_M)

if ("${CMath_LINKS_WITHOUT_M}")
	set(CMath_LIBRARIES "")
else()
	find_library(CMath_LIBRARIES NAMES m math libm)

	if(EXISTS "${CMath_LIBRARIES}")
		cmake_push_check_state()
		set(CMAKE_REQUIRED_FLAGS "")
		set(CMAKE_REQUIRED_LIBRARIES "${CMath_LIBRARIES}")
		check_c_source_compiles("${CHECK_SINE_C_SOURCE}" CMath_LINKS_WITH_M)
		set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_SAVE_CMATH})
		set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_SAVE_CMATH})
		cmake_pop_check_state()
	else()
		set(CMath_LINKS_WITH_M FALSE)
	endif()
endif ()

if (("${CMath_LINKS_WITH_M}" OR "${CMath_LINKS_WITHOUT_M}") )
	set(CMath_WORKS TRUE)
endif()

# we want FPHSA to print a library path when libm is found as a library
if(CMath_LINKS_WITH_M)
	set(CMath_REQUIRED_VARS CMath_LIBRARIES CMath_WORKS)
else()
	set(CMath_REQUIRED_VARS CMath_WORKS)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CMath DEFAULT_MSG ${CMath_REQUIRED_VARS})

if ("${CMath_FOUND}" AND NOT TARGET C::Math)

	if(CMath_LINKS_WITH_M)
		# create imported target
		import_library(C::Math "${CMath_LIBRARIES}")
	else()
		# create empty target
		add_library(C::Math INTERFACE IMPORTED GLOBAL)
	endif()

endif ()


# Mark variables as advanced, effectively hiding from ccmake interface
mark_as_advanced(
		CMath_LINKS_WITHOUT_M
		CMath_LINKS_WITH_M
		CMath_LIBRARIES
)