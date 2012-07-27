include(FlightGearComponent)

set(SOURCES
  canvas.cxx
  canvas_mgr.cxx
  elements/element.cxx
  elements/group.cxx
  elements/map.cxx
  elements/path.cxx
  elements/text.cxx
  gui_mgr.cxx
  placement.cxx
  property_based_element.cxx
  property_based_mgr.cxx
  property_helper.cxx
  window.cxx
)

set(HEADERS
  canvas.hxx
  canvas_mgr.hxx
  elements/element.hxx
  elements/group.hxx
  elements/map.hxx
  elements/path.hxx
  elements/text.hxx
  gui_mgr.hxx
  placement.hxx
  property_based_element.hxx
  property_based_mgr.hxx
  property_helper.hxx
  window.hxx
)

flightgear_component(Canvas "${SOURCES}" "${HEADERS}")

add_subdirectory(ShivaVG/src)