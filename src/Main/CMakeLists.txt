
if (MSVC)
	if (CMAKE_CL_64)
		set( RESOURCE_FILE flightgear64.rc )
	else (CMAKE_CL_64)
		set( RESOURCE_FILE flightgear.rc )
	endif (CMAKE_CL_64)
endif (MSVC)

set(SOURCES
	bootstrap.cxx
	fg_commands.cxx
	fg_init.cxx
	fg_io.cxx
	fg_os_common.cxx
	fg_props.cxx
	globals.cxx
	locale.cxx
	logger.cxx
	main.cxx
	options.cxx
	util.cxx
    positioninit.cxx
    subsystemFactory.cxx
	${RESOURCE_FILE}
	)

set(HEADERS
	fg_commands.hxx
	fg_init.hxx
	fg_io.hxx
	fg_props.hxx
	globals.hxx
	locale.hxx
	logger.hxx
	main.hxx
	options.hxx
	util.hxx
    positioninit.hxx
    subsystemFactory.hxx
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
add_executable(fgfs ${SOURCES} ${FG_SOURCES} ${FG_HEADERS} ${HEADERS})

# disable sqlite3 dynamic lib support
# this should really be a SOURCE property, but the way we handle
# Fcomponent sources is making that tricky
add_definitions(-DSQLITE_OMIT_LOAD_EXTENSION)

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

if(GOOGLE_PERFTOOLS_FOUND)
    target_link_libraries(fgfs profiler)
endif()

target_link_libraries(fgfs
	${SQLITE3_LIBRARY}
	${SIMGEAR_LIBRARIES}
	${OPENSCENEGRAPH_LIBRARIES}
	${OPENGL_LIBRARIES}
	${PLIB_LIBRARIES}
	${JPEG_LIBRARY}
	${HLA_LIBRARIES}
	${EVENT_INPUT_LIBRARIES}
	${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
	${SIMGEAR_SCENE_LIBRARY_DEPENDENCIES}
	${PLATFORM_LIBS}
)

install(TARGETS fgfs RUNTIME DESTINATION bin)

add_executable(metar metar_main.cxx)

target_link_libraries(metar
	${SIMGEAR_LIBRARIES}
	${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
	${SIMGEAR_SCENE_LIBRARY_DEPENDENCIES}
	${PLATFORM_LIBS}
)

install(TARGETS metar RUNTIME DESTINATION bin)
