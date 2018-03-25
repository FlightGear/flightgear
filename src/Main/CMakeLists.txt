# CMake module includes.
include(SetupFGFSLibraries)

if(MSVC)
    set(RESOURCE_FILE flightgear.rc)
endif(MSVC)

set(SOURCES
    bootstrap.cxx
    fg_commands.cxx
    fg_init.cxx
    fg_io.cxx
    fg_os_common.cxx
    fg_scene_commands.cxx
    fg_props.cxx
    FGInterpolator.cxx
    globals.cxx
    locale.cxx
    logger.cxx
    main.cxx
    options.cxx
    positioninit.cxx
    screensaver_control.cxx
    subsystemFactory.cxx
    util.cxx
    ${RESOURCE_FILE}
    ${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.cxx
)

set(HEADERS
    AircraftDirVisitorBase.hxx
    fg_commands.hxx
    fg_init.hxx
    fg_io.hxx
    fg_props.hxx
    FGInterpolator.hxx
    globals.hxx
    locale.hxx
    logger.hxx
    main.hxx
    options.hxx
    positioninit.hxx
    screensaver_control.hxx
    subsystemFactory.hxx
    util.hxx
    ${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.hxx
)

# On Windows, make sure fgrcc can be run (it needs third-party libraries)
if(MSVC)
  if(MSVC_3RDPARTY_ROOT AND MSVC_3RDPARTY_DIR)
    set(CMAKE_MSVCIDE_RUN_PATH ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/bin)
  else()
    message(FATAL_ERROR
      "Either MSVC_3RDPARTY_ROOT or MSVC_3RDPARTY_DIR is empty or unset")
  endif()
endif()

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.cxx
         ${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.hxx
  COMMAND fgrcc --root=${CMAKE_SOURCE_DIR} --output-cpp-file=${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.cxx --init-func-name=initFlightGearEmbeddedResources --output-header-file=${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.hxx --output-header-identifier=_FG_FLIGHTGEAR_EMBEDDED_RESOURCES ${CMAKE_SOURCE_DIR}/src/EmbeddedResources/FlightGear-resources.xml
  DEPENDS
    fgrcc ${CMAKE_SOURCE_DIR}/src/EmbeddedResources/FlightGear-resources.xml
)

get_property(FG_SOURCES GLOBAL PROPERTY FG_SOURCES)
get_property(FG_HEADERS GLOBAL PROPERTY FG_HEADERS)

get_property(FG_GROUPS_C GLOBAL PROPERTY FG_GROUPS_C)
string(REPLACE "@" ";" groups ${FG_GROUPS_C} )
foreach(g ${groups})
    string(REPLACE "#" ";" g2 ${g})
    list(GET g2 0 name)
    list(REMOVE_AT g2 0)
    source_group("${name}\\Sources" FILES ${g2})
endforeach()

get_property(FG_GROUPS_H GLOBAL PROPERTY FG_GROUPS_H)
string(REPLACE "@" ";" groups ${FG_GROUPS_H} )
foreach(g ${groups})
    string(REPLACE "#" ";" g2 ${g})
    list(GET g2 0 name)
    list(REMOVE_AT g2 0)
    source_group("${name}\\Headers" FILES ${g2})
endforeach()

source_group("Main\\Headers" FILES ${HEADERS})
source_group("Main\\Sources" FILES ${SOURCES})

# important we pass WIN32 here so the console is optional. Other
# platforms ignore this option. If a console is needed we allocate
# it manually via AllocConsole()
# similarly pass MACOSX_BUNDLE so we generate a .app on Mac
add_executable(fgfs WIN32 MACOSX_BUNDLE
               ${SOURCES} ${FG_SOURCES} ${FG_HEADERS} ${HEADERS})

#-----------------------------------------------------------------------------
# MacOSX bundle packaging

if(APPLE)
    execute_process(COMMAND date +%Y
        OUTPUT_VARIABLE CURRENT_YEAR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # in our local CMakeModules dir
    set_target_properties(fgfs PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST FlightGearBundleInfo.plist.in
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.flightgear.FlightGear"
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${FLIGHTGEAR_VERSION}
        MACOSX_BUNDLE_LONG_VERSION_STRING "FlightGear ${FLIGHTGEAR_VERSION} Nightly"
        MACOSX_BUNDLE_BUNDLE_VERSION ${FLIGHTGEAR_VERSION}
        MACOSX_BUNDLE_COPYRIGHT "FlightGear ${FLIGHTGEAR_VERSION} © 1997-${CURRENT_YEAR}, The FlightGear Project. Licensed under the GNU Public License version 2."
        MACOSX_BUNDLE_INFO_STRING "Nightly build of FlightGear ${FLIGHTGEAR_VERSION} for testing and development"
        MACOSX_BUNDLE_BUNDLE_NAME "FlightGear"
        MACOSX_BUNDLE_ICON_FILE "FlightGear.icns"
    )
endif()

# Set up the target links and includes.
setup_fgfs_libraries(fgfs)

if (APPLE)
    install(TARGETS fgfs BUNDLE DESTINATION .)
else()
    install(TARGETS fgfs RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(ENABLE_METAR)
    add_executable(metar metar_main.cxx)
    target_link_libraries(metar
        SimGearScene SimGearCore
        ${PLATFORM_LIBS}
    )

    install(TARGETS metar RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if (COMMAND flightgear_test)
    flightgear_test(autosaveMigration test_autosaveMigration.cxx)
endif()
