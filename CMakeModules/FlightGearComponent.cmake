
macro(flightgear_component name sources)

	set(libName "fg${name}")
	add_library(${libName} STATIC ${sources} )

	set_property(GLOBAL APPEND PROPERTY FG_LIBS ${libName})
    
endmacro()
