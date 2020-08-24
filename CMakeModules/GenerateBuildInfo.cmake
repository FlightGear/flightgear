
add_custom_target(
  buildId
  ${CMAKE_COMMAND} -D SRC=${CMAKE_SOURCE_DIR}
                   -D DST=${CMAKE_BINARY_DIR}/src/Include/flightgearBuildId.h
                   -P ${CMAKE_SOURCE_DIR}/CMakeModules/buildId.cmake
)

