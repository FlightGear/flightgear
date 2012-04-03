include(FlightGearComponent)

set(SOURCES
	antenna.cxx
	radio.cxx
	)

set(HEADERS
	antenna.hxx
	radio.hxx
	)

	
flightgear_component(Radio "${SOURCES}" "${HEADERS}")

