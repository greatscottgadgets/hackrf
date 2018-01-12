# add a target to generate API documentation with Doxygen
find_package(Doxygen)
option(
  BUILD_DOCUMENTATION
  "Create and install the HTML based API documentation (requires Doxygen)"
  ${DOXYGEN_FOUND})

if(BUILD_DOCUMENTATION)
    if(NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build the documentation.")
    endif()

    set(DOX_EXECUTABLE ${DOXYGEN_EXECUTABLE})
    set(DOX_INCLUDE    ${CMAKE_CURRENT_SOURCE_DIR}/include)
    set(DOX_WRAPPER    ${CMAKE_CURRENT_SOURCE_DIR}/doxygen.bash)
    set(DOXYFILE_IN    ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYFILE       ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${DOXYFILE_IN} ${DOXYFILE} @ONLY)

    file(COPY ${DOX_INCLUDE}
      DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

    add_custom_target(doc ALL
      COMMAND bash ${DOX_WRAPPER} ${DOX_EXECUTABLE} ${DOXYFILE}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM)

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/
            DESTINATION share/doc/${PROJECT_NAME})
endif()
