include(FlightGearComponent)

set(SOURCES
	ATC.cxx
	atis.cxx
	commlist.cxx
	ATCDialog.cxx
	ATCVoice.cxx
	ATCmgr.cxx
	ATCutils.cxx
	ATCProjection.cxx
	)
	
flightgear_component(ATCDCL "${SOURCES}")
