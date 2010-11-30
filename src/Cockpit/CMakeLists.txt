include(FlightGearComponent)

set(SOURCES
	panel.cxx
	panel_io.cxx
	built_in/FGMagRibbon.cxx
	)
	
flightgear_component(Cockpit "${SOURCES}")