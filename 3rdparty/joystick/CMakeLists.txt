message(STATUS "Enabling legacy joystick code (from PLIB)")

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

add_library(PLIBJoystick STATIC ${JS_SOURCES})

# for logging code at least, more in the future
target_link_libraries(PLIBJoystick SimGearCore)

if(APPLE)
    target_link_libraries(PLIBJoystick ${IOKIT_LIBRARY} ${CF_LIBRARY} )
elseif(WIN32)
    target_link_libraries(PLIBJoystick ${WINMM_LIBRARY})
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    # no libs here?
elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
	set_target_properties(PLIBJoystick PROPERTIES 
		COMPILE_DEFINITIONS "HAVE_USBHID_H")
	target_include_directories(PLIBJoystick PRIVATE ${USBHID_INCLUDE_DIR})
    target_link_libraries(PLIBJoystick ${USBHID_LIBRARY})
else()
    # other / none platform
endif()

target_include_directories(PLIBJoystick PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# eof
