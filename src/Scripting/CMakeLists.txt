include(FlightGearComponent)

set(SOURCES
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
  NasalFlightPlan.cxx
  sqlitelib.cxx
  # we don't add this here becuase we need to exclude it the testSuite
  # so it can't go nto fgfsObjects library
  #   NasalUnitTesting.cxx
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

    # PCH doesn't work for Objective-C++
    set_property(SOURCE ClipboardCocoa.mm
      TARGET_DIRECTORY fgfsObjects
      PROPERTY SKIP_PRECOMPILE_HEADERS ON)

else()
  find_package(X11)
  if(X11_FOUND)
    list(APPEND SOURCES ClipboardX11.cxx)
  else()
    list(APPEND SOURCES ClipboardFallback.cxx)
  endif()
endif()


# NasalSys.cxx is passed as TEST_SOURCES since we need to compile it
# seperately when building the test_suite

flightgear_component(Scripting "${SOURCES}" "${HEADERS}"  NasalSys.cxx)

