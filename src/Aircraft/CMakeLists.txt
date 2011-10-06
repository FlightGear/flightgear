include(FlightGearComponent)

set(SOURCES
	controls.cxx
	replay.cxx
	flightrecorder.cxx
	)

set(HEADERS
	controls.hxx
	replay.hxx
	flightrecorder.hxx
	)


flightgear_component(Aircraft "${SOURCES}" "${HEADERS}")
