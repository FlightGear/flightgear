include(FlightGearComponent)

set(SOURCES
	NasalSys.cxx
	nasal-props.cxx
    NasalPositioned.cxx
    NasalCanvas.cxx
    NasalClipboard.cxx
	)

set(HEADERS
	NasalSys.hxx
    NasalPositioned.hxx
    NasalCanvas.hxx
    NasalClipboard.hxx
	)

if(WIN32)
  list(APPEND SOURCES ClipboardWindows.cxx)
else()
  find_package(X11)
  if(X11_FOUND)
    list(APPEND SOURCES ClipboardX11.cxx)
  else()
    list(APPEND SOURCES ClipboardFallback.cxx)
  endif()
endif()

flightgear_component(Scripting "${SOURCES}" "${HEADERS}")

