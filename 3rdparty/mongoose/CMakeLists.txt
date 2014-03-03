include(FlightGearComponent)

set(SOURCES
	mongoose.c
	)

set(HEADERS
	mongoose.h
	)

flightgear_component(Mongoose "${SOURCES}" "${HEADERS}")
