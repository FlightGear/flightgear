include(FlightGearComponent)

set(SOURCES
	apt_loader.cxx
	calc_loc.cxx
	dynamicloader.cxx
	dynamics.cxx
	gnnode.cxx
	groundnetwork.cxx
	parking.cxx
	pavement.cxx
	runwaybase.cxx
	runwayprefloader.cxx
	runwayprefs.cxx
	runways.cxx
	sidstar.cxx
	simple.cxx
	xmlloader.cxx
	)
	
flightgear_component(Airports "${SOURCES}")