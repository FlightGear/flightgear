
add_executable(GPSsmooth
    gps.cxx gps.hxx
    gps_main.cxx
)

add_executable(MIDGsmooth
    MIDG-II.cxx MIDG-II.hxx
    MIDG_main.cxx
)

add_executable(UGsmooth
    UGear.cxx UGear.hxx
    UGear_command.cxx UGear_command.hxx
    UGear_telnet.cxx UGear_telnet.hxx
    UGear_main.cxx 
)

target_link_libraries(GPSsmooth
    ${SIMGEAR_CORE_LIBRARIES}
    ${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
    ${PLIB_SG_LIBRARY}
    ${PLIB_UL_LIBRARY}
)

target_link_libraries(MIDGsmooth
    ${SIMGEAR_CORE_LIBRARIES}
    ${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
    ${PLIB_SG_LIBRARY}
    ${PLIB_UL_LIBRARY}
)

target_link_libraries(UGsmooth
    ${SIMGEAR_CORE_LIBRARIES}
    ${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
    ${PLIB_SG_LIBRARY}
    ${PLIB_UL_LIBRARY}
    ${ZLIB_LIBRARY}
)

install(TARGETS GPSsmooth MIDGsmooth UGsmooth RUNTIME DESTINATION bin)
