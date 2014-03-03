include(FlightGearComponent)

set(SOURCES
	cJSON.c
	)

set(HEADERS
	cJSON.h
	)

flightgear_component(CJSON "${SOURCES}" "${HEADERS}")
