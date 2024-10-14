

# placehodler target for other ones to depend upon
add_custom_target(
    debug_symbols
)

function(export_debug_symbols target)

    if (APPLE)
        add_custom_target(${target}.dSYM
            COMMENT "Generating dSYM files for ${target}"
            COMMAND dsymutil --out=${CMAKE_BINARY_DIR}/symbols/${target}.dSYM $<TARGET_FILE:${target}>
            DEPENDS $<TARGET_FILE:${target}>
        ) 

        install(DIRECTORY ${CMAKE_BINARY_DIR}/symbols/${target}.dSYM DESTINATION symbols OPTIONAL)

        add_dependencies(debug_symbols ${target}.dSYM)
    endif()

    if (MSVC)
        set_property(TARGET ${target} PROPERTY PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/symbols)
    endif()
endfunction()


file(TO_NATIVE_PATH "${FINAL_MSVC_3RDPARTY_DIR}/bin" _msvc_3rdparty_bin_dir)
set(CMAKE_MSVCIDE_RUN_PATH "${_msvc_3rdparty_bin_dir}")

add_custom_target(upload_debug_symbols
    COMMENT "Uploading debug symbols via sentry-cli"
    COMMAND sentry-cli upload-dif --org flightgear --project flightgear ${CMAKE_BINARY_DIR}/symbols
) 


add_dependencies(upload_debug_symbols debug_symbols)

