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

    # set the flag for CMake policy 00071, ensure Qt AUTOfoo don't process
    # generated files
    foreach(sourcefile IN LISTS ${SOURCES} ${HEADERS})
      set_property(SOURCE ${sourcefile} PROPERTY SKIP_AUTOMOC ON)
      set_property(SOURCE ${sourcefile} PROPERTY SKIP_AUTOUIC ON)
    endforeach()

    # On Windows, make sure fgrcc can be run (it needs third-party libraries)
    if(MSVC)
      if(MSVC_3RDPARTY_ROOT AND MSVC_3RDPARTY_DIR)
        set(CMAKE_MSVCIDE_RUN_PATH ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/bin PARENT_SCOPE)
      else()
        message(FATAL_ERROR
          "Either MSVC_3RDPARTY_ROOT or MSVC_3RDPARTY_DIR is empty or unset")
      endif()
    endif()
endfunction()
