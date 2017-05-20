if(ENABLE_QT AND ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    install(FILES org.flightgear.FlightGear.desktop DESTINATION share/applications)
endif(ENABLE_QT AND ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
