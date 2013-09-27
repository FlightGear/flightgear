# 20130904 - build of fgcom standalone - geoff
if(ENABLE_IAX)
    # set data installed location
    if (UNIX)
     if (APPLE)
      set(PKGDATAPATH "fgcom-data")
     else ()
      set(PKGDATAPATH 
        "${CMAKE_INSTALL_PREFIX}/share/flightgear")
     endif ()
    else ()
      set(PKGDATAPATH "fgcom-data")
    endif ()

    # then to define the files themselves, in their 'installed' 
    # location, not their source location...
    if (NOT DEFAULT_POSITIONS_FILE)
      set(DEFAULT_POSITIONS_FILE "${PKGDATAPATH}/positions.txt")
    endif ()
    if (NOT SPECIAL_FREQUENCIES_FILE)
      set(SPECIAL_FREQUENCIES_FILE "${PKGDATAPATH}/special_frequencies.txt")
    endif ()
    # pass these to the compiler
    add_definitions( -DDEFAULT_POSITIONS_FILE="${DEFAULT_POSITIONS_FILE}" )
    add_definitions( -DSPECIAL_FREQUENCIES_FILE="${SPECIAL_FREQUENCIES_FILE}" )

    # Project fgcom, type Console Application
    set(name fgcom)
    set( ${name}_SOURCES fgcom.cxx fgcom_init.cxx position.cxx utils.cxx )
    set( ${name}_HEADERS fgcom.hxx fgcom_init.hxx position.hxx utils.hxx )
    if(WIN32)
        list(APPEND ${name}_SOURCES fgcom_getopt.c)
        list(APPEND ${name}_HEADERS fgcom_getopt.h)
    endif()
    add_executable( ${name} ${${name}_SOURCES} ${${name}_HEADERS} )
    if(WIN32)
        set_target_properties( ${name} PROPERTIES DEBUG_POSTFIX d )
    endif()
    target_link_libraries( ${name} iaxclient_lib )
    # this could be just on a target basis, but for now
    include_directories( ${CMAKE_SOURCE_DIR}/utils/iaxclient/lib ) # for iaxclient.h
    # Now include simgear libraries 
    target_link_Libraries( ${name}
        ${win_LIBS}
        ${PLIB_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${OPENAL_LIBRARY}
        ${SIMGEAR_CORE_LIBRARIES}
        ${SIMGEAR_CORE_LIBRARY_DEPENDENCIES} )
    # deal with install
    install(TARGETS ${name} RUNTIME DESTINATION bin)
    # then install, from their source to install destination
    set( inst_FILES utils/positions.txt
        utils/special_frequencies.txt )
    if (UNIX)
     if (APPLE)
      install(FILES ${inst_FILES} DESTINATION bin/${PKGDATAPATH})
     else ()
      install(FILES ${inst_FILES} DESTINATION ${PKGDATAPATH})
     endif ()
    else ()
     install(FILES ${inst_FILES} DESTINATION bin/${PKGDATAPATH})
    endif ()
endif()

# eof
