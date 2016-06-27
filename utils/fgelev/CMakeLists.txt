add_executable(fgelev fgelev.cxx)

target_link_libraries(fgelev
	${SIMGEAR_LIBRARIES}
	${OPENSCENEGRAPH_LIBRARIES}
	${OPENGL_LIBRARIES}
        ${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
        ${ZLIB_LIBRARY}
)

install(TARGETS fgelev RUNTIME DESTINATION bin)
