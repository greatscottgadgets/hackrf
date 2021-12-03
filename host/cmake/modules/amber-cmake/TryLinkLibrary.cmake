#[=======================================================================[.rst:
TryLinkLibrary
------------------

Check if a library works and is linkable using the current compiler.  This works by compiling and linking an executable file that calls one of the library's functions.

TryLinkLibrary was designed as an upgraded replacement for the modules CheckLibraryExists and CheckC(XX)SourceCompiles.  It can replace any existing usage of these modules with a more convenient and flexible syntax.  It can also handle other situations, such as Fortran code and Windows DLLs.  Last but not least, it can work with imported and interface targets in its LIBRARIES, so it's compatible with newer find modules that only provide imported targets. 

By default, source code for a void no-argument function prototype is generated according to the function name and then called in the test code.  This will work for most cases, but breaks in certain scenarios: 

- DLL imports/exports on Windows.  This requires the function to be declared with a specific prefix, like "extern" or "extern __declspec(dllimport)".  Use the FUNC_PREFIX option to set this.
- C++ mangling.  C++ functions have different symbol names based on their arguments and return type.  For C++, use the FUNC_DECLARATION and FUNC_CALL options to specify custom blocks of code for this.
- Fortran modules.  Requires a seperate use statement and then a function call.  Use FUNC_CALL to provide code for this.

.. command:: try_link_library

  .. code-block:: cmake

    try_link_library(<result var> 
      LANGUAGE <C|CXX|Fortran> 
      FUNCTION <function name> 
      [LIBRARIES <library paths...>]
      [INCLUDES <include paths...>]
      [FUNC_PREFIX <prefix>]
      [FUNC_DECLARATION <declaration code>]
      [FUNC_CALL <calling code>])

  ::

    <result var>     - Name of cache variable to store result in.  Gets set to TRUE or FALSE.
    LANGUAGE         - Language to perform the check in.  Important so that mangling is done correctly.
    FUNCTION         - Name of function in the library to attempt to call.  If both FUNC_DECLARATION and FUNC_CALL are provided, then this will only be used for status messages.
    LIBRARIES        - Library paths and imported targets to link when doing this check.  
    INCLUDES         - Include paths for headers/modules that are used in the check.
    FUNC_PREFIX      - Optional code to put before the start of the function declaration in C and CXX.
    FUNC_DECLARATION - Optional code to use in place of the default function declaration in C and CXX.  Must prepare the named function to be called later in the code.  Can be multiline.  Can also include the library's header if that is easier to work with.
    FUNC_CALL        - Optional code to use to call the function.  Can be multiline.  NOTE: CMake argument passing weirdness prevents a trailing semicolon from being passed in FUNC_CALL.  For C and C++, a trailing semicolon is added to the code passed.

Under some conditions, if it appears that nonexistant libraries were passed to the LIBRARIES argument or nonexistant directories were passed to INCLUDES, the check will immediately 
report false since preceding code has clearly failed to find the needed libraries.  This will happen if:
* A library is not a target and ends in -NOTFOUND
* A library is an imported target name (containing "::"), but does not currently exist
* An include ends in -NOTFOUND

The following variables may be set before calling this macro to modify
the way the check is run:

::

  CMAKE_REQUIRED_FLAGS = string of compile command line flags
  CMAKE_REQUIRED_DEFINITIONS = list of macros to define (-DFOO=bar)
  CMAKE_REQUIRED_LINK_OPTIONS = list of options to pass to link command
  CMAKE_REQUIRED_LIBRARIES = list of additonal libraries to link
  CMAKE_REQUIRED_QUIET = execute quietly without messages
  CMAKE_POSITION_INDEPENDENT_CODE = whether to compile a position independent executable (and therefore require the checked libraries to be position independent)
#]=======================================================================]

