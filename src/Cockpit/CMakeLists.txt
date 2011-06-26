include(FlightGearComponent)

set(SOURCES
	panel.cxx
	panel_io.cxx
	built_in/FGMagRibbon.cxx
	)
	
set(HEADERS
	panel.hxx
	panel_io.hxx
	built_in/FGMagRibbon.hxx
	)


flightgear_component(Cockpit "${SOURCES}" "${HEADERS}")