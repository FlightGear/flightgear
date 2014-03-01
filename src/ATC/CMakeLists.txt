include(FlightGearComponent)

set(SOURCES
	atc_mgr.cxx
	atcdialog.cxx
	trafficcontrol.cxx
	CommStation.cxx
        ATISEncoder.cxx
        MetarPropertiesATISInformationProvider.cxx
        CurrentWeatherATISInformationProvider.cxx
	)

set(HEADERS
	atc_mgr.hxx
	atcdialog.hxx
	trafficcontrol.hxx
	CommStation.hxx
        ATISEncoder.hxx
        MetarPropertiesATISInformationProvider.hxx
        CurrentWeatherATISInformationProvider.hxx
	)
    	
flightgear_component(ATC "${SOURCES}" "${HEADERS}")
