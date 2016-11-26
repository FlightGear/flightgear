include(FlightGearComponent)

set(SOURCES
	httpd.cxx
	ScreenshotUriHandler.cxx
	PropertyUriHandler.cxx
	JsonUriHandler.cxx
    FlightHistoryUriHandler.cxx
	PkgUriHandler.cxx
	RunUriHandler.cxx
	MirrorPropertyTreeWebsocket.cxx
	NavdbUriHandler.cxx
	PropertyChangeWebsocket.cxx
	PropertyChangeObserver.cxx
	jsonprops.cxx
	SimpleDOM.cxx
	)

set(HEADERS
	urihandler.hxx
	httpd.hxx
	ScreenshotUriHandler.hxx
	PropertyUriHandler.hxx
	JsonUriHandler.hxx
    FlightHistoryUriHandler.hxx
	PkgUriHandler.hxx
	RunUriHandler.hxx
	NavdbUriHandler.hxx
	HTTPRequest.hxx
	Websocket.hxx
	PropertyChangeWebsocket.hxx
	PropertyChangeObserver.hxx
	MirrorPropertyTreeWebsocket.hxx
	jsonprops.hxx
    SimpleDOM.hxx
	)

flightgear_component(Http "${SOURCES}" "${HEADERS}")
