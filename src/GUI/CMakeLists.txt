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
	
flightgear_component(GUI "${SOURCES}")
