
set(BUILD_ID_DST_PATH "${PROJECT_BINARY_DIR}/src/Include/flightgearBuildId.h")

if (FG_BUILD_TYPE STREQUAL "Dev")
  
  set(REVISION "none")

  # generate a placeholder buildId header
  configure_file (${PROJECT_SOURCE_DIR}/src/Include/flightgearBuildId.h.cmake-in ${BUILD_ID_DST_PATH})

  # dummy target, does nothing
  add_custom_target(buildId)
else()
  # for non-Dev builds, run this each time. using some CMake in script mode
  # this takes a little bit of time since we have to actually run Git as a sub-process
  add_custom_target(
    buildId
    ${CMAKE_COMMAND} -D SRC=${PROJECT_SOURCE_DIR}
                    -D DST=${BUILD_ID_DST_PATH}
                    -P ${PROJECT_SOURCE_DIR}/CMakeModules/buildId.cmake
  )
endif()
