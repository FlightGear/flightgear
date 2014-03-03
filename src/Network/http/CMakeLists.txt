include(FlightGearComponent)

set(SOURCES
	httpd.cxx
	ScreenshotUriHandler.cxx
	PropertyUriHandler.cxx
	JsonUriHandler.cxx
	RunUriHandler.cxx
	)

set(HEADERS
	urihandler.hxx
	httpd.hxx
	ScreenshotUriHandler.hxx
	PropertyUriHandler.hxx
	JsonUriHandler.hxx
	RunUriHandler.hxx
	HTTPRequest.hxx
	)

flightgear_component(Http "${SOURCES}" "${HEADERS}")
