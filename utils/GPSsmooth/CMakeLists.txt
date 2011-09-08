
add_executable(GPSsmooth gps.cxx gps_main.cxx)
add_executable(MIDGsmooth MIDG-II.cxx MIDG_main.cxx)
add_executable(UGsmooth UGear.cxx UGear_command.cxx UGear_main.cxx UGear_telnet.cxx)

target_link_libraries(GPSsmooth
	${SIMGEAR_CORE_LIBRARIES}
	${PLIB_SG_LIBRARY}
	${PLIB_UL_LIBRARY}
	${ZLIB_LIBRARIES}
	${WINMM_LIBRARY}
	${WINSOCK_LIBRARY}
	${ZLIB_LIBRARIES}
	${RT_LIBRARY}
)

target_link_libraries(MIDGsmooth
	${SIMGEAR_CORE_LIBRARIES}
	${PLIB_SG_LIBRARY}
	${PLIB_UL_LIBRARY}
	${WINMM_LIBRARY}
	${WINSOCK_LIBRARY}
	${RT_LIBRARY}
)

target_link_libraries(UGsmooth
	${SIMGEAR_CORE_LIBRARIES}
	${PLIB_SG_LIBRARY}
	${PLIB_UL_LIBRARY}
	${WINMM_LIBRARY}
	${WINSOCK_LIBRARY}
	${ZLIB_LIBRARIES}
	${RT_LIBRARY}
)

install(TARGETS GPSsmooth MIDGsmooth UGsmooth RUNTIME DESTINATION bin)
