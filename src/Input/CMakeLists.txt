include(FlightGearComponent)

IF(APPLE)
	set(EVENT_INPUT_SOURCES FGMacOSXEventInput.cxx)
else()
	set(EVENT_INPUT_SOURCES FGLinuxEventInput.cxx)
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
	
if(EVENT_INPUT)
	list(APPEND SOURCES ${EVENT_INPUT_SOURCES})
	include_directories(${DBUS_INCLUDE_DIR} ${DBUS_ARCH_INCLUDE_DIR})
endif()
	
set(FGJS_SOURCES
	fgjs.cxx
	jsinput.cxx
	jssuper.cxx
	)
	
add_executable(fgjs ${FGJS_SOURCES})

target_link_libraries(fgjs 
    ${SIMGEAR_CORE_LIBRARIES}
    ${PLIB_LIBRARIES}
    ${SIMGEAR_CORE_LIBRARY_DEPENDENCIES})

add_executable(js_demo js_demo.cxx)

target_link_libraries(js_demo 
	${SIMGEAR_CORE_LIBRARIES}
	${PLIB_LIBRARIES}
	${SIMGEAR_CORE_LIBRARY_DEPENDENCIES})

flightgear_component(Input "${SOURCES}")

install(TARGETS fgjs js_demo RUNTIME DESTINATION bin)
