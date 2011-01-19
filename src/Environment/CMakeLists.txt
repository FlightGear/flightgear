include(FlightGearComponent)

set(SOURCES
	atmosphere.cxx
	environment.cxx
	environment_ctrl.cxx
	environment_mgr.cxx
	ephemeris.cxx
	fgclouds.cxx
	fgmetar.cxx
	fgwind.cxx
	metarairportfilter.cxx
	metarproperties.cxx
	precipitation_mgr.cxx
	realwx_ctrl.cxx
	ridge_lift.cxx
	terrainsampler.cxx
	presets.cxx
	)
	
flightgear_component(Environment "${SOURCES}")
