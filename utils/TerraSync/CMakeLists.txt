
if(LIBSVN_FOUND)
	message(STATUS "includes '${LIBSVN_INCLUDE_DIRS}'")
	include_directories(${LIBSVN_INCLUDE_DIRS})
	add_definitions(${APR_CFLAGS})
endif(LIBSVN_FOUND)
	
add_executable(terrasync terrasync.cxx)

target_link_libraries(terrasync 
	${SIMGEAR_LIBRARIES}
	${ZLIB_LIBRARIES}
    ${WINSOCK_LIBRARY})

if(LIBSVN_FOUND)
	target_link_libraries(terrasync ${LIBSVN_LIBRARIES})
endif()


install(TARGETS terrasync RUNTIME DESTINATION bin)
