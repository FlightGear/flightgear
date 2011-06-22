include(FlightGearComponent)

set(SOURCES
	airways.cxx
	awynet.cxx
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

flightgear_component(Navaids "${SOURCES}")