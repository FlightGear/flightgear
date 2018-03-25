# CMake module includes.
include(SetupFGFSEmbeddedResources)
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
)

# Set up the embedded resources.
setup_fgfs_embedded_resources()

# All sources and headers to be built into fgfs.
get_property(FG_SOURCES GLOBAL PROPERTY FG_SOURCES)
get_property(FG_HEADERS GLOBAL PROPERTY FG_HEADERS)
get_property(EMBEDDED_RESOURCE_SOURCES GLOBAL PROPERTY EMBEDDED_RESOURCE_SOURCES)
get_property(EMBEDDED_RESOURCE_HEADERS GLOBAL PROPERTY EMBEDDED_RESOURCE_HEADERS)

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
add_executable(fgfs
    WIN32
    MACOSX_BUNDLE
    ${SOURCES}
    ${HEADERS}
    ${FG_SOURCES}
    ${FG_HEADERS}
    ${EMBEDDED_RESOURCE_SOURCES}
    ${EMBEDDED_RESOURCE_HEADERS}
)

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
