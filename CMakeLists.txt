cmake_minimum_required (VERSION 2.8.11)

include (CheckFunctionExists)
include (CheckCSourceCompiles)
include (CheckCXXSourceCompiles)
include (CheckIncludeFile)

if(COMMAND cmake_policy)
   if(POLICY CMP0054)
       cmake_policy(SET CMP0054 NEW)
   endif()
   if(POLICY CMP0042)
       cmake_policy(SET CMP0042 NEW)
   endif()
endif()

# using 10.7 because boost requires libc++ and 10.6 doesn't include it
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.7)
set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk)

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

# Warning when build is not an out-of-source build.
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" InSourceBuild)
if(InSourceBuild)
    message(WARNING  "Avoid building inside the source tree!")
    message(WARNING  "Create a separate build directory instead (i.e. 'fgbuild') and call CMake from there: ")
    message(WARNING  "  mkdir ../fgbuild && cd ../fgbuild && cmake ${CMAKE_SOURCE_DIR}")
endif(InSourceBuild)

# System detection/default settings
include( DetectDistro )
include( DetectBrowser )

set(CMAKE_DEBUG_POSTFIX          "d" CACHE STRING "add a postfix, usually d on windows")
set(CMAKE_RELEASE_POSTFIX        ""  CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_RELWITHDEBINFO_POSTFIX ""  CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_MINSIZEREL_POSTFIX     ""  CACHE STRING "add a postfix, usually empty on windows")

# read 'version' file into a variable (stripping any newlines or spaces)
file(READ version versionFile)
if (NOT versionFile)
    message(FATAL_ERROR "Unable to determine FlightGear version. Version file is missing.")
endif()
string(STRIP "${versionFile}" FLIGHTGEAR_VERSION)

# FlightGear packaging (to build a source tarball)
include( ConfigureCPack )

# FlightGear base package path
if (FG_DATA_DIR)
    message(STATUS "Using explicit data directory for base package: ${FG_DATA_DIR}")
else()
    set(FG_DATA_DIR "${CMAKE_INSTALL_PREFIX}/lib/FlightGear" CACHE PATH "Default location where data files are located")
    message(STATUS "Using default data directory for base package:  ${FG_DATA_DIR}")
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
    message(STATUS "running under Hudson/Jenkins, build-number is ${HUDSON_BUILD_NUMBER}")
else()
    set(HUDSON_BUILD_NUMBER 0)
    set(HUDSON_BUILD_ID "none")
endif()

#####################################################################################
# Configure library search paths
#####################################################################################


if(NOT "${CMAKE_LIBRARY_ARCHITECTURE}" STREQUAL "")
    # Workaround for Ubuntu/Debian which introduced the "multiarch" library
    # directory structure, which is unsupported by CMake < 2.8.10, so we need to
    # add paths manually
    # see http://www.cmake.org/Bug/view.php?id=12049 and
    # http://www.cmake.org/Bug/view.php?id=12037
    list(APPEND ADDITIONAL_LIBRARY_PATHS
         /usr/local/lib/${CMAKE_LIBRARY_ARCHITECTURE}
         /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
         /lib/${CMAKE_LIBRARY_ARCHITECTURE})
    message(STATUS "additional library directories: ${ADDITIONAL_LIBRARY_PATHS}")
endif()

#####################################################################################

