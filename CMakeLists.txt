cmake_minimum_required (VERSION 2.6.4)

include (CheckFunctionExists)
include (CheckCSourceCompiles)
include (CheckCXXSourceCompiles)
include (CheckIncludeFile)

project(FlightGear)

# We have some custom .cmake scripts not in the official distribution.
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")

if(${CMAKE_VERSION} VERSION_GREATER 2.8.4)
    # use official include provided by latest CMake
    include(GNUInstallDirs)
else(${CMAKE_VERSION} VERSION_GREATER 2.8.4)
    # backward compatibility: use our own module for older cmake versions
    include(OldGNUInstallDirs)
endif(${CMAKE_VERSION} VERSION_GREATER 2.8.4)

set(CMAKE_DEBUG_POSTFIX          "d" CACHE STRING "add a postfix, usually d on windows")
set(CMAKE_RELEASE_POSTFIX        ""  CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_RELWITHDEBINFO_POSTFIX ""  CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_MINSIZEREL_POSTFIX     ""  CACHE STRING "add a postfix, usually empty on windows")

# read 'version' file into a variable (stripping any newlines or spaces)
file(READ version versionFile)
string(STRIP ${versionFile} FLIGHTGEAR_VERSION)

#packaging
if(EXISTS ${PROJECT_SOURCE_DIR}/.gitignore)
    file(READ .gitignore CPACK_SOURCE_IGNORE_FILES)
else()
    # clean tar-balls do not contain SCM (.git/.gitignore/...) files.
    set(CPACK_SOURCE_IGNORE_FILES
        "Makefile.am;~$;${CPACK_SOURCE_IGNORE_FILES}")
endif()

list (APPEND CPACK_SOURCE_IGNORE_FILES "${PROJECT_SOURCE_DIR}/.git;\\\\.gitignore")

# split version string into components, note CMAKE_MATCH_0 is the entire regexp match
string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" CPACK_PACKAGE_VERSION ${FLIGHTGEAR_VERSION} )
set(CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1}) 
set(CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_2})
set(CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_3})
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
set(CPACK_RESOURCE_FILE_README  "${PROJECT_SOURCE_DIR}/README")

set(CPACK_SOURCE_GENERATOR TBZ2)
set(CPACK_SOURCE_PACKAGE_FILE_NAME "flightgear-${FLIGHTGEAR_VERSION}" CACHE INTERNAL "tarball basename")

include (CPack)

if (FG_DATA_DIR)
    message(STATUS "Using explicit data-dir: ${FG_DATA_DIR}")
else()
    set(FG_DATA_DIR "${CMAKE_INSTALL_PREFIX}/lib/FlightGear" CACHE PATH "Default location where data files are located")
    message(STATUS "Using default data-dir:  ${FG_DATA_DIR}")
endif()

# Change the default build type to something fast
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if(NOT "$ENV{BUILD_ID}" STREQUAL "")
    set(HUDSON_BUILD_ID $ENV{BUILD_ID})
    set(HUDSON_BUILD_NUMBER $ENV{BUILD_NUMBER})
    message(STATUS "running under Hudson, build-number is ${HUDSON_BUILD_NUMBER}")
else()
    set(HUDSON_BUILD_NUMBER 0)
    set(HUDSON_BUILD_ID "none")
endif()

IF(APPLE)
    set(EVENT_INPUT_DEFAULT 1)

    find_library(CORESERVICES_LIBRARY CoreServices)
    find_library(COCOA_LIBRARY Cocoa)
    list(APPEND PLATFORM_LIBS ${COCOA_LIBRARY} ${CORESERVICES_LIBRARY})

elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(UDev)

    if(UDEV_FOUND)
        set(EVENT_INPUT_DEFAULT 1)
    endif(UDEV_FOUND)
endif()

