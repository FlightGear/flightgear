add_executable(fgrcc fgrcc.cxx fgrcc.hxx)
target_link_libraries(fgrcc SimGearCore ${PLATFORM_LIBS})

# On Windows, make sure fgrcc can be run (it needs third-party libraries) in add_custom_target
if(MSVC)
  set_target_properties(fgrcc PROPERTIES DEBUG_POSTFIX d)
  if(MSVC_3RDPARTY_ROOT AND MSVC_3RDPARTY_DIR)
    set(CMAKE_MSVCIDE_RUN_PATH ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/bin)
  else()
    message(FATAL_ERROR
      "Either MSVC_3RDPARTY_ROOT or MSVC_3RDPARTY_DIR is empty or unset")
  endif()
endif()

add_custom_target(
    embeddedresources
    COMMAND
      fgrcc --root=${PROJECT_SOURCE_DIR}
            --output-cpp-file=${PROJECT_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.cxx
            --init-func-name=initFlightGearEmbeddedResources
            --output-header-file=${PROJECT_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.hxx
            --output-header-identifier=_FG_FLIGHTGEAR_EMBEDDED_RESOURCES
            ${PROJECT_SOURCE_DIR}/src/EmbeddedResources/FlightGear-resources.xml
    BYPRODUCTS
      ${PROJECT_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.cxx
      ${PROJECT_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.hxx
    DEPENDS
      fgrcc ${PROJECT_SOURCE_DIR}/src/EmbeddedResources/FlightGear-resources.xml
)
