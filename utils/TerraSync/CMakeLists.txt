
if(LIBSVN_FOUND)
	include_directories(${LIBSVN_INCLUDE_DIR})
endif(LIBSVN_FOUND)
	
add_executable(terrasync terrasync.cxx)

target_link_libraries(terrasync 
	${SIMGEAR_LIBRARIES}
	${ZLIB_LIBRARIES}
    ${WINSOCK_LIBRARY})

if(LIBSVN_FOUND)
	target_link_libraries(terrasync ${LIBSVN_LIBRARIES})
	set_property(TARGET terrasync APPEND PROPERTY COMPILE_FLAGS ${APR_CFLAGS})
endif()


install(TARGETS terrasync RUNTIME DESTINATION bin)
