add_executable(fgelev fgelev.cxx)

target_link_libraries(fgelev
	${SIMGEAR_LIBRARIES}
	${OPENSCENEGRAPH_LIBRARIES}
	${OPENGL_LIBRARIES}
        ${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
)

install(TARGETS fgelev RUNTIME DESTINATION bin)
