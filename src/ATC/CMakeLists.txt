include(FlightGearComponent)

set(SOURCES
        AirportGroundRadar.cxx
	atc_mgr.cxx
	trafficcontrol.cxx
	CommStation.cxx
        ATISEncoder.cxx
        MetarPropertiesATISInformationProvider.cxx
        CurrentWeatherATISInformationProvider.cxx
        ATCController.cxx
        ApproachController.cxx
        EnRouteController.cxx
        GroundController.cxx
        StartupController.cxx
        TowerController.cxx
	)

set(HEADERS
        AirportGroundRadar.hxx
	atc_mgr.hxx
	trafficcontrol.hxx
	CommStation.hxx
        ATISEncoder.hxx
        MetarPropertiesATISInformationProvider.hxx
        CurrentWeatherATISInformationProvider.hxx
        ATCController.hxx
        ApproachController.hxx
        EnRouteController.hxx
        GroundController.hxx
        StartupController.hxx
        TowerController.hxx
        QuadTree.hxx
	)
    	
flightgear_component(ATC "${SOURCES}" "${HEADERS}")
