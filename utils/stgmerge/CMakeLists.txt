add_executable(stgmerge stgmerge.cxx)

target_link_libraries(stgmerge
	SimGearScene
	${OPENSCENEGRAPH_LIBRARIES}
	${OPENGL_LIBRARIES}
)

install(TARGETS stgmerge RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
