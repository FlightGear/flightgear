add_executable(fgrcc fgrcc.cxx fgrcc.hxx)
target_link_libraries(fgrcc SimGearCore ${PLATFORM_LIBS})

add_custom_target(
    embeddedresources
    COMMAND
      fgrcc --root=${CMAKE_SOURCE_DIR}
            --output-cpp-file=${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.cxx
            --init-func-name=initFlightGearEmbeddedResources
            --output-header-file=${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.hxx
            --output-header-identifier=_FG_FLIGHTGEAR_EMBEDDED_RESOURCES
            ${CMAKE_SOURCE_DIR}/src/EmbeddedResources/FlightGear-resources.xml
    BYPRODUCTS
      ${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.cxx
      ${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.hxx
    DEPENDS
      fgrcc ${CMAKE_SOURCE_DIR}/src/EmbeddedResources/FlightGear-resources.xml
)
