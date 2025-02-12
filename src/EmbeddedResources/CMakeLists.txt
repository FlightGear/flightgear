add_executable(fgrcc fgrcc.cxx fgrcc.hxx)
target_link_libraries(fgrcc SimGearCore)

# On Windows, make sure fgrcc can be run (it needs third-party libraries) in add_custom_target
# note this only works when using the Visual Studio generators, and *not* when using Ninja
# for Ninja, easiest fix is to ensure the Windows-3rd-party bin dir is in PATH. 
# https://stackoverflow.com/questions/35029277/how-to-modify-environment-variables-passed-to-custom-cmake-target
# Even better would be to use cmake -E env fgrcc, but this is currently fighting me.

if(MSVC)
  set_target_properties(fgrcc PROPERTIES DEBUG_POSTFIX d)
  if (FINAL_MSVC_3RDPARTY_DIR)
    set(CMAKE_MSVCIDE_RUN_PATH ${FINAL_MSVC_3RDPARTY_DIR}/bin)
  else()
    message(FATAL_ERROR "FINAL_MSVC_3RDPARTY_DIR is empty or unset")
  endif()

  if (CMAKE_GENERATOR MATCHES "Ninja*")
    message(WARNING "Ninja generator in use on Windows, ensure PATH is set to include zlib.dll or FGRCC will fail to run.")
  endif()
endif()

add_custom_command(
    COMMAND
      fgrcc --root=${PROJECT_SOURCE_DIR}
            --output-cpp-file=${CMAKE_CURRENT_BINARY_DIR}/FlightGear-resources.cxx
            --init-func-name=initFlightGearEmbeddedResources
            --output-header-file=${CMAKE_CURRENT_BINARY_DIR}/FlightGear-resources.hxx
            --output-header-identifier=_FG_FLIGHTGEAR_EMBEDDED_RESOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/FlightGear-resources.xml
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/FlightGear-resources.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/FlightGear-resources.hxx
    MAIN_DEPENDENCY
      fgrcc ${CMAKE_CURRENT_SOURCE_DIR}/FlightGear-resources.xml
    COMMENT
      "Re-generating embedded resources"
)

# set automatically by add_custom_command? 
#set_source_files_properties(${EMBEDDED_RESOURCE_SOURCES} PROPERTIES GENERATED TRUE)
#set_source_files_properties(${EMBEDDED_RESOURCE_HEADERS} PROPERTIES GENERATED TRUE)

add_library(fgembeddedresources STATIC 
  ${CMAKE_CURRENT_BINARY_DIR}/FlightGear-resources.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/FlightGear-resources.hxx
  )

# users of the library include this as <EmbeddedResources/FlightGear-resources.hxx>
target_include_directories(fgembeddedresources INTERFACE ${PROJECT_BINARY_DIR}/src)

target_link_libraries(fgembeddedresources SimGearCore)

# don't try to Automoc these files, it confuses the dependency logic
set_property(TARGET fgembeddedresources PROPERTY AUTOMOC OFF)

