add_executable(demconvert demconvert.cxx )

target_link_libraries(demconvert
    ${SIMGEAR_LIBRARIES}
    ${OPENSCENEGRAPH_LIBRARIES}
    ${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
)

install(TARGETS demconvert RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
