

# placehodler target for other ones to depend upon
add_custom_target(
    debug_symbols
)

function(export_debug_symbols target)

    if (APPLE)
        add_custom_target(${target}.dSYM
            COMMENT "Generating dSYM files for ${target}"
            COMMAND dsymutil --out=${target}.dSYM $<TARGET_FILE:${target}>
            DEPENDS $<TARGET_FILE:${target}>
        ) 

        install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${target}.dSYM DESTINATION symbols OPTIONAL)

        add_dependencies(debug_symbols ${target}.dSYM)
    endif()

endfunction()