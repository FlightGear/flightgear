include(FlightGearComponent)

set(SOURCES
	atmosphere.cxx
	environment.cxx
	environment_ctrl.cxx
	environment_mgr.cxx
	ephemeris.cxx
        climate.cxx
	fgclouds.cxx
	fgmetar.cxx
	metarairportfilter.cxx
	metarproperties.cxx
	precipitation_mgr.cxx
	realwx_ctrl.cxx
	ridge_lift.cxx
	terrainsampler.cxx
	presets.cxx
	gravity.cxx
        magvarmanager.cxx
	)

set(HEADERS
	atmosphere.hxx
	environment.hxx
	environment_ctrl.hxx
	environment_mgr.hxx
	ephemeris.hxx
	fgclouds.hxx
        climate.hxx
	fgmetar.hxx
	metarairportfilter.hxx
	metarproperties.hxx
	precipitation_mgr.hxx
	realwx_ctrl.hxx
	ridge_lift.hxx
	terrainsampler.hxx
	presets.hxx
	gravity.hxx
        magvarmanager.hxx
	)
    		
flightgear_component(Environment "${SOURCES}" "${HEADERS}")
