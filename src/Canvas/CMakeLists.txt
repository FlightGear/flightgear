include(FlightGearComponent)

set(SOURCES
  canvas.cxx
  canvas_mgr.cxx
  elements/element.cxx
  elements/group.cxx
  elements/text.cxx
)

set(HEADERS
  canvas.hxx
  canvas_mgr.hxx
  elements/element.hxx
  elements/group.hxx
  elements/text.hxx
)
    
flightgear_component(Canvas "${SOURCES}" "${HEADERS}")
