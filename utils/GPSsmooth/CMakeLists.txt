
check_library_exists(rt clock_gettime "" HAVE_CLOCK_GETTIME)

add_executable(GPSsmooth gps.cxx gps.hxx gps_main.cxx)
add_executable(MIDGsmooth MIDG-II.cxx MIDG-II.hxx MIDG_main.cxx)
add_executable(UGsmooth 
    UGear.cxx UGear.hxx
    UGear_command.cxx UGear_command.hxx
    UGear_main.cxx 
    UGear_telnet.cxx
    UGear_telnet.hxx)

if(HAVE_CLOCK_GETTIME)
	target_link_libraries(GPSsmooth rt)
endif(HAVE_CLOCK_GETTIME)

target_link_libraries(GPSsmooth
	${SIMGEAR_CORE_LIBRARIES}
	${PLIB_SG_LIBRARY}
	${PLIB_UL_LIBRARY}
	${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
	${ZLIB_LIBRARY}
	${WINMM_LIBRARY}
)

if(HAVE_CLOCK_GETTIME)
	target_link_libraries(MIDGsmooth rt)
endif(HAVE_CLOCK_GETTIME)

target_link_libraries(MIDGsmooth
	${SIMGEAR_CORE_LIBRARIES}
	${PLIB_SG_LIBRARY}
	${PLIB_UL_LIBRARY}
	${WINMM_LIBRARY}
	${ZLIB_LIBRARY}
	${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
)

if(HAVE_CLOCK_GETTIME)
	target_link_libraries(UGsmooth rt)
endif(HAVE_CLOCK_GETTIME)

target_link_libraries(UGsmooth
	${SIMGEAR_CORE_LIBRARIES}
	${PLIB_SG_LIBRARY}
	${PLIB_UL_LIBRARY}
	${WINMM_LIBRARY}
	${ZLIB_LIBRARY}
	${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
)

install(TARGETS GPSsmooth MIDGsmooth UGsmooth RUNTIME DESTINATION bin)
