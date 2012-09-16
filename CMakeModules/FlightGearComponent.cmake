
macro(flightgear_component name sources)
    set(fc ${name})
    set(fh ${name})
    foreach(s ${sources})
        set_property(GLOBAL
            APPEND PROPERTY FG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${s}")
        set(fc "${fc}#${CMAKE_CURRENT_SOURCE_DIR}/${s}")
    endforeach()

    foreach(h ${ARGV2})
        set_property(GLOBAL
            APPEND PROPERTY FG_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${h}")
        set(fh "${fh}#${CMAKE_CURRENT_SOURCE_DIR}/${h}")
    endforeach()

    set_property(GLOBAL APPEND PROPERTY FG_GROUPS_C "${fc}@")
    set_property(GLOBAL APPEND PROPERTY FG_GROUPS_H "${fh}@")
endmacro()
