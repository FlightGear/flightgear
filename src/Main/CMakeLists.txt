
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
	)
	
add_executable(fgfs ${SOURCES})

get_property(FG_LIBS GLOBAL PROPERTY FG_LIBS)
message(STATUS "fg libs ${FG_LIBS}")
message(STATUS "OSG libs ${OPENSCENEGRAPH_LIBRARIES}")
message(STATUS "SG libs ${SIMGEAR_LIBRARIES}")

target_link_libraries(fgfs 
	${FG_LIBS}
	${SIMGEAR_LIBRARIES}
	${OPENSCENEGRAPH_LIBRARIES} 
	${OPENAL_LIBRARY} 
	${OPENGL_LIBRARIES}
	${ALUT_LIBRARY} 
	${ZLIB_LIBRARIES}
	${PLIB_LIBRARIES})
	