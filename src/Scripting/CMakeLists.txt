include(FlightGearComponent)

set(SOURCES
  NasalSys.cxx
  nasal-props.cxx
  NasalAddons.cxx
  NasalAircraft.cxx
  NasalPositioned.cxx
  NasalPositioned_cppbind.cxx
  NasalCanvas.cxx
  NasalClipboard.cxx
  NasalCondition.cxx
  NasalHTTP.cxx
  NasalString.cxx
  NasalModelData.cxx
  NasalSGPath.cxx
  NasalUnitTesting.cxx
  NasalFlightPlan.cxx
)

set(HEADERS
  NasalSys.hxx
  NasalSys_private.hxx
  NasalAddons.hxx
  NasalAircraft.hxx
  NasalPositioned.hxx
  NasalCanvas.hxx
  NasalClipboard.hxx
  NasalCondition.hxx
  NasalHTTP.hxx
  NasalString.hxx
  NasalModelData.hxx
  NasalSGPath.hxx
  NasalFlightPlan.hxx
)

if(WIN32)
  list(APPEND SOURCES ClipboardWindows.cxx)
elseif(APPLE)
    list(APPEND SOURCES ClipboardCocoa.mm)
else()
  find_package(X11)
  if(X11_FOUND)
    list(APPEND SOURCES ClipboardX11.cxx)
  else()
    list(APPEND SOURCES ClipboardFallback.cxx)
  endif()
endif()

flightgear_component(Scripting "${SOURCES}" "${HEADERS}")

