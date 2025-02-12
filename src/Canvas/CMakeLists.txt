include(FlightGearComponent)

set(SOURCES
  canvas_mgr.cxx
  FGCanvasSystemAdapter.cxx
  texture_replace.cxx
)

set(HEADERS
  canvas_mgr.hxx
  FGCanvasSystemAdapter.hxx
  gui_mgr.hxx
  texture_replace.hxx
)

# gui_mgr.cxx is compiled differently for testing
flightgear_component(Canvas "${SOURCES}" "${HEADERS}"  gui_mgr.cxx)
