function(setup_fgfs_includes target)
    if(ENABLE_JSBSIM)
        # FIXME - remove once JSBSim doesn't expose private headers
        target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/src/FDM/JSBSim)
    endif()

    target_include_directories(${target} PRIVATE ${PLIB_INCLUDE_DIR})
    target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/3rdparty/cjson)
    # only actually needed for httpd.cxx
    target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/3rdparty/mongoose)

endfunction()