find_package(Git)
if (GIT_FOUND)
    execute_process(COMMAND git --git-dir ${PROJECT_SOURCE_DIR}/.git rev-parse  HEAD
        OUTPUT_VARIABLE REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Git revision is ${REVISION}")
else()
    set(REVISION "none")
endif()

option(LOGGING           "Set to OFF to build FlightGear without logging" ON)
option(SP_FDMS           "Set to ON to build FlightGear with special-purpose FDMs" OFF)
option(ENABLE_UIUC_MODEL "Set to ON to build FlightGear with UIUCModel FDM" OFF)
option(ENABLE_LARCSIM    "Set to ON to build FlightGear with LaRCsim FDM" OFF)
option(ENABLE_YASIM      "Set to ON to build FlightGear with YASIM FDM" ON)
option(ENABLE_JSBSIM     "Set to ON to build FlightGear with JSBSim FDM" ON)
option(ENABLE_FGADMIN    "Set to ON to build FlightGear with FGADMIN" ON)
option(EVENT_INPUT       "Set to ON to build FlightGear with event-based Input support" ${EVENT_INPUT_DEFAULT})
option(ENABLE_LIBSVN     "Set to ON to build FlightGear/terrasync with libsvnclient support" ON)
option(ENABLE_RTI        "Set to ON to build SimGear with RTI support" OFF)
option(WITH_FGPANEL      "Set to ON to build the fgpanel application" ON)
option(JPEG_FACTORY      "Enable JPEG-factory support" OFF)
option(ENABLE_TESTS      "Set to OFF to disable building test applications" ON)

if (MSVC)
    GET_FILENAME_COMPONENT(PARENT_DIR ${PROJECT_SOURCE_DIR} PATH)
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

if(LOGGING)
    # nothing
else()
    set(FG_NDEBUG 1)
endif()

if(SP_FDMS)
    set(ENABLE_SP_FDM 1)
endif()

if (MSVC AND MSVC_3RDPARTY_ROOT)
    message(STATUS "3rdparty files located in ${MSVC_3RDPARTY_ROOT}")
    set( OSG_MSVC "msvc" )
    if (${MSVC_VERSION} EQUAL 1600)
        set( OSG_MSVC ${OSG_MSVC}100 )
    else (${MSVC_VERSION} EQUAL 1600)
        set( OSG_MSVC ${OSG_MSVC}90 )
    endif (${MSVC_VERSION} EQUAL 1600)
    if (CMAKE_CL_64)
        set( OSG_MSVC ${OSG_MSVC}-64 )
        set( MSVC_3RDPARTY_DIR 3rdParty.x64 )
		set( BOOST_LIB lib64 )
    else (CMAKE_CL_64)
        set( MSVC_3RDPARTY_DIR 3rdParty )
		set( BOOST_LIB lib )
    endif (CMAKE_CL_64)

    set (CMAKE_LIBRARY_PATH ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/lib ${MSVC_3RDPARTY_ROOT}/install/${OSG_MSVC}/OpenScenegraph/lib ${MSVC_3RDPARTY_ROOT}/install/${OSG_MSVC}/SimGear/lib $(BOOST_ROOT)/$(BOOST_LIB) )
    set (CMAKE_INCLUDE_PATH ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/include ${MSVC_3RDPARTY_ROOT}/install/${OSG_MSVC}/OpenScenegraph/include ${MSVC_3RDPARTY_ROOT}/install/${OSG_MSVC}/SimGear/include)
    set (BOOST_ROOT         ${MSVC_3RDPARTY_ROOT}/boost_1_44_0)
    set (OPENAL_INCLUDE_DIR ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/include)
    set (ALUT_INCLUDE_DIR   ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/include)
    set (OPENAL_LIBRARY_DIR ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/lib)
endif (MSVC AND MSVC_3RDPARTY_ROOT)

if(EVENT_INPUT)
    message(STATUS "checking event-based Input")

    IF(APPLE)

    elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
        if(NOT UDEV_FOUND)
            message(WARNING "UDev not found, event input is disabled!")
            set(EVENT_INPUT 0)
        else()
            set(EVENT_INPUT_LIBRARIES ${UDEV_LIBRARIES})
        endif()

    else()
        message(WARNING "event input is not supported on this platform yet")
    endif()

    # Keep PLIB INPUT enabled as long as EventInput does not replace current joystick configurations.
    set(ENABLE_PLIB_JOYSTICK 1)
else(EVENT_INPUT)
    set(ENABLE_PLIB_JOYSTICK 1)
endif(EVENT_INPUT)

# check required dependencies
find_package(Boost   REQUIRED)
find_package(ZLIB    REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenGL  REQUIRED)
find_package(OpenAL  REQUIRED)
find_package(ALUT    REQUIRED)
find_package(OpenSceneGraph 3.0.0 REQUIRED osgText osgSim osgDB osgParticle osgFX osgUtil osgViewer osgGA)

if(ENABLE_FGADMIN)
    find_package(FLTK)

    if ( FLTK_FOUND )
        if ( X11_Xinerama_FOUND )
            message(STATUS "Found X11_Xinerama...")
            list(APPEND FLTK_LIBRARIES ${X11_Xinerama_LIB})
        endif()

        if ( X11_Xft_FOUND )
            message(STATUS "Found X11_Xft...")
            list(APPEND FLTK_LIBRARIES ${X11_Xft_LIB})
        endif()

        message(STATUS "Using FLTK_LIBRARIES for fgadmin: ${FLTK_LIBRARIES}")
    endif ( FLTK_FOUND )
endif(ENABLE_FGADMIN)

if(ENABLE_LIBSVN)
    find_package(SvnClient)

    if(LIBSVN_FOUND)
        message(STATUS "libsvn found, enabling in terrasync")
        set(HAVE_SVN_CLIENT_H 1)
        set(HAVE_LIBSVN_CLIENT_1 1)
    endif(LIBSVN_FOUND)
endif(ENABLE_LIBSVN)

find_package(PLIB REQUIRED puaux pu js fnt)

# FlightGear and SimGear versions need to match
find_package(SimGear ${FLIGHTGEAR_VERSION} REQUIRED)

if (JPEG_FACTORY)
    # check simgear was built with JPEG-factory support
    find_package(JPEG REQUIRED)
    include_directories(${JPEG_INCLUDE_DIR})

    set(CMAKE_REQUIRED_INCLUDES
        ${SIMGEAR_INCLUDE_DIR}
        ${JPEG_INCLUDE_DIR}
        ${OPENSCENEGRAPH_INCLUDE_DIRS})

    check_cxx_source_compiles(
        "#include <simgear/screen/jpgfactory.hxx>
        int main()    { return 0; } "
        FG_JPEG_SERVER)

    if (NOT FG_JPEG_SERVER)
        message(STATUS "JPEG server support requested, but SimGear was built without JPEG support")
    endif()
endif()

check_include_file(unistd.h   HAVE_UNISTD_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(windows.h  HAVE_WINDOWS_H)

if(ENABLE_RTI)
    find_package(RTI)
    if(RTI_FOUND)
        set(FG_HAVE_HLA 1)
    endif(RTI_FOUND)
endif(ENABLE_RTI)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(WARNING_FLAGS_CXX "-Wall")
    set(WARNING_FLAGS_C   "-Wall")
endif(CMAKE_COMPILER_IS_GNUCXX)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang") 
   set (WARNING_FLAGS_CXX "-Wall -Wno-overloaded-virtual")
   set (WARNING_FLAGS_C   "-Wall")
endif() 

if(WIN32)

    if(MSVC)
        # turn off various warnings
        # foreach(warning 4244 4251 4267 4275 4290 4786 4305 4996)
        #     SET(WARNING_FLAGS "${WARNING_FLAGS} /wd${warning}")
        # endforeach(warning)

        set(MSVC_FLAGS "-DNOMINMAX -D_USE_MATH_DEFINES -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D__CRT_NONSTDC_NO_WARNINGS")
        if (${MSVC_VERSION} EQUAL 1600)
            set( MSVC_LD_FLAGS "/FORCE:MULTIPLE" )
        endif (${MSVC_VERSION} EQUAL 1600)
    endif(MSVC)

    set(NOMINMAX 1)
endif(WIN32)

set (BOOST_CXX_FLAGS "-DBOOST_MULTI_INDEX_DISABLE_SERIALIZATION -DBOOST_BIMAP_DISABLE_SERIALIZATION")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS_C} ${MSVC_FLAGS} -D_REENTRANT")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS_CXX} ${MSVC_FLAGS} -D_REENTRANT ${BOOST_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MSVC_LD_FLAGS}")

include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${ALUT_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
    ${OPENAL_INCLUDE_DIR}
    ${SIMGEAR_INCLUDE_DIR}
    ${PLIB_INCLUDE_DIR} )

include_directories(${PROJECT_SOURCE_DIR}/src)

# following is needed, because config.h is include 'bare', whereas
# version.h is included as <Include/version.h> - this should be cleaned up
include_directories(${PROJECT_BINARY_DIR}/src)
include_directories(${PROJECT_BINARY_DIR}/src/Include)

add_definitions(-DHAVE_CONFIG_H)

check_function_exists(mkfifo HAVE_MKFIFO)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
    "${PROJECT_SOURCE_DIR}/src/Include/config_cmake.h.in"
    "${PROJECT_BINARY_DIR}/src/Include/config.h"
)

#and the same for the version header
configure_file (
    "${PROJECT_SOURCE_DIR}/src/Include/version.h.cmake-in"
    "${PROJECT_BINARY_DIR}/src/Include/version.h"
)

add_subdirectory(src)
add_subdirectory(utils)
add_subdirectory(man)

#-----------------------------------------------------------------------------
### uninstall target
#-----------------------------------------------------------------------------
CONFIGURE_FILE(
    "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall
    "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
