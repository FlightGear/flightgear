
add_executable(fgadmin fgadmin.cxx fgadmin_funcs.cxx main.cxx untarka.c)

target_link_libraries(fgadmin 
	${SIMGEAR_LIBRARIES}
	${ZLIB_LIBRARIES}
	${PLIB_LIBRARIES}
	${FLTK_LIBRARIES}
)
	
install(TARGETS fgadmin RUNTIME DESTINATION bin)
