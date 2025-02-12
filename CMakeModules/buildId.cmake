


find_package(Git)
if (Git_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} --git-dir ${SRC}/.git rev-parse  HEAD
        OUTPUT_VARIABLE REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Git revision is ${REVISION}")
else()
    set(REVISION "none")
endif()

string(TIMESTAMP CURRENT_DATE "%Y-%m-%d")

configure_file (${SRC}/src/Include/flightgearBuildId.h.cmake-in ${DST})