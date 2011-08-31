
	
add_executable(terrasync terrasync.cxx)

target_link_libraries(terrasync 
	${SIMGEAR_LIBRARIES}
  ${OPENTHREADS_LIBRARIES}
	${ZLIB_LIBRARIES}
    ${WINSOCK_LIBRARY})

if(LIBSVN_FOUND)
	target_link_libraries(terrasync ${LIBSVN_LIBRARIES})
	set_property(TARGET terrasync APPEND PROPERTY COMPILE_FLAGS "${APR_CFLAGS}")
	
	IF(APPLE)
	    set_property(SOURCE terrasync.cxx PROPERTY COMPILE_FLAGS "-iwithsysroot ${LIBSVN_INCLUDE_DIR}")
	ELSE()
	    include_directories(${LIBSVN_INCLUDE_DIR})
	ENDIF(APPLE)
endif()


install(TARGETS terrasync RUNTIME DESTINATION bin)
