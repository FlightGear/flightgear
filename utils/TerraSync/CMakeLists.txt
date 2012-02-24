
add_executable(terrasync terrasync.cxx)

target_link_libraries(terrasync
	${SIMGEAR_CORE_LIBRARIES}
	${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
)

install(TARGETS terrasync RUNTIME DESTINATION bin)
