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

if (ENABLE_HID_INPUT)
	list(APPEND EVENT_INPUT_SOURCES FGHIDEventInput.cxx)
	list(APPEND EVENT_INPUT_HEADERS FGHIDEventInput.hxx)	
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
        SimGearCore
        ${PLIB_LIBRARIES}
    )

    install(TARGETS fgjs RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(ENABLE_JS_DEMO)
    add_executable(js_demo js_demo.cxx)
    target_link_libraries(js_demo
        SimGearCore
        ${PLIB_LIBRARIES}
    )

    install(TARGETS js_demo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if (COMMAND flightgear_test)
	set(HID_INPUT_TEST_SOURCES test_hidinput.cxx FGEventInput.cxx 
		FGCommonInput.cxx FGDEviceConfigurationMap.cxx)

	flightgear_test(hidinput "${HID_INPUT_TEST_SOURCES}")
	target_link_libraries(hidinput ${EVENT_INPUT_LIBRARIES} hidapi)	
endif()

flightgear_component(Input "${SOURCES}" "${HEADERS}")
