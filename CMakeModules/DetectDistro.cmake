# DetectDistro.cmake -- Detect Linux distribution

message(STATUS "System is: ${CMAKE_SYSTEM_NAME}")

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    # Detect Linux distribution (if possible)
    execute_process(COMMAND "/usr/bin/lsb_release" "-is"
                    TIMEOUT 4
                    OUTPUT_VARIABLE LINUX_DISTRO
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Linux distro is: ${LINUX_DISTRO}")
endif()
