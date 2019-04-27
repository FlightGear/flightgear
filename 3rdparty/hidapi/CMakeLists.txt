
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

# only needed for Linux, but doesn't do any harm here
target_link_libraries(hidapi ${UDEV_LIBRARIES})

if(WIN32)
    find_library(SETUP_API_LIB Setupapi)
	if (NOT SETUP_API_LIB)
		message(WARNING "Failed to find Setupapi.lib")
	endif()

    target_link_libraries(hidapi ${SETUP_API_LIB})
endif()


if(APPLE)
    find_library(IOKIT_FRAMEWORK IOKit)
    target_link_libraries(hidapi ${IOKIT_FRAMEWORK})
endif()

target_include_directories(hidapi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hidapi)
target_include_directories(hidapi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# eof
