include(FlightGearComponent)

set(SOURCES
	FGEventHandler.cxx
	WindowBuilder.cxx
	WindowSystemAdapter.cxx
	fg_os_osgviewer.cxx
	fgviewer.cxx
        ViewPropertyEvaluator.cxx
	splash.cxx
	view.cxx
	viewmgr.cxx
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
	)

if (Qt5Core_FOUND)
    list(APPEND HEADERS GraphicsWindowQt5.hxx 
					    OSGQtAdaption.hxx)
    list(APPEND SOURCES GraphicsWindowQt5.cpp 
						OSGQtAdaption.cxx)
endif()

if (YES)
    list(APPEND HEADERS PUICamera.hxx)
    list(APPEND SOURCES PUICamera.cxx)
endif()

if (ENABLE_COMPOSITOR)
  list(APPEND SOURCES CameraGroup_compositor.cxx renderer_compositor.cxx)
  list(APPEND HEADERS CameraGroup_compositor.hxx renderer_compositor.hxx)
else ()
  list(APPEND SOURCES CameraGroup_legacy.cxx renderer_legacy.cxx renderingpipeline.cxx)
  list(APPEND HEADERS CameraGroup_legacy.hxx renderer_legacy.hxx renderingpipeline.hxx)
endif()

flightgear_component(Viewer "${SOURCES}" "${HEADERS}")
