include(FlightGearComponent)

set(SOURCES
    cockpitDisplayManager.cxx
	panel.cxx
	panel_io.cxx
	built_in/FGMagRibbon.cxx
    agradar.cxx
    groundradar.cxx
    od_gauge.cxx
    render_area_2d.cxx
    wxradar.cxx
    NavDisplay.cxx
	)
	
set(HEADERS
    cockpitDisplayManager.hxx
	panel.hxx
	panel_io.hxx
	built_in/FGMagRibbon.hxx
    agradar.hxx
    groundradar.hxx
    od_gauge.hxx
    render_area_2d.hxx
    wxradar.hxx
    NavDisplay.hxx
	)


flightgear_component(Cockpit "${SOURCES}" "${HEADERS}")