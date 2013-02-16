include(FlightGearComponent)

set(SOURCES
	apt_loader.cxx
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

set(HEADERS
	apt_loader.hxx
	dynamicloader.hxx
	dynamics.hxx
	gnnode.hxx
	groundnetwork.hxx
	parking.hxx
	pavement.hxx
	runwaybase.hxx
	runwayprefloader.hxx
	runwayprefs.hxx
	runways.hxx
	sidstar.hxx
	simple.hxx
	xmlloader.hxx
	)
    		
flightgear_component(Airports "${SOURCES}" "${HEADERS}")