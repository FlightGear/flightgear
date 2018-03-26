function(setup_fgfs_includes)
    if(ENABLE_JSBSIM)
        # FIXME - remove once JSBSim doesn't expose private headers
        include_directories(${PROJECT_SOURCE_DIR}/src/FDM/JSBSim)
    endif()

    if(FG_HAVE_GPERFTOOLS)
        include_directories(${GooglePerfTools_INCLUDE_DIR})
    endif()
endfunction()
