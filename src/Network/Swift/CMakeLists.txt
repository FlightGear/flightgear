include(FlightGearComponent)

set(SOURCES
	swift_connection.cxx
	dbusconnection.cpp
	dbusobject.cpp
	dbusmessage.cpp
	dbusdispatcher.cpp
	dbuserror.cpp
	dbusserver.cpp
	plugin.cpp
	service.cpp
	traffic.cpp
	SwiftAircraftManager.cpp
	)

set(HEADERS
	swift_connection.hxx
	dbusconnection.h
	dbusobject.h
	dbusmessage.h
	dbuscallbacks.h
	dbusdispatcher.h
	dbuserror.h
	dbusserver.h
	plugin.h
	service.h
	traffic.h
	SwiftAircraftManager.h
	)
    	
flightgear_component(Swift "${SOURCES}" "${HEADERS}")
