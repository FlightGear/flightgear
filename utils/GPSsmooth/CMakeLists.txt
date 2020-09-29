add_executable(GPSsmooth
    GPSsmooth.cxx GPSsmooth.hxx
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

target_include_directories(GPSsmooth PRIVATE ${PLIB_INCLUDE_DIR})
target_include_directories(MIDGsmooth PRIVATE ${PLIB_INCLUDE_DIR})
target_include_directories(UGsmooth PRIVATE ${PLIB_INCLUDE_DIR})

target_link_libraries(GPSsmooth
    SimGearCore
    ${PLIB_SG_LIBRARY}
    ${PLIB_UL_LIBRARY}
)

target_link_libraries(MIDGsmooth
    SimGearCore
    ${PLIB_SG_LIBRARY}
    ${PLIB_UL_LIBRARY}
)

target_link_libraries(UGsmooth
    SimGearCore
    ${PLIB_SG_LIBRARY}
    ${PLIB_UL_LIBRARY}
    ${ZLIB_LIBRARY}
)

install(TARGETS GPSsmooth MIDGsmooth UGsmooth RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
