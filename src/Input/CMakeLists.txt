include(FlightGearComponent)

IF(APPLE)
	# no Mac implemention, use HID
elseif(WIN32)
	# no Win32 specific implementation, at least for now
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
	FGKeyboardInput.cxx
	FGMouseInput.cxx
	input.cxx
	)

set(HEADERS
	FGButton.hxx
	FGCommonInput.hxx
	FGDeviceConfigurationMap.hxx
	FGEventInput.hxx
	FGKeyboardInput.hxx
	FGMouseInput.hxx
	input.hxx
	)

if (ENABLE_PLIB_JOYSTICK)
	list(APPEND SOURCES FGJoystickInput.cxx)
	list(APPEND HEADERS FGJoystickInput.hxx)
endif()

if(EVENT_INPUT)
	list(APPEND SOURCES ${EVENT_INPUT_SOURCES})
	list(APPEND HEADERS ${EVENT_INPUT_HEADERS})
	include_directories(${UDEV_INCLUDE_DIR})
endif()

if(ENABLE_FGJS AND ENABLE_PLIB_JOYSTICK)
    set(FGJS_SOURCES
        fgjs.cxx
        jsinput.cxx
        jssuper.cxx
    )

    add_executable(fgjs ${FGJS_SOURCES})
    target_link_libraries(fgjs
		SimGearCore
		PLIBJoystick
    )

    install(TARGETS fgjs RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(ENABLE_JS_DEMO AND ENABLE_PLIB_JOYSTICK)
    add_executable(js_demo js_demo.cxx)
    target_link_libraries(js_demo
        SimGearCore
        PLIBJoystick
    )

    install(TARGETS js_demo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if (ENABLE_HID_INPUT)
	if (COMMAND flightgear_test)
		set(HID_INPUT_TEST_SOURCES test_hidinput.cxx FGEventInput.cxx 
			FGCommonInput.cxx FGDeviceConfigurationMap.cxx)

		flightgear_test(hidinput "${HID_INPUT_TEST_SOURCES}")
		target_link_libraries(hidinput ${EVENT_INPUT_LIBRARIES} hidapi)	
	endif()
endif()

flightgear_component(Input "${SOURCES}" "${HEADERS}")
