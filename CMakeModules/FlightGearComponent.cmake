
function(flightgear_component name sources)
    foreach(s ${sources})
        target_sources(fgfsObjects PRIVATE ${s})
    endforeach()

    foreach(h ${ARGV2})
        target_sources(fgfsObjects PRIVATE ${h})
    endforeach()

    # third argument is TEST_SOURCES
    foreach(t ${ARGV3})
        set_property(GLOBAL
            APPEND PROPERTY FG_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${t}")
    endforeach()
endfunction()
