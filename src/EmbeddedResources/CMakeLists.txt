add_executable(fgrcc fgrcc.cxx fgrcc.hxx)
target_link_libraries(fgrcc SimGearCore)

# On Windows, make sure fgrcc can be run (it needs third-party libraries) in add_custom_target
if(MSVC)
  set_target_properties(fgrcc PROPERTIES DEBUG_POSTFIX d)
    if (FINAL_MSVC_3RDPARTY_DIR)
      set(CMAKE_MSVCIDE_RUN_PATH ${FINAL_MSVC_3RDPARTY_DIR}/bin)
    else()
      message(FATAL_ERROR "FINAL_MSVC_3RDPARTY_DIR is empty or unset")
    endif()
endif()

add_custom_command(
    COMMAND
      fgrcc --root=${CMAKE_SOURCE_DIR}
            --output-cpp-file=${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.cxx
            --init-func-name=initFlightGearEmbeddedResources
            --output-header-file=${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.hxx
            --output-header-identifier=_FG_FLIGHTGEAR_EMBEDDED_RESOURCES
            ${CMAKE_SOURCE_DIR}/src/EmbeddedResources/FlightGear-resources.xml
    OUTPUT
      ${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.cxx
      ${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.hxx
    MAIN_DEPENDENCY
      fgrcc ${CMAKE_SOURCE_DIR}/src/EmbeddedResources/FlightGear-resources.xml
    COMMENT
      "Re-generating embedded resources"
)

# set automatically by add_custom_command? 
#set_source_files_properties(${EMBEDDED_RESOURCE_SOURCES} PROPERTIES GENERATED TRUE)
#set_source_files_properties(${EMBEDDED_RESOURCE_HEADERS} PROPERTIES GENERATED TRUE)

add_library(fgembeddedresources STATIC 
  ${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.cxx
  ${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.hxx
  )

# users of the library include this as <EmbeddedResources/FlightGear-resources.hxx>
target_include_directories(fgembeddedresources INTERFACE ${CMAKE_BINARY_DIR}/src)

target_link_libraries(fgembeddedresources SimGearCore)

# don't try to Automoc these files, it confuses the dependency logic
set_property(TARGET fgembeddedresources PROPERTY AUTOMOC OFF)

