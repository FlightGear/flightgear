
# this is the timestampe of when we ran Cmake, we could 
# switch to parsing this from flightgear/version file explicity,
# but it's only needed by AppStream metainfo for the moment

string(TIMESTAMP FG_CMAKE_TIMESTAMP "%Y-%m-%d")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # our .desktop file specifies --launcher, so only install it for
    # a build with the launcher enabled.
    if (ENABLE_QT)
        install(FILES org.flightgear.FlightGear.desktop DESTINATION share/applications)
    endif()

    # generate a metainfo file corresponding to this release
    configure_file (
        "${CMAKE_CURRENT_SOURCE_DIR}/org.flightgear.FlightGear.metainfo.in"
        "${CMAKE_CURRENT_BINARY_DIR}/org.flightgear.FlightGear.metainfo.xml"
    )

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.flightgear.FlightGear.metainfo.xml" DESTINATION share/metainfo)

endif()
