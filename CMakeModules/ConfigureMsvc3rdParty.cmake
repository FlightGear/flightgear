# ConfigureMsvc3rdParty.cmake - Configure 3rd Party Library Paths on Windows

if (MSVC)
    get_filename_component(PARENT_DIR ${PROJECT_BINARY_DIR} DIRECTORY)
    if (CMAKE_CL_64)
        SET(TEST_3RDPARTY_DIR "${PARENT_DIR}/3rdparty.x64")
    else (CMAKE_CL_64)
        SET(TEST_3RDPARTY_DIR "${PARENT_DIR}/3rdparty")
    endif (CMAKE_CL_64)
    if (EXISTS ${TEST_3RDPARTY_DIR})
        set(MSVC_3RDPARTY_ROOT ${PARENT_DIR} CACHE PATH "Location where the third-party dependencies are extracted")
    else (EXISTS ${TEST_3RDPARTY_DIR})
        set(MSVC_3RDPARTY_ROOT NOT_FOUND CACHE PATH "Location where the third-party dependencies are extracted")
    endif (EXISTS ${TEST_3RDPARTY_DIR})
    list(APPEND PLATFORM_LIBS "winmm.lib")
else (MSVC)
    set(MSVC_3RDPARTY_ROOT NOT_FOUND CACHE PATH "Location where the third-party dependencies are extracted")
endif (MSVC)

if (MSVC AND MSVC_3RDPARTY_ROOT)
    message(STATUS "3rdparty files located in ${MSVC_3RDPARTY_ROOT}")

    string(SUBSTRING ${MSVC_VERSION} 0 2 MSVC_VERSION_MAJOR)
    string(SUBSTRING ${MSVC_VERSION} 2 2 MSVC_VERSION_MINOR)

    set( OSG_MSVC "msvc" )
    if (${MSVC_VERSION_MAJOR} EQUAL "19")
        if (${MSVC_VERSION_MINOR} EQUAL "00")
            set( OSG_MSVC ${OSG_MSVC}140 )
        else ()
            set( OSG_MSVC ${OSG_MSVC}141 )
        endif ()
    elseif (${MSVC_VERSION_MAJOR} EQUAL "18")
        set( OSG_MSVC ${OSG_MSVC}120 )
    else ()
        message(FATAL_ERROR "Visual Studio 2013/15/17 is required")
    endif ()

    if (CMAKE_CL_64)
        set( OSG_MSVC ${OSG_MSVC}-64 )
        set( MSVC_3RDPARTY_DIR 3rdParty.x64 )
		    set( BOOST_LIB lib64 )
    else (CMAKE_CL_64)
        set( MSVC_3RDPARTY_DIR 3rdParty )
	    	set( BOOST_LIB lib )
    endif (CMAKE_CL_64)

    set (CMAKE_LIBRARY_PATH ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/lib ${MSVC_3RDPARTY_ROOT}/install/${OSG_MSVC}/OpenScenegraph/lib ${MSVC_3RDPARTY_ROOT}/install/${OSG_MSVC}/OpenRTI/lib ${MSVC_3RDPARTY_ROOT}/install/${OSG_MSVC}/SimGear/lib  )
    set (CMAKE_INCLUDE_PATH ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/include ${MSVC_3RDPARTY_ROOT}/install/${OSG_MSVC}/OpenScenegraph/include ${MSVC_3RDPARTY_ROOT}/install/${OSG_MSVC}/OpenRTI/include ${MSVC_3RDPARTY_ROOT}/install/${OSG_MSVC}/SimGear/include)

    # ensure 3rdparty/lib/cmake is searched
    list(APPEND CMAKE_PREFIX_PATH ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR})

    if(NOT BOOST_INCLUDEDIR)
        # if this variable was not set by the user, set it to 3rdparty root's
        # parent dir, which is the normal location for people using our
        # windows-3rd-party repo
        get_filename_component(MSVC_ROOT_PARENT_DIR ${MSVC_3RDPARTY_ROOT}
                               DIRECTORY)
        set(BOOST_INCLUDEDIR ${MSVC_ROOT_PARENT_DIR})
        message(STATUS "BOOST_INCLUDEDIR is ${BOOST_INCLUDEDIR}")
      endif()

    if (USE_AEONWAVE)
      find_package(AAX COMPONENTS aax REQUIRED)
    else()
      set (OPENAL_INCLUDE_DIR ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/include)
      set (OPENAL_LIBRARY_DIR ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/lib)
    endif()
endif (MSVC AND MSVC_3RDPARTY_ROOT)
