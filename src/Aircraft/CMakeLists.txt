include(FlightGearComponent)

set(SOURCES
	controls.cxx
	replay.cxx
	)

set(HEADERS
	controls.hxx
	replay.hxx
	)


flightgear_component(Aircraft "${SOURCES}" "${HEADERS}")