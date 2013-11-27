# 20130904 - build of fgcom standalone - geoff

    if (MSVC)
      set( RESOURCE_FILE fgcom.rc )
    endif (MSVC)

    if (NOT FGCOM_DATA_PATH)
        # use relative paths (for standalone and custom installation)
        set(FGCOM_DATA_PATH "${CMAKE_INSTALL_PREFIX}/share/flightgear")
        if (APPLE)
            # on Mac "share" directory doesn't exist
            # .txt files are moved in the Resources directory because of Mac convention
            # This movement is handled by the fgmeta.git/hudson_mac_package_release.rb line 139
            set(DEFAULT_POSITIONS_FILE "../Resources/positions.txt")
            set(SPECIAL_FREQUENCIES_FILE "../Resources/special_frequencies.txt")
        else (APPLE)
            set(DEFAULT_POSITIONS_FILE "../share/flightgear/positions.txt")
            set(SPECIAL_FREQUENCIES_FILE "../share/flightgear/special_frequencies.txt")
        endif (APPLE)
    else()
        # use absolute paths, useful for package creation (e.g -DFGCOM_DATA_PATH=/usr/share/fgcom)
        set(DEFAULT_POSITIONS_FILE "${FGCOM_DATA_PATH}/positions.txt")
        set(SPECIAL_FREQUENCIES_FILE "${FGCOM_DATA_PATH}/special_frequencies.txt")
    endif()

    # pass these to the compiler
    add_definitions( -DDEFAULT_POSITIONS_FILE="${DEFAULT_POSITIONS_FILE}" )
    add_definitions( -DSPECIAL_FREQUENCIES_FILE="${SPECIAL_FREQUENCIES_FILE}" )

    # Project fgcom, type Console Application
    set(name fgcom)
    set( ${name}_SOURCES fgcom.cxx fgcom_init.cxx position.cxx utils.cxx ${RESOURCE_FILE} )
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
    include_directories( ${CMAKE_SOURCE_DIR}/3rdparty/iaxclient/lib ) # for iaxclient.h
    # Now include simgear libraries 
    target_link_Libraries( ${name}
        ${OPENAL_LIBRARY}
        ${SIMGEAR_CORE_LIBRARIES}
        ${SIMGEAR_CORE_LIBRARY_DEPENDENCIES} )
    # deal with install
    install(TARGETS ${name} RUNTIME DESTINATION bin)
    # then install, from their source to install destination
    set( inst_FILES utils/positions.txt
        utils/special_frequencies.txt )
    install(FILES ${inst_FILES} DESTINATION ${FGCOM_DATA_PATH})
# eof
