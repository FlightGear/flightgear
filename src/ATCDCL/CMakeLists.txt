include(FlightGearComponent)

set(SOURCES
	ATC.cxx
	atis.cxx
	ATCVoice.cxx
	ATISmgr.cxx
	ATCutils.cxx
	ATCProjection.cxx
	)

set(HEADERS
	ATC.hxx
	atis.hxx
	ATCVoice.hxx
	ATISmgr.hxx
	ATCutils.hxx
	ATCProjection.hxx
	atis_lexicon.hxx
	atis_remap.hxx
	)
    	
flightgear_component(ATCDCL "${SOURCES}" "${HEADERS}")
