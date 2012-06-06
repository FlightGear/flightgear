include(FlightGearComponent)

set(SOURCES
  canvas.cxx
  canvas_mgr.cxx
  elements/element.cxx
  elements/group.cxx
  elements/path.cxx
  elements/text.cxx
  property_helper.cxx
)

set(HEADERS
  canvas.hxx
  canvas_mgr.hxx
  elements/element.hxx
  elements/group.hxx
  elements/path.hxx
  elements/text.hxx
  property_helper.hxx
)

flightgear_component(Canvas "${SOURCES}" "${HEADERS}")

add_subdirectory(ShivaVG/src)