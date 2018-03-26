# CMake module includes.
include(FlightGearComponent)
include(SetupFGFSBundle)
include(SetupFGFSEmbeddedResources)
include(SetupFGFSLibraries)
include(SetupMSVCGrouping)

# Set up the Main FG file sources and headers (excluding bootstrap.cxx and its main() function).
if(MSVC)
    set(MS_RESOURCE_FILE flightgear.rc)
endif(MSVC)

set(SOURCES
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
    ${MS_RESOURCE_FILE}
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

flightgear_component(Main "${SOURCES}" "${HEADERS}")

# The main() function.
set(MAIN_SOURCE
    bootstrap.cxx
)

# Set up the embedded resources.
setup_fgfs_embedded_resources()

# Sort the sources and headers for MSVC.
setup_msvc_grouping()

# All sources and headers to be built into fgfs.
get_property(FG_SOURCES GLOBAL PROPERTY FG_SOURCES)
get_property(FG_HEADERS GLOBAL PROPERTY FG_HEADERS)
get_property(EMBEDDED_RESOURCE_SOURCES GLOBAL PROPERTY EMBEDDED_RESOURCE_SOURCES)
get_property(EMBEDDED_RESOURCE_HEADERS GLOBAL PROPERTY EMBEDDED_RESOURCE_HEADERS)

# important we pass WIN32 here so the console is optional. Other
# platforms ignore this option. If a console is needed we allocate
# it manually via AllocConsole()
# similarly pass MACOSX_BUNDLE so we generate a .app on Mac
add_executable(fgfs
    WIN32
    MACOSX_BUNDLE
    ${MAIN_SOURCE}
    ${FG_SOURCES}
    ${FG_HEADERS}
    ${EMBEDDED_RESOURCE_SOURCES}
    ${EMBEDDED_RESOURCE_HEADERS}
)

# MacOSX bundle packaging
if(APPLE)
    setup_fgfs_bundle(fgfs)
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