function(try_link_library RESULT_VAR)

  if(DEFINED "${RESULT_VAR}")
    # already in cache
    return()
  endif()

  # Parse arguments
  # ----------------------------------------------------------
  # NOTE: FUNC_DECLARATION and FUNC_CALL must be listed as multi-value arguments in order to allow code containing semicolons
  cmake_parse_arguments(TLL "" "LANGUAGE;FUNCTION;FUNC_PREFIX" "LIBRARIES;INCLUDES;FUNC_DECLARATION;FUNC_CALL" ${ARGN})

  if("${TLL_LANGUAGE}" STREQUAL "" OR "${TLL_FUNCTION}" STREQUAL "")
    message(FATAL_ERROR "Usage error: required arguments missing.")
  endif()

  if(NOT "${TLL_UNPARSED_ARGUMENTS}" STREQUAL "")
    message(FATAL_ERROR "Usage error: extra or unparsed arguments provided.")
  endif()

  if(NOT ("${TLL_LANGUAGE}" STREQUAL "C" OR "${TLL_LANGUAGE}" STREQUAL "CXX" OR "${TLL_LANGUAGE}" STREQUAL "Fortran"))
    message(FATAL_ERROR "Usage error: unsupported LANGUAGE.")
  endif()

  if(NOT "${CMAKE_${TLL_LANGUAGE}_COMPILER_LOADED}")
     message(FATAL_ERROR "Error: no compiler for ${TLL_LANGUAGE} has been loaded.")
  endif()

  # Write source file
  # ----------------------------------------------------------

  if("${TLL_LANGUAGE}" STREQUAL "C")
    set(SOURCE_EXTENSION ".c")
  elseif("${TLL_LANGUAGE}" STREQUAL "CXX")
    set(SOURCE_EXTENSION ".cpp")
  elseif("${TLL_LANGUAGE}" STREQUAL "Fortran")
    set(SOURCE_EXTENSION ".F90")
  endif()

  set(SOURCE_FILE_PATH "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/TryLinkLibrary${SOURCE_EXTENSION}")

  if("${TLL_LANGUAGE}" STREQUAL "Fortran")

    if("${TLL_FUNC_CALL}" STREQUAL "")
      set(TLL_FUNC_CALL "call ${TLL_FUNCTION}()")
    endif()

    file(WRITE ${SOURCE_FILE_PATH}
"program testf
  ${TLL_FUNC_CALL}
end program testf
")

  else() # C or CXX

    if("${TLL_FUNC_DECLARATION}" STREQUAL "")
      set(TLL_FUNC_DECLARATION "void ${TLL_FUNCTION}();")
    endif()

    if("${TLL_FUNC_CALL}" STREQUAL "")
      set(TLL_FUNC_CALL "${TLL_FUNCTION}()")
    endif()

    file(WRITE ${SOURCE_FILE_PATH} "
${TLL_FUNC_PREFIX} ${TLL_FUNC_DECLARATION}

int main(int argc, char** args)
{
  ${TLL_FUNC_CALL};
  return 0;
}")

  endif()

  # Gather libraries
  # ----------------------------------------------------------
  set(ALL_LIBRARIES ${TLL_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES})

  # convert library targets to paths
  resolve_cmake_library_list(ALL_LIBRARY_PATHS ${ALL_LIBRARIES})

  # check for missing libraries
  set(MISSING_LIBRARIES FALSE)
  foreach(LIBRARY ${ALL_LIBRARY_PATHS})
    if("${LIBRARY}" MATCHES "-NOTFOUND$")
      set(MISSING_LIBRARIES TRUE)
    elseif(NOT EXISTS "${LIBRARY}")
      # resolved library is not a full path, so it must be a target or a library to be linked with -l
      if("${LIBRARY}" MATCHES "::")
        # library has an imported target name.  Passing an unresolved target with this name to try_compile would cause a 
        # cmake error, so we have to bail out here.
        set(MISSING_LIBRARIES TRUE)
      endif()
    endif()
  endforeach()

  # Also check for NOTFOUND includes
  foreach(INCLUDE ${TLL_INCLUDES})
    if("${INCLUDE}" MATCHES "-NOTFOUND$")
      set(MISSING_LIBRARIES TRUE)
    endif()
  endforeach()

  if(MISSING_LIBRARIES)
    set(${RESULT_VAR} FALSE PARENT_SCOPE)
    return()
  endif()

  # generate message text
  set(LIBRARY_MESSAGE_TEXT "")
  foreach(LIBRARY ${ALL_LIBRARY_PATHS})
    get_filename_component(LIBRARY_FILENAME ${LIBRARY} NAME)
    string(APPEND LIBRARY_MESSAGE_TEXT " ${LIBRARY_FILENAME}")
  endforeach()

  # Compile the code
  # ----------------------------------------------------------

  if(NOT CMAKE_REQUIRED_QUIET)
    message(STATUS "Looking for ${TLL_FUNCTION} in${LIBRARY_MESSAGE_TEXT}")
  endif()
  try_compile(${RESULT_VAR}
    ${CMAKE_BINARY_DIR}
    SOURCES ${SOURCE_FILE_PATH}
    COMPILE_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
    LINK_LIBRARIES ${ALL_LIBRARY_PATHS}
    CMAKE_FLAGS 
      "-DINCLUDE_DIRECTORIES=${TLL_INCLUDES}"
    OUTPUT_VARIABLE TRY_COMPILE_OUTPUT)

  if(${RESULT_VAR})

    if(NOT CMAKE_REQUIRED_QUIET)
      message(STATUS "Looking for ${TLL_FUNCTION} in${LIBRARY_MESSAGE_TEXT} - found")
    endif()

    # update docstring and type
    set(${RESULT_VAR} TRUE CACHE INTERNAL "Have function ${TLL_FUNCTION} in${LIBRARY_MESSAGE_TEXT}")

    file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
      "Determining if the function ${TLL_FUNCTION} exists in${LIBRARY_MESSAGE_TEXT} "
      "passed with the following output:\n"
      "${TRY_COMPILE_OUTPUT}\n\n")

  else()

    if(NOT CMAKE_REQUIRED_QUIET)
      message(STATUS "Looking for ${TLL_FUNCTION} in${LIBRARY_MESSAGE_TEXT} - not found")
      message(STATUS "Detailed information about why this check failed can be found in ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log")
    endif()

    # update docstring and type
    set(${RESULT_VAR} FALSE CACHE INTERNAL "Have function ${TLL_FUNCTION} in library ${TLL_NAME}")

    file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
      "Determining if the function ${TLL_FUNCTION} exists in the library ${TLL_NAME} "
      "failed with the following output:\n"
      "${TRY_COMPILE_OUTPUT}\n\n")

  endif()

endfunction(try_link_library)