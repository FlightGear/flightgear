
if (MSVC)
	if (CMAKE_CL_64)
		set( RESOURCE_FILE flightgear64.rc )
	else (CMAKE_CL_64)
		set( RESOURCE_FILE flightgear.rc )
	endif (CMAKE_CL_64)
endif (MSVC)

set(SOURCES
	CameraGroup.cxx
	FGEventHandler.cxx
	WindowBuilder.cxx
	WindowSystemAdapter.cxx
	bootstrap.cxx
	fg_commands.cxx
	fg_init.cxx
	fg_io.cxx
	fg_os_common.cxx
	fg_os_osgviewer.cxx
	fg_props.cxx
	fgviewer.cxx
	globals.cxx
	logger.cxx
	main.cxx
	options.cxx
	renderer.cxx
	splash.cxx
	util.cxx
	viewer.cxx
	viewmgr.cxx
	${RESOURCE_FILE}
	)

set(HEADERS
	CameraGroup.hxx
	FGEventHandler.hxx
	WindowBuilder.hxx
	WindowSystemAdapter.hxx
	fg_commands.hxx
	fg_init.hxx
	fg_io.hxx
	fg_props.hxx
	fgviewer.hxx
	globals.hxx
	logger.hxx
	main.hxx
	options.hxx
	renderer.hxx
	splash.hxx
	util.hxx
	viewer.hxx
	viewmgr.hxx
	)

add_executable(fgfs ${SOURCES} ${HEADERS})

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

target_link_libraries(fgfs
	${FG_LIBS}
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

