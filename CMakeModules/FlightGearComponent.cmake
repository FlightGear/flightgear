
macro(flightgear_component name sources)
    foreach(s ${sources})
        set_property(GLOBAL
            APPEND PROPERTY FG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${s}")
    endforeach()

	foreach(h ${ARGV2})
		set_property(GLOBAL
			APPEND PROPERTY FG_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${h}")
	endforeach()
endmacro()
