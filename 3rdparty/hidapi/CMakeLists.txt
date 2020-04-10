

set(HIDAPI_SOURCES
    hidapi/hidapi.h
    hidapi/hidparse.h
    hidparser/hidparse.c
)

if (WIN32)
    list(APPEND HIDAPI_SOURCES windows/hid.c)
elseif(APPLE)
    list(APPEND HIDAPI_SOURCES mac/hid.c)
else()
    list(APPEND HIDAPI_SOURCES linux/hid.c)
endif(WIN32)


#add_definitions( -DHAVE_CONFIG_H ) # to use fgfs config.h to get FG version, if needed
#add_definitions( -DLIBVER="SVN 261" ) # add an iaxclient_lib version string

add_library(hidapi STATIC
    ${HIDAPI_SOURCES}
)

if(WIN32)
    target_link_libraries(hidapi PUBLIC SetupApi)
elseif(APPLE)
    find_library(IOKIT_FRAMEWORK IOKit)
    target_link_libraries(hidapi ${IOKIT_FRAMEWORK})
else()
    # Linux-y things
    find_package(UDev QUIET)
    if (UDEV_FOUND)
        target_link_libraries(hidapi ${UDEV_LIBRARIES})
    else()
		message(FATAL_ERROR "Failed to find UDev")
	endif()
endif()

target_include_directories(hidapi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hidapi)
target_include_directories(hidapi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# eof
