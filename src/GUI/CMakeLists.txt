include(FlightGearComponent)

set(SOURCES
	AirportList.cxx
	MapWidget.cxx
	WaypointList.cxx
	dialog.cxx
	FGPUIDialog.cxx
	fonts.cxx
	gui.cxx
	gui_funcs.cxx
	layout-props.cxx
	layout.cxx
	menubar.cxx
	FGPUIMenuBar.cxx
	new_gui.cxx
	property_list.cxx
	FGFontCache.cxx
	FGColor.cxx
	)

set(HEADERS
	AirportList.hxx
	MapWidget.hxx
	WaypointList.hxx
	dialog.hxx
	FGPUIDialog.hxx
	gui.h
	layout.hxx
	menubar.hxx
	FGPUIMenuBar.hxx
	new_gui.hxx
	property_list.hxx
	FGFontCache.hxx
	FGColor.hxx
	)
    		
if (APPLE)
    list(APPEND HEADERS FGCocoaMenuBar.hxx)
    list(APPEND SOURCES FGCocoaMenuBar.mm)
endif()
    		
flightgear_component(GUI "${SOURCES}" "${HEADERS}")
