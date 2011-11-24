include(FlightGearComponent)

set(SOURCES
	radio.cxx
	itm.cpp
	)

set(HEADERS
	radio.hxx
	)

	
flightgear_component(Radio "${SOURCES}" "${HEADERS}")

