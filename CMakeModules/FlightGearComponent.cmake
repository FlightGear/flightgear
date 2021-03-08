
macro(flightgear_component name sources)
    set(fc ${name})
    set(fh ${name})
    foreach(s ${sources})
        set_property(GLOBAL
            APPEND PROPERTY FG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${s}")
        set(fc "${fc}#${CMAKE_CURRENT_SOURCE_DIR}/${s}")

        # becuase we can't require CMake 3.13, we can't use CMP0076
        # so we need to manually resolve relative paths into absolute paths
        target_sources(fgfsObjects PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${s})

    endforeach()

    foreach(h ${ARGV2})
        set_property(GLOBAL
            APPEND PROPERTY FG_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${h}")
        set(fh "${fh}#${CMAKE_CURRENT_SOURCE_DIR}/${h}")

        # becuase we can't require CMake 3.13, we can't use CMP0076
        # so we need to manually resolve relative paths into absolute paths
        target_sources(fgfsObjects PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${h})
    endforeach()

    # third argument is TEST_SOURCES
    foreach(t ${ARGV3})
        set_property(GLOBAL
            APPEND PROPERTY FG_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${t}")
        set(fc "${fh}#${CMAKE_CURRENT_SOURCE_DIR}/${t}")
    endforeach()

    set_property(GLOBAL APPEND PROPERTY FG_GROUPS_C "${fc}@")
    set_property(GLOBAL APPEND PROPERTY FG_GROUPS_H "${fh}@")

endmacro()
