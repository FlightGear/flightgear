include(FlightGearComponent)

set(SOURCES
	controls.cxx
	replay.cxx
	flightrecorder.cxx
    FlightHistory.cxx
		initialstate.cxx
	AircraftPerformance.cxx
	)

set(HEADERS
	controls.hxx
	replay.hxx
	flightrecorder.hxx
    FlightHistory.hxx
		initialstate.hxx
	AircraftPerformance.hxx
	)


flightgear_component(Aircraft "${SOURCES}" "${HEADERS}")
