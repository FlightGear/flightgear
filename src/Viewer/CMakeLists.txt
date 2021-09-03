include(FlightGearComponent)

set(SOURCES
	CameraGroup.cxx
	FGEventHandler.cxx
	WindowBuilder.cxx
	WindowSystemAdapter.cxx
	fg_os_osgviewer.cxx
	fgviewer.cxx
    ViewPropertyEvaluator.cxx
	renderer.cxx
	splash.cxx
	view.cxx
	viewmgr.cxx
	sview.cxx
	GraphicsPresets.cxx
	)

set(HEADERS
	CameraGroup.hxx
	FGEventHandler.hxx
	WindowBuilder.hxx
	WindowSystemAdapter.hxx
	fgviewer.hxx
	renderer.hxx
	splash.hxx
	view.hxx
	viewmgr.hxx
	sview.hxx
	GraphicsPresets.hxx
	VRManager.hxx
	)

if (ENABLE_OSGXR)
	list(APPEND SOURCES
		VRManager.cxx
		)
endif()

if (YES)
    list(APPEND HEADERS PUICamera.hxx)
    list(APPEND SOURCES PUICamera.cxx)
endif()

flightgear_component(Viewer "${SOURCES}" "${HEADERS}")
