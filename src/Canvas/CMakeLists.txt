include(FlightGearComponent)

set(SOURCES
  canvas.cxx
  canvas_mgr.cxx
  elements/element.cxx
  elements/group.cxx
  elements/map.cxx
  elements/path.cxx
  elements/text.cxx
  elements/CanvasImage.cxx
  gui_mgr.cxx
  placement.cxx
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
  elements/CanvasImage.hxx
  gui_mgr.hxx
  placement.hxx
  property_helper.hxx
  window.hxx
)

flightgear_component(Canvas "${SOURCES}" "${HEADERS}")

add_subdirectory(ShivaVG/src)