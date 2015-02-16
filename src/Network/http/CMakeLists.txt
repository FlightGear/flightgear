include(FlightGearComponent)

set(SOURCES
	httpd.cxx
	ScreenshotUriHandler.cxx
	PropertyUriHandler.cxx
	JsonUriHandler.cxx
	PkgUriHandler.cxx
	RunUriHandler.cxx
	NavdbUriHandler.cxx
	PropertyChangeWebsocket.cxx
	PropertyChangeObserver.cxx
	jsonprops.cxx
	)

set(HEADERS
	urihandler.hxx
	httpd.hxx
	ScreenshotUriHandler.hxx
	PropertyUriHandler.hxx
	JsonUriHandler.hxx
	PkgUriHandler.hxx
	RunUriHandler.hxx
	NavdbUriHandler.hxx
	HTTPRequest.hxx
	Websocket.hxx
	PropertyChangeWebsocket.hxx
	PropertyChangeObserver.hxx
	jsonprops.hxx
	)

flightgear_component(Http "${SOURCES}" "${HEADERS}")
