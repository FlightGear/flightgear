

add_executable(terrasync terrasync.cxx)

target_link_libraries(terrasync
	${SIMGEAR_CORE_LIBRARIES}
	${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
)

if(LIBSVN_FOUND)
	target_link_libraries(terrasync ${LIBSVN_LIBRARIES})
	set_property(TARGET terrasync APPEND PROPERTY COMPILE_FLAGS "${APR_CFLAGS} -DNO_OPENSCENEGRAPH_INTERFACE")
endif()


install(TARGETS terrasync RUNTIME DESTINATION bin)
