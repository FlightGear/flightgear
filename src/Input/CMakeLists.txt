include(FlightGearComponent)

set(EVENT_INPUT_SOURCES
	FGLinuxEventInput.cxx
	FGMacOSXEventInput.cxx
	)

set (JS_INPUT_SOURCES
	js.cxx
	jsBSD.cxx
	jsLinux.cxx
	jsMacOSX.cxx
	jsWindows.cxx
	jsNone.cxx
	)

set(SOURCES
	FGButton.cxx
	FGCommonInput.cxx
	FGDeviceConfigurationMap.cxx
	FGEventInput.cxx
	FGJoystickInput.cxx
	FGKeyboardInput.cxx
	FGMouseInput.cxx
	input.cxx
	)

#
# we still have a dependency on ul from plib!

if(EVENT_INPUT)
	list(APPEND SOURCES ${EVENT_INPUT_SOURCES})
else()
	list(APPEND SOURCES ${JS_INPUT_SOURCES})
endif()

set(FGJS_SOURCES
	fgjs.cxx
	jsinput.cxx
	jssuper.cxx
	)
	
add_executable(fgjs ${FGJS_SOURCES})

if(WIN32)
   set(SOCKETS_LIBRARY wsock32.lib)
endif(WIN32)

target_link_libraries(fgjs 
    fgInput
    ${SOCKETS_LIBRARY}
	${SIMGEAR_LIBRARIES}
	${PLIB_LIBRARIES}
	${JS_LIBS}
	${ZLIB_LIBRARY})

add_executable(js_demo js_demo.cxx )

target_link_libraries(js_demo
	fgInput
	${JS_LIBS}
	${PLIB_LIBRARIES}
	)

flightgear_component(Input "${SOURCES}")

install(TARGETS fgjs js_demo RUNTIME DESTINATION bin)
