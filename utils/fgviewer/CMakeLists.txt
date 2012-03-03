
add_executable(fgviewer fgviewer.cxx)

target_link_libraries(fgviewer
	${SIMGEAR_LIBRARIES}
	${OPENSCENEGRAPH_LIBRARIES}
	${OPENGL_LIBRARIES}
        ${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
)

install(TARGETS fgviewer RUNTIME DESTINATION bin)
