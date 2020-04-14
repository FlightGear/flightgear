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

# Additional search paths for includes.
setup_fgfs_includes()

# All sources and headers to be built into fgfs.
get_property(FG_SOURCES GLOBAL PROPERTY FG_SOURCES)
get_property(FG_HEADERS GLOBAL PROPERTY FG_HEADERS)
get_property(EMBEDDED_RESOURCE_SOURCES GLOBAL PROPERTY EMBEDDED_RESOURCE_SOURCES)
get_property(EMBEDDED_RESOURCE_HEADERS GLOBAL PROPERTY EMBEDDED_RESOURCE_HEADERS)

set_source_files_properties(${EMBEDDED_RESOURCE_SOURCES} PROPERTIES GENERATED TRUE)
set_source_files_properties(${EMBEDDED_RESOURCE_HEADERS} PROPERTIES GENERATED TRUE)

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

add_dependencies(fgfs buildId embeddedresources)
# explicitly disable automoc for main fgfs target
set_property(TARGET fgfs PROPERTY AUTOMOC OFF)

# MacOSX bundle packaging
if(APPLE)
    setup_fgfs_bundle(fgfs)
endif()

# Set up the target links.
setup_fgfs_libraries(fgfs)
export_debug_symbols(fgfs)

if (APPLE)
    install(TARGETS fgfs BUNDLE DESTINATION .)
else()
    install(TARGETS fgfs RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if (TARGET sentry::sentry)
    target_link_libraries(fgfs sentry::sentry)
endif()

if (TARGET sentry_crashpad::handler)
    if (APPLE)
        # install inside the bundle
        install(FILES $<TARGET_FILE:sentry_crashpad::handler> DESTINATION fgfs.app/Contents/MacOS OPTIONAL)
    else()
        # install in the bin-dir, next to the application binary
        install(FILES $<TARGET_FILE:sentry_crashpad::handler> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
    endif()
endif()

if(ENABLE_METAR)
    add_executable(metar metar_main.cxx)
    target_link_libraries(metar
        SimGearScene SimGearCore
        ${PLATFORM_LIBS}
    )

    install(TARGETS metar RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if (MSVC)
  if (TARGET Qt5::qmake)
    # property is the full path to qmake.exe
    get_target_property(_qt5_qmake_path Qt5::qmake LOCATION)
    get_filename_component(_qt5_bin_dir ${_qt5_qmake_path} DIRECTORY)
    file(TO_NATIVE_PATH "${_qt5_bin_dir}" _qt5_bin_dir_native)
  endif()

  file(TO_NATIVE_PATH "${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/bin" _msvc_3rdparty_bin_dir)
  file(TO_NATIVE_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}" _install_bin_dir)

  set_property(TARGET fgfs PROPERTY
  VS_GLOBAL_LocalDebuggerEnvironment "PATH=${_install_bin_dir};${_msvc_3rdparty_bin_dir};${_qt5_bin_dir_native}")
endif()
