

# placehodler target for other ones to depend upon
if (NOT TARGET debug_symbols)
    add_custom_target(debug_symbols)
endif()

function(export_debug_symbols target)

    if (APPLE)
        # workaround because we adjust the bundle name in some cases, and
        # TARGET_FILE seems to have a bug of assuming the executable name matches
        if (isBundle) 
            add_custom_target(${target}.dSYM
                COMMENT "Generating dSYM files for ${target}"
                COMMAND dsymutil --out=${CMAKE_BINARY_DIR}/symbols/${target}.dSYM $<TARGET_BUNDLE_CONTENT_DIR:${target}>/MacOS/$<TARGET_NAME:${target}>
                DEPENDS ${target}
            ) 
        else()
            add_custom_target(${target}.dSYM
                COMMENT "Generating dSYM files for ${target}"
                COMMAND dsymutil --out=${CMAKE_BINARY_DIR}/symbols/${target}.dSYM $<TARGET_FILE:${target}>
                DEPENDS ${target}
            ) 
        endif()

        install(DIRECTORY ${CMAKE_BINARY_DIR}/symbols/${target}.dSYM 
            DESTINATION symbols OPTIONAL
            COMPONENT symbols EXCLUDE_FROM_ALL)

        add_dependencies(debug_symbols ${target}.dSYM)
    endif()

    if (MSVC)
        set_property(TARGET ${target} PROPERTY PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/symbols)
        install(FILES $<TARGET_PDB_FILE:${target}>
            DESTINATION symbols OPTIONAL
            COMPONENT symbols EXCLUDE_FROM_ALL)
    endif()
endfunction()


file(TO_NATIVE_PATH "${FINAL_MSVC_3RDPARTY_DIR}/bin" _msvc_3rdparty_bin_dir)
set(CMAKE_MSVCIDE_RUN_PATH "${_msvc_3rdparty_bin_dir}")

if (FG_BUILD_TYPE STREQUAL "Release")
    add_custom_target(upload_debug_symbols
        COMMENT "Uploading debug symbols via sentry-cli"
        COMMAND sentry-cli upload-dif --org flightgear --project flightgear ${CMAKE_BINARY_DIR}/symbols
    ) 

    add_dependencies(upload_debug_symbols debug_symbols)
else()
    # make a dummy target to keep CI simple, but it doesn't do anything
    add_custom_target(upload_debug_symbols
        COMMENT "Non-release FG_BUILD_TYPE: symbol upload is disabled"
    ) 
endif()