IF(APPLE)
    set(EVENT_INPUT_DEFAULT 1)

    find_library(CORESERVICES_LIBRARY CoreServices)
    find_library(COCOA_LIBRARY Cocoa)
    list(APPEND PLATFORM_LIBS ${COCOA_LIBRARY} ${CORESERVICES_LIBRARY})
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR
	${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    find_package(Threads REQUIRED)
    find_package(X11 REQUIRED)

    set(USE_DBUS_DEFAULT 1)

    find_package(UDev)
    if(UDEV_FOUND)
        set(EVENT_INPUT_DEFAULT 1)
    endif(UDEV_FOUND)

    find_package(Speex)
    find_package(Speexdsp)
    if(SPEEX_FOUND AND SPEEXDSP_FOUND)
        set(SYSTEM_SPEEX_DEFAULT 1)
    endif(SPEEX_FOUND AND SPEEXDSP_FOUND)

    find_package(Gsm)
    if(GSM_FOUND)
        set(SYSTEM_GSM_DEFAULT 1)
    endif(GSM_FOUND)

    find_package(Flite)
    if(FLITE_FOUND)
        set(SYSTEM_FLITE_DEFAULT 1)
    endif()

    find_package(HtsEngine)
    if(HTS_ENGINE_FOUND)
        set(SYSTEM_HTS_ENGINE_DEFAULT 1)
    endif()
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

# FlightGear build options
option(SIMGEAR_SHARED    "Set to ON when SimGear was built as a shared library" OFF)
option(LOGGING           "Set to ON to build FlightGear with logging support (default)" ON)
option(JSBSIM_TERRAIN    "Set to ON to build FlightGear with JSBSim terrain handling code" ON)
option(SP_FDMS           "Set to ON to build FlightGear with special-purpose FDMs" OFF)
option(ENABLE_UIUC_MODEL "Set to ON to build FlightGear with UIUCModel FDM" OFF)
option(ENABLE_LARCSIM    "Set to ON to build FlightGear with LaRCsim FDM" OFF)
option(ENABLE_YASIM      "Set to ON to build FlightGear with YASIM FDM (default)" ON)
option(ENABLE_JSBSIM     "Set to ON to build FlightGear with JSBSim FDM (default)" ON)
option(EVENT_INPUT       "Set to ON to build FlightGear with event-based Input support" ${EVENT_INPUT_DEFAULT})
option(ENABLE_RTI        "Set to ON to build FlightGear with RTI support" OFF)
option(ENABLE_PROFILE    "Set to ON to build FlightGear with gperftools profiling support" OFF)
option(SYSTEM_SQLITE     "Set to ON to build FlightGear with the system's SQLite3 library" OFF)
option(ENABLE_IAX        "Set to ON to build FlightGear with IAXClient/fgcom built-in (default)" ON)
option(USE_DBUS          "Set to ON to build FlightGear with DBus screensaver interaction (default on Linux)" ${USE_DBUS_DEFAULT})
option(SYSTEM_SPEEX      "Set to ON to build IAXClient with the system's speex and speexdsp library" ${SYSTEM_SPEEX_DEFAULT})
option(SYSTEM_GSM        "Set to ON to build IAXClient with the system's GSM library" ${SYSTEM_GSM_DEFAULT})
option(SYSTEM_FLITE      "Set to ON to build Flightgear with the system's Flite library" ${SYSTEM_FLITE_DEFAULT})
option(SYSTEM_HTS_ENGINE "Set to ON to build Flightgear with the system's HTS Engine library" ${SYSTEM_HTS_ENGINE_DEFAULT})
option(FG_NIGHTLY        "Set to ON to mark this as a nightly build" OFF)

# additional utilities
option(ENABLE_FGADMIN    "Set to ON to build the FGADMIN application (default)" ON)
option(ENABLE_FGELEV     "Set to ON to build the fgelev application (default)" ON)
option(WITH_FGPANEL      "Set to ON to build the fgpanel application (default)" ON)
option(ENABLE_FGVIEWER   "Set to ON to build the fgviewer application (default)" ON)
option(ENABLE_GPSSMOOTH  "Set to ON to build the GPSsmooth application (default)" ON)
option(ENABLE_TERRASYNC  "Set to ON to build the terrasync application (default)" ON)
option(ENABLE_FGJS       "Set to ON to build the fgjs application (default)" ON)
option(ENABLE_JS_DEMO    "Set to ON to build the js_demo application (default)" ON)
option(ENABLE_METAR      "Set to ON to build the metar application (default)" ON)
option(ENABLE_TESTS      "Set to ON to build test applications (default)" ON)
option(ENABLE_FGCOM      "Set to ON to build the FGCom application (default)" ON)
option(ENABLE_FLITE      "Set to ON to build the Flite text-to-speech module" ON)
option(ENABLE_QT         "Set to ON to build the internal Qt launcher" ON)

if(LOGGING)
    # nothing
else()
    set(FG_NDEBUG 1)
endif()

if(JSBSIM_TERRAIN)
   set(JSBSIM_USE_GROUNDREACTIONS 1)
endif()

if(SP_FDMS)
    set(ENABLE_SP_FDM 1)
endif()

if(ENABLE_FGCOM)
  set(ENABLE_IAX 1)
  include_directories(${PROJECT_SOURCE_DIR}/3rdparty/iaxclient/lib ) # for iaxclient.h
endif()

# Setup MSVC 3rd party directories
include( ConfigureMsvc3rdParty )

if(EVENT_INPUT)
    if(APPLE)
    elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
        if(NOT UDEV_FOUND)
            message(WARNING "UDev not found, event input is disabled!")
            set(EVENT_INPUT 0)
        else()
            add_definitions(-DWITH_EVENTINPUT)
            set(EVENT_INPUT_LIBRARIES ${UDEV_LIBRARIES})
            message(STATUS "event-based input enabled. Using ${UDEV_LIBRARIES}")
        endif()
    else()
        message(WARNING "event-based input is not supported on this platform yet")
    endif()

    # Keep PLIB INPUT enabled as long as EventInput does not replace current joystick configurations.
    set(ENABLE_PLIB_JOYSTICK 1)
else(EVENT_INPUT)
    set(ENABLE_PLIB_JOYSTICK 1)
endif(EVENT_INPUT)

# check required dependencies
find_package(Boost   REQUIRED)
find_package(ZLIB    REQUIRED)
find_package(OpenGL  REQUIRED)
find_package(OpenAL  REQUIRED)
find_package(OpenSceneGraph 3.2.0 REQUIRED
  osgText
  osgSim
  osgDB
  osgParticle
  osgFX
  osgUtil
  osgViewer
  osgGA
)

if (MSVC)
	find_package(CrashRpt)
	if (CRASHRPT_FOUND)
		set(HAVE_CRASHRPT 1)
		message(STATUS "Using CrashRpt")
        include_directories( ${CRASHRPT_INCLUDE_DIR})
	endif()
endif()

##############################################################################
## Sqlite3 setup

if (SYSTEM_SQLITE)
    find_package(SQLite3 REQUIRED)

    message(STATUS "Using system SQLite3 library")
else()
    set(SQLITE3_INCLUDED_DIR "${CMAKE_SOURCE_DIR}/3rdparty/sqlite3")
    # this target is defined in src/Navaids/CMakeLists.txt
    list(APPEND SQLITE3_LIBRARY fgsqlite3)
endif (SYSTEM_SQLITE)

# Sqlite always depends on the threading lib
list(APPEND SQLITE3_LIBRARY ${CMAKE_THREAD_LIBS_INIT})

##############################################################################
## DBus setup

if (USE_DBUS)
    include(FindPkgConfig)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(DBUS dbus-1)
    endif (PKG_CONFIG_FOUND) #if we don't have pkg-config, assume we don't have libdbus-1-dev either http://packages.debian.org/sid/libdbus-1-dev
    if (DBUS_FOUND)
        set(HAVE_DBUS 1)
        message(STATUS "Using DBus")
        include_directories( ${DBUS_INCLUDE_DIRS})
    else()
        message(STATUS "DBus not found, screensaver control disabled")
    endif (DBUS_FOUND)
else()
endif (USE_DBUS)

##############################################################################
## Qt5 setup setup
if (ENABLE_QT)
	message(STATUS "Qt launcher enabled, checking for Qt 5.1 / qmake")
	find_package(Qt5 5.1 COMPONENTS Widgets)
	if (Qt5Widgets_FOUND)
		message(STATUS "Will enable Qt launcher GUI")
		message(STATUS "  Qt5Widgets version: ${Qt5Widgets_VERSION_STRING}")
		message(STATUS "  Qt5Widgets include dir: ${Qt5Widgets_INCLUDE_DIRS}")
		set(HAVE_QT 1)
	endif()
endif (ENABLE_QT)
##############################################################################

find_package(PLIB REQUIRED puaux pu js fnt)

# FlightGear and SimGear versions need to match
find_package(SimGear ${FLIGHTGEAR_VERSION} REQUIRED)

##############################################################################

check_include_file(unistd.h   HAVE_UNISTD_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(windows.h  HAVE_WINDOWS_H)

if(ENABLE_PROFILE)
    find_package(GooglePerfTools REQUIRED)
    set(FG_HAVE_GPERFTOOLS 1)
    message(STATUS "Built-in profiler using gperftools available")
endif()

if(ENABLE_RTI)
    message(STATUS "RTI: ENABLED")
    find_package(RTI)
    if(RTI_FOUND)
        set(FG_HAVE_HLA 1)
    endif(RTI_FOUND)
else()
    message(STATUS "RTI: DISABLED")
endif(ENABLE_RTI)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(WARNING_FLAGS_CXX "-Wall")
    set(WARNING_FLAGS_C   "-Wall")
endif(CMAKE_COMPILER_IS_GNUCXX)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
   set (WARNING_FLAGS_CXX "-Wall -Wno-overloaded-virtual -Wno-redeclared-class-member")
   set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
   set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
   set (WARNING_FLAGS_C "-Wall")
endif()

if(WIN32)

    if(MSVC)
        # turn off various warnings
        # foreach(warning 4244 4251 4267 4275 4290 4786 4305 4996)
        #     SET(WARNING_FLAGS "${WARNING_FLAGS} /wd${warning}")
        # endforeach(warning)

        set(MSVC_FLAGS "-DNOMINMAX -D_USE_MATH_DEFINES -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D__CRT_NONSTDC_NO_WARNINGS -Dstrdup=_strdup")
        if (${MSVC_VERSION} GREATER 1599)
            set( MSVC_LD_FLAGS "/FORCE:MULTIPLE" )
        endif (${MSVC_VERSION} GREATER 1599)
    endif(MSVC)

    set(NOMINMAX 1)
endif(WIN32)

set (BOOST_CXX_FLAGS "-DBOOST_BIMAP_DISABLE_SERIALIZATION")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS_C} ${MSVC_FLAGS} -D_REENTRANT")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS_CXX} ${MSVC_FLAGS} -D_REENTRANT ${BOOST_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MSVC_LD_FLAGS}")

include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
    ${OPENAL_INCLUDE_DIR}
    ${SIMGEAR_INCLUDE_DIRS}
    ${PLIB_INCLUDE_DIR}
    ${SQLITE3_INCLUDED_DIR} )

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/src)
# following is needed, because config.h is include 'bare', whereas
# version.h is included as <Include/version.h> - this should be cleaned up
include_directories(${PROJECT_BINARY_DIR}/src)
include_directories(${PROJECT_BINARY_DIR}/src/Include)

if (ENABLE_FLITE)
    include_directories(${PROJECT_SOURCE_DIR}/3rdparty/hts_engine_API/include )
    include_directories(${PROJECT_SOURCE_DIR}/3rdparty/flite_hts_engine/include )
endif()

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

add_subdirectory(3rdparty)
add_subdirectory(utils)
add_subdirectory(src)
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
