include(FlightGearComponent)

set(SOURCES
	AirportList.cxx
	MapWidget.cxx
	SafeTexFont.cxx
	WaypointList.cxx
	dialog.cxx
	fonts.cxx
	gui.cxx
	gui_funcs.cxx
	layout-props.cxx
	layout.cxx
	menubar.cxx
	new_gui.cxx
	property_list.cxx
	)

set(HEADERS
	AirportList.hxx
	MapWidget.hxx
	SafeTexFont.hxx
	WaypointList.hxx
	dialog.hxx
	gui.h
	layout.hxx
	menubar.hxx
	new_gui.hxx
	property_list.hxx
	)
    		
flightgear_component(GUI "${SOURCES}" "${HEADERS}")
