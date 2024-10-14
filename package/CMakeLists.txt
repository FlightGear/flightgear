
# this is the timestampe of when we ran Cmake, we could 
# switch to parsing this from flightgear/version file explicity,
# but it's only needed by AppStream metainfo for the moment

string(TIMESTAMP FG_CMAKE_TIMESTAMP "%Y-%m-%d")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(DEKSTOP_FILE_VERSION "1.0")
    
    if (ENABLE_QT)
        set(DESKTOP_FILE_RUN_ARGS "--launcher")
    endif()

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/org.flightgear.FlightGear.desktop.in"
        "${CMAKE_CURRENT_BINARY_DIR}/org.flightgear.FlightGear.desktop"
    )

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.flightgear.FlightGear.desktop" 
    DESTINATION share/applications)

    # generate a metainfo file corresponding to this release
    configure_file (
        "${CMAKE_CURRENT_SOURCE_DIR}/org.flightgear.FlightGear.metainfo.xml.in"
        "${CMAKE_CURRENT_BINARY_DIR}/org.flightgear.FlightGear.metainfo.xml"
    )

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.flightgear.FlightGear.metainfo.xml" DESTINATION share/metainfo)

endif()
