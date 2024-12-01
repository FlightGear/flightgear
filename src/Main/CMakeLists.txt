# CMake module includes.
include(FlightGearComponent)
include(SetupFGFSBundle)
include(SetupFGFSEmbeddedResources)
include(SetupFGFSIncludes)
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
    XLIFFParser.cxx
    ErrorReporter.cxx
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
    XLIFFParser.hxx
    ErrorReporter.hxx
    sentryIntegration.hxx
)

flightgear_component(Main "${SOURCES}" "${HEADERS}" sentryIntegration.cxx)

# the main() function
set(MAIN_SOURCE
    bootstrap.cxx
    # we only want this file when building FGFS, so it can't be
    # part of fgfsObjects
    ${PROJECT_SOURCE_DIR}/src/Scripting/NasalUnitTesting.cxx
)

# Set up the embedded resources.
setup_fgfs_embedded_resources()

# Sort the sources and headers for MSVC.
setup_msvc_grouping()

# souerces which are different for fgfs vs the test-suite
get_property(TEST_SOURCES GLOBAL PROPERTY FG_TEST_SOURCES)


# important we pass WIN32 here so the console is optional. Other
# platforms ignore this option. If a console is needed we allocate
# it manually via AllocConsole()
# similarly pass MACOSX_BUNDLE so we generate a .app on Mac
add_executable(fgfs
    WIN32
    MACOSX_BUNDLE
    ${MAIN_SOURCE}
    ${TEST_SOURCES}
    $<TARGET_OBJECTS:fgfsObjects>
)

add_dependencies(fgfs buildId)
# explicitly disable automoc for main fgfs target
set_property(TARGET fgfs PROPERTY AUTOMOC OFF)

# MacOSX bundle packaging
if(APPLE)
    setup_fgfs_bundle(fgfs)
endif()

# Set up the target links.
setup_fgfs_libraries(fgfs)
export_debug_symbols(fgfs)
# Additional search paths for includes.
setup_fgfs_includes(fgfs)

add_executable(nasal
    nasal-bin.cxx
    ${PROJECT_SOURCE_DIR}/src/Scripting/sqlitelib.cxx
)
setup_fgfs_libraries(nasal)
install(TARGETS nasal RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(ENABLE_METAR)
    add_executable(metar metar_main.cxx)
    target_link_libraries(metar SimGearScene)
    install(TARGETS metar RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# ensure run/debug paths are set automatically in Visual Studio
if (MSVC)
  file(TO_NATIVE_PATH "${FG_QT_BIN_DIR}" _qt_bin_dir_native)
  file(TO_NATIVE_PATH "${FINAL_MSVC_3RDPARTY_DIR}/bin" _msvc_3rdparty_bin_dir)
  file(TO_NATIVE_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}" _install_bin_dir)

  set_property(TARGET fgfs PROPERTY
  VS_GLOBAL_LocalDebuggerEnvironment "PATH=${_install_bin_dir};${_msvc_3rdparty_bin_dir};${_qt_bin_dir_native}")
endif()
