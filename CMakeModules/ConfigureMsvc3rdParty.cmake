# ConfigureMsvc3rdParty.cmake - Configure 3rd Party Library Paths on Windows

# we want to handle various cases here:
# fgmeta layout, where windows-3rd-party is a sibling of our flightgear source dir
#  - this should work with no manual options
# explicitly specifying MSVC_3RDPARTY_ROOT: we'll select a subdir based on MSVC version
# and architecture. We want to allow for people specify various paths here:
# - path to windows-3rd-party
# - path to an MSVC versioned subdir, eg -DMSVC_3RDPARTY_ROOT=C:\FGFS\windows-3rd-party\msvc140
# - path to an architecture specific subdir, eg -DMSVC_3RDPARTY_ROOT=C:\FGFS\windows-3rd-party\msvc140\3rdparty.x64

set(_FOUND_3RDPARTY_DIR "NOTFOUND")
set(_FOUND_BOOST_INCLUDE_DIR "NOTFOUND")

# try various suffixes of a base directory, and
# set the variables above on success
function(_check_candidate_msvc_path pathToCheck)
    unset (_freeTypeHeader CACHE )
    unset (_zlibDll CACHE )
    unset (_boostHeaders CACHE )

    find_path(_freeTypeHeader include/ft2build.h
        PATHS 
            ${pathToCheck}
        PATH_SUFFIXES
            ${ARCH_SUBDIR_NAME}
            ${MSVC_SUBDIR_NAME}/${ARCH_SUBDIR_NAME}
            ${COMPAT_SUBDIR_NAME}/${ARCH_SUBDIR_NAME}
        NO_DEFAULT_PATH
    )

    find_path(_zlibDll bin/zlib.dll
        PATHS 
            ${pathToCheck}
        PATH_SUFFIXES
            ${ARCH_SUBDIR_NAME}
            ${MSVC_SUBDIR_NAME}/${ARCH_SUBDIR_NAME}
            ${COMPAT_SUBDIR_NAME}/${ARCH_SUBDIR_NAME}
        NO_DEFAULT_PATH
    )

    find_path(_boostHeaders boost/atomic.hpp
        PATHS 
            ${pathToCheck}
        NO_DEFAULT_PATH
    )
    
    if (_freeTypeHeader AND _zlibDll)
        set(_FOUND_3RDPARTY_DIR "${_freeTypeHeader}" PARENT_SCOPE)

        if (_boostHeaders) 
            set(_FOUND_BOOST_INCLUDE_DIR "${_boostHeaders}" PARENT_SCOPE)
        endif()
    endif()
endfunction()


if (MSVC)
    # compute values for the compiler and arch subdirs
    string(SUBSTRING ${MSVC_VERSION} 0 2 MSVC_VERSION_MAJOR)
    string(SUBSTRING ${MSVC_VERSION} 2 2 MSVC_VERSION_MINOR)

    if (${MSVC_VERSION_MAJOR} EQUAL "19")
        if (${MSVC_VERSION_MINOR} EQUAL "00")
            set( MSVC_SUBDIR_NAME msvc140 )
        else ()
            set( MSVC_SUBDIR_NAME msvc141 )
            set( COMPAT_SUBDIR_NAME msvc140 )
        endif()
    else ()
        message(FATAL_ERROR "Visual Studio 2017 is required")
    endif ()

    if (CMAKE_CL_64)
        SET(ARCH_SUBDIR_NAME "3rdParty.x64")
    else (CMAKE_CL_64)
        SET(ARCH_SUBDIR_NAME "3rdParty")
    endif (CMAKE_CL_64)

    
    # try the explicitly specified value first
    if (EXISTS ${MSVC_3RDPARTY_ROOT})
        _check_candidate_msvc_path("${MSVC_3RDPARTY_ROOT}")
    endif()

    # then try the fgmeta setup: look for a windows-3rdparty sibling of
    # our source dir
    get_filename_component(PARENT_SOURCE_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)
    get_filename_component(PARENT_BINARY_DIR ${PROJECT_BINARY_DIR} DIRECTORY)


    if (NOT _FOUND_3RDPARTY_DIR AND EXISTS "${PARENT_SOURCE_DIR}/windows-3rd-party")
        message(STATUS "Trying src 3rdparty")
        _check_candidate_msvc_path("${PARENT_SOURCE_DIR}/windows-3rd-party")
    endif()

    if (NOT _FOUND_3RDPARTY_DIR AND EXISTS "${PARENT_BINARY_DIR}/windows-3rd-party")
        message(STATUS "Trying bin 3rdparty")
        _check_candidate_msvc_path("${PARENT_BINARY_DIR}/windows-3rd-party")
    endif()

    # try the Jenkins setup, whre the arch dir is copied into the WORKSPACE
    if (NOT _FOUND_3RDPARTY_DIR AND EXISTS "${PARENT_BINARY_DIR}/${ARCH_SUBDIR_NAME}")
        message(STATUS "Trying arch subdir ${PARENT_BINARY_DIR}/${ARCH_SUBDIR_NAME}")
        _check_candidate_msvc_path("${PARENT_BINARY_DIR}/${ARCH_SUBDIR_NAME}")
    endif()

    if (NOT _FOUND_3RDPARTY_DIR)
        message(WARNING "Failed to find the Windows 3rdparty files at all.")
        set(MSVC_3RDPARTY_ROOT NOT_FOUND CACHE PATH "Location where the third-party dependencies are extracted")
    endif()

    list(APPEND PLATFORM_LIBS "winmm.lib")
else (MSVC)
    set(MSVC_3RDPARTY_ROOT NOT_FOUND CACHE PATH "Location where the third-party dependencies are extracted")
endif (MSVC)

if (MSVC AND _FOUND_3RDPARTY_DIR)
    message(STATUS "3rdparty files located in ${_FOUND_3RDPARTY_DIR}")
    list(APPEND CMAKE_PREFIX_PATH ${_FOUND_3RDPARTY_DIR})
    set(FINAL_MSVC_3RDPARTY_DIR ${_FOUND_3RDPARTY_DIR})

    if (CMAKE_CL_64)
		set( BOOST_LIB lib64 )
    else (CMAKE_CL_64)
	    set( BOOST_LIB lib )
    endif (CMAKE_CL_64)

    if(NOT BOOST_INCLUDEDIR AND _FOUND_BOOST_INCLUDE_DIR)
        set(BOOST_INCLUDEDIR ${_FOUND_BOOST_INCLUDE_DIR})
        message(STATUS "found Boost headers at ${_FOUND_BOOST_INCLUDE_DIR}")
      endif()
endif ()
