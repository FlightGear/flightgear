if(NOT "$ENV{BUILD_ID}" STREQUAL "")
  set(JENKINS_BUILD_ID $ENV{BUILD_ID})
  set(JENKINS_BUILD_NUMBER $ENV{BUILD_NUMBER})
  message(STATUS "running under Jenkins, build-number is ${JENKINS_BUILD_NUMBER}")
else()
  set(JENKINS_BUILD_NUMBER 0)
  set(JENKINS_BUILD_ID "none")
endif()

find_package(Git)
if (Git_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} --git-dir ${SRC}/.git rev-parse  HEAD
        OUTPUT_VARIABLE REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Git revision is ${REVISION}")
else()
    set(REVISION "none")
endif()

configure_file (${SRC}/src/Include/flightgearBuildId.h.cmake-in ${DST})