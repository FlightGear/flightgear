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

# change once the code below is optional (imported from PLIB)
set(ENABLE_PLIB_JOYSTICK 1)

if(APPLE)
	# resolve frameworks to full paths
		find_library(IOKIT_LIBRARY IOKit)
		find_library(CF_LIBRARY CoreFoundation)
		set(JS_LIBS ${IOKIT_LIBRARY} ${CF_LIBRARY})
		set(JS_SOURCES js.cxx jsMacOSX.cxx)
elseif(WIN32)
		set(WINMM_LIBRARY winmm)
		set(JS_LIBS ${WINMM_LIBRARY})
		set(JS_SOURCES js.cxx jsWindows.cxx)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
		set(JS_SOURCES js.cxx jsLinux.cxx)
elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
		find_library(USBHID_LIBRARY usbhid)
		# check_function_exists(hidinit)
		set(JS_LIBS ${USBHID_LIBRARY})
		set(JS_SOURCES js.cxx jsBSD.cxx)
else()
		message(WARNING "Unsupported platform for  Joystick")
		set(JS_SOURCES js.cxx jsNone.cxx)
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
	${JS_SOURCES}
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
	js.h
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

    add_executable(fgjs ${FGJS_SOURCES} ${JS_SOURCES})
    target_link_libraries(fgjs
        ${SIMGEAR_CORE_LIBRARIES}
				${JS_LIBS}
        ${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
    )

    install(TARGETS fgjs RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(ENABLE_JS_DEMO)
    add_executable(js_demo js_demo.cxx ${JS_SOURCES})
    target_link_libraries(js_demo
        ${SIMGEAR_CORE_LIBRARIES}
        ${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
				${JS_LIBS}
    )

    install(TARGETS js_demo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

flightgear_component(Input "${SOURCES}" "${HEADERS}")

set_property(GLOBAL PROPERTY JS_LIBS ${JS_LIBS})
