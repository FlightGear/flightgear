function(setup_fgfs_embedded_resources)
    # The source and header files.
    set(SOURCES
        ${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.cxx
    )
    set(HEADERS
        ${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.hxx
    )
    set_property(GLOBAL APPEND PROPERTY EMBEDDED_RESOURCE_SOURCES ${SOURCES})
    set_property(GLOBAL APPEND PROPERTY EMBEDDED_RESOURCE_HEADERS ${HEADERS})

    # On Windows, make sure fgrcc can be run (it needs third-party libraries)
    if(MSVC)
      if(MSVC_3RDPARTY_ROOT AND MSVC_3RDPARTY_DIR)
        set(CMAKE_MSVCIDE_RUN_PATH ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/bin)
      else()
        message(FATAL_ERROR
          "Either MSVC_3RDPARTY_ROOT or MSVC_3RDPARTY_DIR is empty or unset")
      endif()
    endif()

    add_custom_command(
      OUTPUT ${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.cxx
             ${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.hxx
      COMMAND fgrcc --root=${CMAKE_SOURCE_DIR} --output-cpp-file=${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.cxx --init-func-name=initFlightGearEmbeddedResources --output-header-file=${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.hxx --output-header-identifier=_FG_FLIGHTGEAR_EMBEDDED_RESOURCES ${CMAKE_SOURCE_DIR}/src/EmbeddedResources/FlightGear-resources.xml
      DEPENDS
        fgrcc ${CMAKE_SOURCE_DIR}/src/EmbeddedResources/FlightGear-resources.xml
    )
endfunction()
