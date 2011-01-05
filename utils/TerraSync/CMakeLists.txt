
find_package(SvnClient)

add_executable(terrasync terrasync.cxx)

target_link_libraries(terrasync 
	${SIMGEAR_LIBRARIES}
	${ZLIB_LIBRARIES}
    ${WINSOCK_LIBRARY})

if(HAVE_SVN_CLIENT)
	target_link_libraries(terrasync ${SVN_CLIENT_LIBRARIES})
endif()


install(TARGETS terrasync RUNTIME DESTINATION bin)
