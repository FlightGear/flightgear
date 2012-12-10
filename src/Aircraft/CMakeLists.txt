include(FlightGearComponent)

set(SOURCES
	controls.cxx
	replay.cxx
	flightrecorder.cxx
    FlightHistory.cxx
	)

set(HEADERS
	controls.hxx
	replay.hxx
	flightrecorder.hxx
    FlightHistory.hxx
	)


flightgear_component(Aircraft "${SOURCES}" "${HEADERS}")
