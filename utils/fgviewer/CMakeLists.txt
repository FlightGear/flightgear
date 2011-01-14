
add_executable(fgviewer fgviewer.cxx)

target_link_libraries(fgviewer 
	${SIMGEAR_LIBRARIES}
	${OPENSCENEGRAPH_LIBRARIES} 
	${OPENGL_LIBRARIES}
	${ZLIB_LIBRARIES}
	${PLIB_LIBRARIES}
	${JS_LIBS})
	
install(TARGETS fgviewer RUNTIME DESTINATION bin)
