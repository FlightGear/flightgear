include(FlightGearComponent)

set(SOURCES
	airways.cxx
	fixlist.cxx
	markerbeacon.cxx
	navdb.cxx
	navlist.cxx
	navrecord.cxx
	positioned.cxx
	procedure.cxx
	route.cxx
	routePath.cxx
	waypoint.cxx
	PositionedBinding.cxx
	)

set(HEADERS
    airways.hxx
	fixlist.hxx
	markerbeacon.hxx
	navdb.hxx
	navlist.hxx
	navrecord.hxx
	positioned.hxx
	procedure.hxx
	route.hxx
	routePath.hxx
	waypoint.hxx
	PositionedBinding.hxx
    )

flightgear_component(Navaids "${SOURCES}" "${HEADERS}")