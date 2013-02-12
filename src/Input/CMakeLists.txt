include(FlightGearComponent)

IF(APPLE)
	set(EVENT_INPUT_SOURCES FGMacOSXEventInput.cxx)
	set(EVENT_INPUT_HEADERS FGMacOSXEventInput.hxx)
elseif(MSVC)
	message(STATUS "EventInput not implemented for Windows yet")
else()
	set(EVENT_INPUT_SOURCES FGLinuxEventInput.cxx)
	set(EVENT_INPUT_HEADERS FGLinuxEventInput.hxx)
endif()


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

set(HEADERS
	FGButton.hxx
	FGCommonInput.hxx
	FGDeviceConfigurationMap.hxx
	FGEventInput.hxx
	FGJoystickInput.hxx
	FGKeyboardInput.hxx
	FGMouseInput.hxx
	input.hxx
	)

if(EVENT_INPUT)
	list(APPEND SOURCES ${EVENT_INPUT_SOURCES})
	list(APPEND SOURCES ${EVENT_INPUT_HEADERS})
	include_directories(${UDEV_INCLUDE_DIR})
endif()

if(ENABLE_FGJS)
    set(FGJS_SOURCES
        fgjs.cxx
        jsinput.cxx
        jssuper.cxx
    )

    add_executable(fgjs ${FGJS_SOURCES})
    target_link_libraries(fgjs
        ${SIMGEAR_CORE_LIBRARIES}
        ${PLIB_LIBRARIES}
        ${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
    )

    install(TARGETS fgjs RUNTIME DESTINATION bin)
endif()

if(ENABLE_JS_DEMO)
    add_executable(js_demo js_demo.cxx)
    target_link_libraries(js_demo
        ${SIMGEAR_CORE_LIBRARIES}
        ${PLIB_LIBRARIES}
        ${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
    )

    install(TARGETS js_demo RUNTIME DESTINATION bin)
endif()

flightgear_component(Input "${SOURCES}" "${HEADERS}")