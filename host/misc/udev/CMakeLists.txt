cmake_minimum_required(VERSION 2.8)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(SYSTEM_IS_LINUX TRUE)
    SET(UDEV_OPTION_DEFAULT ON)
else()
    SET(SYSTEM_IS_LINUX FALSE)
    SET(UDEV_OPTION_DEFAULT OFF)
endif()

option(INSTALL_UDEV_RULES
    "Install udev rules for the HackRF"
    ${UDEV_OPTION_DEFAULT}
)

set(UDEV_RULES_PATH
    "/etc/udev/rules.d"
    CACHE STRING
    "Target directory for udev rule installation. Ensure you have permissions to write to this directory."
)

if(SYSTEM_IS_LINUX)
    if(INSTALL_UDEV_RULES)
        if(NOT DEFINED UDEV_RULES_GROUP)
            foreach(group usb plugdev)
                execute_process(COMMAND "getent" group "${group}"
                                RESULT_VARIABLE _GETENT_RESULT
                                OUTPUT_QUIET
                                ERROR_QUIET)
                if(NOT _GETENT_RESULT)
                    message(STATUS "Setting udev rule group to - ${group}")
                    set(UDEV_RULES_GROUP ${group})
                    break()
                endif(NOT _GETENT_RESULT)
            endforeach(group)
        endif(NOT DEFINED UDEV_RULES_GROUP)
        if(DEFINED UDEV_RULES_GROUP)
            set(HACKRF_GROUP "${UDEV_RULES_GROUP}"
                CACHE STRING "Group to associate HackRF devices with in udev rules")
            configure_file(${CMAKE_CURRENT_SOURCE_DIR}/53-hackrf.rules.in
                        ${CMAKE_CURRENT_BINARY_DIR}/53-hackrf.rules
                        @ONLY
                        )
            message(STATUS  "HackRF udev rules will be installed to '${UDEV_RULES_PATH}' upon running 'make install'")
            install(FILES ${CMAKE_CURRENT_BINARY_DIR}/53-hackrf.rules
                    DESTINATION ${UDEV_RULES_PATH}
                    COMPONENT "udev_rules")
        else(UDEV_RULES_GROUP)
            message(STATUS "HackRF udev rules will not be installed because no suitable group was found")
            message(STATUS "A group can be specified with -DUDEV_RULES_GROUP=<group>")
        endif(DEFINED UDEV_RULES_GROUP)
    else(INSTALL_UDEV_RULES)
        message(STATUS
                "HackRF udev rules will not be installed because INSTALL_UDEV_RULES=OFF"
               )
    endif(INSTALL_UDEV_RULES)
else(SYSTEM_IS_LINUX)
    if(INSTALL_UDEV_RULES)
        message(STATUS "udev rules not supported on this platform. Hide this message via -DINSTALL_UDEV_RULES=Off")
    endif(INSTALL_UDEV_RULES)
endif(SYSTEM_IS_LINUX)
