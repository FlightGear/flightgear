include(FlightGearComponent)

set(SOURCES
	soundgenerator.cxx
	beacon.cxx
	fg_fx.cxx
	morse.cxx
	sample_queue.cxx
	voice.cxx
	voiceplayer.cxx
	)

set(HEADERS
	soundgenerator.hxx
	beacon.hxx
	fg_fx.hxx
	morse.hxx
	sample_queue.hxx
	voice.hxx
	voiceplayer.hxx
	)
    		
flightgear_component(Sound "${SOURCES}" "${HEADERS}")
