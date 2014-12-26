
if (MSVC)
	set( RESOURCE_FILE flightgear.rc )
endif (MSVC)

set(SOURCES
	bootstrap.cxx
	fg_commands.cxx
	fg_init.cxx
	fg_io.cxx
	fg_os_common.cxx
	fg_props.cxx
	FGInterpolator.cxx
	globals.cxx
	locale.cxx
	logger.cxx
	main.cxx
	options.cxx
	util.cxx
    positioninit.cxx
    subsystemFactory.cxx
    screensaver_control.cxx
	${RESOURCE_FILE}
	)

set(HEADERS
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
	util.hxx
    positioninit.hxx
    subsystemFactory.hxx
    AircraftDirVisitorBase.hxx
    screensaver_control.hxx
	)

get_property(FG_SOURCES GLOBAL PROPERTY FG_SOURCES)
get_property(FG_HEADERS GLOBAL PROPERTY FG_HEADERS)

link_directories ( ${Boost_LIBRARY_DIRS} )

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
# MacOSX bundle packagaing

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

#-----------------------------------------------------------------------------

get_property(FG_LIBS GLOBAL PROPERTY FG_LIBS)
#message(STATUS "fg libs ${FG_LIBS}")
#message(STATUS "OSG libs ${OPENSCENEGRAPH_LIBRARIES}")
#message(STATUS "SG libs ${SIMGEAR_LIBRARIES}")

if(RTI_FOUND)
	set(HLA_LIBRARIES ${RTI_LIBRARIES})
else()
	set(HLA_LIBRARIES "")
endif()

if(ENABLE_JSBSIM)
    # FIXME - remove once JSBSim doesn't expose private headers
    include_directories(${PROJECT_SOURCE_DIR}/src/FDM/JSBSim)
    
    target_link_libraries(fgfs JSBSim)
endif()
if(ENABLE_IAX)
    target_link_libraries(fgfs iaxclient_lib ${OPENAL_LIBRARY})
endif()
if(USE_DBUS)
    target_link_libraries(fgfs ${DBUS_LIBRARIES})
endif()
if(FG_HAVE_GPERFTOOLS)
    include_directories(${GooglePerfTools_INCLUDE_DIR})
    target_link_libraries(fgfs ${GooglePerfTools_LIBRARIES})
endif()

if (CRASHRPT_FOUND)
	target_link_libraries(fgfs ${CRASHRPT_LIBRARY})
endif()

target_link_libraries(fgfs
	${SQLITE3_LIBRARY}
	${SIMGEAR_LIBRARIES}
	${OPENSCENEGRAPH_LIBRARIES}
	${OPENGL_LIBRARIES}
	${PLIB_LIBRARIES}
	${HLA_LIBRARIES}
	${EVENT_INPUT_LIBRARIES}
	${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
	${SIMGEAR_SCENE_LIBRARY_DEPENDENCIES}
	${PLATFORM_LIBS}
)

if(ENABLE_FLITE)
    if(SYSTEM_HTS_ENGINE)
        target_link_libraries(fgfs flite_hts ${HTS_ENGINE_LIBRARIES})
    else()
        target_link_libraries(fgfs flite_hts hts_engine)
    endif()
endif()

if (Qt5Core_FOUND)
    target_link_libraries(fgfs Qt5::Widgets fglauncher)
endif()

if (APPLE)  
    install(TARGETS fgfs BUNDLE DESTINATION .)
else()
    install(TARGETS fgfs RUNTIME DESTINATION bin)
endif()

if(ENABLE_METAR)
    add_executable(metar metar_main.cxx)
    target_link_libraries(metar
        ${SIMGEAR_LIBRARIES}
        ${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
        ${SIMGEAR_SCENE_LIBRARY_DEPENDENCIES}
        ${PLATFORM_LIBS}
    )

    install(TARGETS metar RUNTIME DESTINATION bin)
endif()
