include(FlightGearComponent)

set(SOURCES
	CameraGroup.cxx
	FGEventHandler.cxx
	WindowBuilder.cxx
	WindowSystemAdapter.cxx
	fg_os_osgviewer.cxx
	fgviewer.cxx
	renderer.cxx
	renderingpipeline.cxx
	splash.cxx
	viewer.cxx
	viewmgr.cxx
	)

set(HEADERS
	CameraGroup.hxx
	FGEventHandler.hxx
	WindowBuilder.hxx
	WindowSystemAdapter.hxx
	fgviewer.hxx
	renderer.hxx
	renderingpipeline.hxx
	splash.hxx
	viewer.hxx
	viewmgr.hxx
	)

flightgear_component(Viewer "${SOURCES}" "${HEADERS}")
