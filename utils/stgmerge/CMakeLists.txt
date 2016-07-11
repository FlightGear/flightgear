add_executable(stgmerge stgmerge.cxx)

target_link_libraries(stgmerge
	${SIMGEAR_LIBRARIES}
	${OPENSCENEGRAPH_LIBRARIES}
	${OPENGL_LIBRARIES}
        ${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
)

install(TARGETS stgmerge RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
