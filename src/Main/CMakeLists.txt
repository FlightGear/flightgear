
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
	)

get_property(FG_SOURCES GLOBAL PROPERTY FG_SOURCES)
get_property(FG_HEADERS GLOBAL PROPERTY FG_HEADERS)

link_directories ( ${Boost_LIBRARY_DIRS} )

add_executable(fgfs ${SOURCES} ${FG_SOURCES} ${FG_HEADERS} ${HEADERS})

get_property(FG_LIBS GLOBAL PROPERTY FG_LIBS)
#message(STATUS "fg libs ${FG_LIBS}")
#message(STATUS "OSG libs ${OPENSCENEGRAPH_LIBRARIES}")
#message(STATUS "SG libs ${SIMGEAR_LIBRARIES}")

if(RTI_FOUND)
	find_sg_component(hla SIMGEAR_LIBRARIES)
	find_sg_component(rti13 SIMGEAR_LIBRARIES)
	find_sg_component(rti SIMGEAR_LIBRARIES)
	set(HLA_LIBRARIES ${RTI_LIBRARIES})
else()
	set(HLA_LIBRARIES "")
endif()

if(ENABLE_JSBSIM)
    # FIXME - remove once JSBSim doesn't expose private headers
    include_directories(${PROJECT_SOURCE_DIR}/src/FDM/JSBSim)
    
    target_link_libraries(fgfs JSBSim)
endif()

include_directories(${PROJECT_SOURCE_DIR}/src/Canvas/ShivaVG/include)
add_definitions(-DVG_API_EXPORT)

target_link_libraries(fgfs
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
	ShivaVG
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
