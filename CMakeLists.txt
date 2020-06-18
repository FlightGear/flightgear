cmake_minimum_required (VERSION 3.10)

include (CheckFunctionExists)
include (CheckVariableExists)
include (CheckCSourceCompiles)
include (CheckCXXSourceCompiles)
include (CheckIncludeFile)
include (CheckIncludeFileCXX)

cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0042 NEW) # Mac RPATH policy

# AUTOMOC/AUTOUIC on generated files policy. Since we only
# generate resource files with fgrcc which don't use Qt,
# we set those files source property. See the help for this
# policy for more info
cmake_policy(SET CMP0071 NEW)

# OpenGL VND policy : use the old definition for now, until we can audit this
if(POLICY CMP0072)
    cmake_policy(SET CMP0072 OLD)
endif()

message("CMAKE Build type: ${CMAKE_BUILD_TYPE}")
# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

if(NOT FG_BUILD_TYPE)
  message(STATUS "Setting build type to 'Dev' as none was specified.")
  set(FG_BUILD_TYPE Dev CACHE STRING "Choose the FlightGear build type" FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE FG_BUILD_TYPE PROPERTY STRINGS "Dev" "Nightly" "Release")
endif()

if(APPLE)
	set(CMAKE_INSTALL_RPATH "@loader_path/../Frameworks")
	# when building, don't use the install RPATH already
	# (but later on when installing)
	SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment version")

project(FlightGear)

# Define SRC_PARENT_DIR as the parent directory of the project source directory
get_filename_component(SRC_PARENT_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)

# We have some custom .cmake scripts not in the official distribution.
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")
set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS TRUE)

# Warning when build is not an out-of-source build.
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" InSourceBuild)
if(InSourceBuild)
    message(WARNING  "Avoid building inside the source tree!")
    message(WARNING  "Create a separate build directory instead (i.e. 'fgbuild') and call CMake from there: ")
    message(WARNING  "  mkdir ../fgbuild && cd ../fgbuild && cmake ${CMAKE_SOURCE_DIR}")
endif(InSourceBuild)

include(GNUInstallDirs)
# System detection/default settings
include( DetectDistro )
include( DetectBrowser )
include( ExportDebugSymbols )

set(CMAKE_DEBUG_POSTFIX          "d" CACHE STRING "add a postfix, usually d on windows")
set(CMAKE_RELEASE_POSTFIX        ""  CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_RELWITHDEBINFO_POSTFIX ""  CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_MINSIZEREL_POSTFIX     ""  CACHE STRING "add a postfix, usually empty on windows")

# read 'version' file into a variable (stripping any newlines or spaces)
file(READ flightgear-version versionFile)
if (NOT versionFile)
    message(FATAL_ERROR "Unable to determine FlightGear version. Version file is missing.")
endif()
string(STRIP "${versionFile}" FLIGHTGEAR_VERSION)
# add a dependency on the version file
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS version)

# FlightGear packaging (to build a source tarball)
include( ConfigureCPack )

# FlightGear base package path
if (FG_DATA_DIR)
    message(STATUS "Using explicit data directory for base package: ${FG_DATA_DIR}")
elseif(IS_DIRECTORY "${SRC_PARENT_DIR}/fgdata")
    set(FG_DATA_DIR "${SRC_PARENT_DIR}/fgdata")
    message(STATUS "Using data directory for base package: ${FG_DATA_DIR}")
else()
    set(FG_DATA_DIR "${CMAKE_INSTALL_PREFIX}/lib/FlightGear" CACHE PATH "Default location where data files are located")
    message(STATUS "Using default data directory for base package:  ${FG_DATA_DIR}")
endif()

#####################################################################################
# Configure library search paths
#####################################################################################

find_package(Threads REQUIRED)

IF(APPLE)
    set(EVENT_INPUT_DEFAULT 1)

    find_library(CORESERVICES_LIBRARY CoreServices)
    find_library(COCOA_LIBRARY Cocoa)
    list(APPEND PLATFORM_LIBS ${COCOA_LIBRARY} ${CORESERVICES_LIBRARY})
elseif(WIN32)
    set(EVENT_INPUT_DEFAULT 1)
    list(APPEND PLATFORM_LIBS "Shlwapi.lib")

    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "bin")
    set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
    include(InstallRequiredSystemLibraries)
    message(STATUS "Installing: ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}")

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR
	${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD" OR
	${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
    find_package(X11 REQUIRED)

    set(USE_DBUS_DEFAULT 1)

    find_package(UDev QUIET)
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

# FlightGear build options
option(LOGGING           "Set to ON to build FlightGear with logging support (default)" ON)
option(JSBSIM_TERRAIN    "Set to ON to build FlightGear with JSBSim terrain handling code" ON)
option(SP_FDMS           "Set to ON to build FlightGear with special-purpose FDMs" ON)
option(ENABLE_UIUC_MODEL "Set to ON to build FlightGear with UIUCModel FDM" ON)
option(ENABLE_LARCSIM    "Set to ON to build FlightGear with LaRCsim FDM" ON)
option(ENABLE_YASIM      "Set to ON to build FlightGear with YASIM FDM (default)" ON)
option(ENABLE_JSBSIM     "Set to ON to build FlightGear with JSBSim FDM (default)" ON)
option(EVENT_INPUT       "Set to ON to build FlightGear with event-based Input support" ${EVENT_INPUT_DEFAULT})
option(ENABLE_RTI        "Set to ON to build FlightGear with RTI support" OFF)
option(SYSTEM_SQLITE     "Set to ON to build FlightGear with the system's SQLite3 library" OFF)
option(ENABLE_IAX        "Set to ON to build FlightGear with IAXClient/fgcom built-in (default)" ON)
option(USE_DBUS          "Set to ON to build FlightGear with DBus screensaver interaction (default on Linux)" ${USE_DBUS_DEFAULT})
option(SYSTEM_SPEEX      "Set to ON to build IAXClient with the system's speex and speexdsp library" ${SYSTEM_SPEEX_DEFAULT})
option(SYSTEM_GSM        "Set to ON to build IAXClient with the system's GSM library" ${SYSTEM_GSM_DEFAULT})
option(SYSTEM_FLITE      "Set to ON to build Flightgear with the system's Flite library" ${SYSTEM_FLITE_DEFAULT})
option(SYSTEM_HTS_ENGINE "Set to ON to build Flightgear with the system's HTS Engine library" ${SYSTEM_HTS_ENGINE_DEFAULT})
option(SYSTEM_CPPUNIT    "Set to ON to build Flightgear with the system's CppUnit library")

# additional utilities
option(ENABLE_FGELEV     "Set to ON to build the fgelev application (default)" ON)
option(WITH_FGPANEL      "Set to ON to build the fgpanel application" OFF)
option(ENABLE_FGVIEWER   "Set to ON to build the fgviewer application (default)" ON)
option(ENABLE_GPSSMOOTH  "Set to ON to build the GPSsmooth application (default)" ON)
option(ENABLE_FGJS       "Set to ON to build the fgjs application (default)" ON)
option(ENABLE_JS_DEMO    "Set to ON to build the js_demo application (default)" ON)
option(ENABLE_METAR      "Set to ON to build the metar application (default)" ON)
option(ENABLE_STGMERGE   "Set to ON to build the stgmerge application (default)" OFF)
option(ENABLE_FGCOM      "Set to ON to build the FGCom application (default)" ON)
option(ENABLE_QT         "Set to ON to build the internal Qt launcher" ON)
option(ENABLE_TRAFFIC    "Set to ON to build the external traffic generator modules" ON)
option(ENABLE_FGQCANVAS  "Set to ON to build the Qt-based remote canvas application" OFF)
option(ENABLE_DEMCONVERT "Set to ON to build the dem conversion tool (default)" ON)
option(ENABLE_HID_INPUT  "Set to ON to build HID-based input code" ${EVENT_INPUT_DEFAULT})
option(ENABLE_PLIB_JOYSTICK  "Set to ON to enable legacy joystick code (default)" ON)
option(ENABLE_SWIFT      "Set to ON to build the swift module" ON)


# Test-suite options.
option(ENABLE_AUTOTESTING "Set to ON to execute the test suite after building the test_suite target (default)" ON)

include (DetectArch)

# when building an OSG with commit 15ec7e2ae7a8b983ecc44e1ce7363a9a9fa7da95
# applied, we can use better link options
option(OSG_FSTREAM_EXPORT_FIXED "Set to ON if the osgDB fstream export patch is applied" OFF)

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
endif()

# Setup MSVC 3rd party directories
include( ConfigureMsvc3rdParty )

if(EVENT_INPUT)
    if(APPLE)
      add_definitions(-DWITH_EVENTINPUT)
      find_library(IOKIT_FRAMEWORK IOKit)
      list(APPEND EVENT_INPUT_LIBRARIES ${IOKIT_FRAMEWORK})
    elseif(CMAKE_SYSTEM_NAME MATCHES "Linux|FreeBSD|OpenBSD")
        if(NOT UDEV_FOUND)
            message(WARNING "UDev not found, event input is disabled!")
            set(EVENT_INPUT 0)

            # HIDraw backend for hidapi also needs UDev, so also force
            # HID-input to off in this scenario
            set(ENABLE_HID_INPUT 0)
        else()
            add_definitions(-DWITH_EVENTINPUT)
            set(EVENT_INPUT_LIBRARIES ${UDEV_LIBRARIES})
            message(STATUS "event-based input enabled. Using ${UDEV_LIBRARIES}")
        endif()
    else()
        add_definitions(-DWITH_EVENTINPUT)
    endif()

    if (ENABLE_HID_INPUT)
        message(STATUS "Enabling HID-API input")
        list(APPEND EVENT_INPUT_LIBRARIES hidapi)
    endif()
endif(EVENT_INPUT)

include(SetupSwiftLibraries)

if (SYSTEM_CPPUNIT)
    find_package(CppUnit REQUIRED)
endif()

# check required dependencies
find_package(Boost REQUIRED)
find_package(OpenGL  REQUIRED)
find_package(OpenSceneGraph 3.4.1 REQUIRED
  osgText
  osgSim
  osgDB
  osgParticle
  osgFX
  osgUtil
  osgViewer
  osgGA
)

find_package(sentry QUIET)

set(sentry_api_key $ENV{FLIGHTGEAR_SENTRY_API_KEY})
if (TARGET sentry::sentry)
    if (NOT sentry_api_key)
        message(STATUS "Sentry.io crash reporting detected, but FLIGHTGEAR_SENTRY_API_KEY not set")
    else()
        message(STATUS "Sentry.io crash reporting enabled")
        set(HAVE_SENTRY 1)
    endif()
endif()

##############################################################################
## Sqlite3 setup

if (SYSTEM_SQLITE)
    find_package(SQLite3 REQUIRED)
    message(STATUS "Using system SQLite3 library")
endif (SYSTEM_SQLITE)

##############################################################################
## DBus setup
if (USE_DBUS)
    find_package(DBus)
endif (USE_DBUS)

##############################################################################
## Qt5 setup setup
if (ENABLE_QT)
    find_package(Qt5 5.9 COMPONENTS Widgets Gui Network Qml Quick Svg)
    if (Qt5Widgets_FOUND)
        set(HAVE_QT 1)
        include (Translations)

        # extract the Qt root direction, since we need it for a few things
        if (TARGET Qt5::qmake)
            get_target_property(_qt5_qmake_path Qt5::qmake LOCATION)
            get_filename_component(FG_QT_BIN_DIR ${_qt5_qmake_path} DIRECTORY)
            get_filename_component(FG_QT_ROOT_DIR ${FG_QT_BIN_DIR} DIRECTORY)
        endif()

        message(STATUS "Qt GUI enabled, found Qt at: ${FG_QT_ROOT_DIR}")
    else()
        message(STATUS "Qt GUI disabled, Qt/qmake not found in PATH/CMAKE_PREFIX_PATH")
        # don't try to build FGQCanvas if Qt wasn't found correctly
        set(ENABLE_FGQCANVAS OFF)
    endif()

    # we would ideally use TARGET::Qt5GuiPrivate but Fedora broke it
    # perform a manual check instead
    # see https://bugzilla.redhat.com/show_bug.cgi?id=1846613
    set (CMAKE_REQUIRED_INCLUDES ${Qt5Gui_PRIVATE_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS})
    check_include_file_cxx("private/qopenglcontext_p.h" REALLY_HAVE_QGUIPRIVATE)

    if (REALLY_HAVE_QGUIPRIVATE)
        set(ENABLE_QQ_UI 1)
    else()
        message(STATUS "QtGui private headers not available.")
    endif()
else()
    set(ENABLE_FGQCANVAS OFF)
endif (ENABLE_QT)
##############################################################################

find_package(PLIB REQUIRED puaux pu)

# FlightGear and SimGear versions need to match major + minor
# split version string into components, note CMAKE_MATCH_0 is the entire regexp match
string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" VERSION_REGEX ${FLIGHTGEAR_VERSION} )
set(FG_VERSION_MAJOR ${CMAKE_MATCH_1})
set(FG_VERSION_MINOR ${CMAKE_MATCH_2})
set(FG_VERSION_PATCH ${CMAKE_MATCH_3})

set(MIN_SIMGEAR_VERSION "${FG_VERSION_MAJOR}.${FG_VERSION_MINOR}.0")
message(STATUS "Min Simgear version is ${MIN_SIMGEAR_VERSION}")

find_package(SimGear ${MIN_SIMGEAR_VERSION} CONFIG REQUIRED)

##############################################################################

if(ENABLE_RTI)
    message(STATUS "RTI: ENABLED")
    set(FG_HAVE_HLA 1)
endif(ENABLE_RTI)

if (ENABLE_SIMD)
  message(STATUS "SSE/SSE2 support: ENABLED")
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    set(WARNING_FLAGS_CXX "-Wall")
    set(WARNING_FLAGS_C   "-Wall")

    if (X86 OR X86_64)
        set(SIMD_COMPILER_FLAGS "-msse2 -mfpmath=sse -ftree-vectorize -ftree-slp-vectorize")
    endif()

    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO  "-O3 -g -DNDEBUG")
endif(CMAKE_COMPILER_IS_GNUCXX)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" )
   set(WARNING_FLAGS_CXX "-Wall -Wno-overloaded-virtual \
     -Wno-redeclared-class-member \
     -Wno-inconsistent-missing-override \
     -Wno-unused-local-typedef")

    # override CMake default RelWithDebInfo flags.
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO  "-O3 -g -DNDEBUG")

    set(SIMD_COMPILER_FLAGS "-msse2 -mfpmath=sse -ftree-vectorize -ftree-slp-vectorize")

  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
   set(WARNING_FLAGS_C "-Wall")
endif()

# ASan is enabled in SimGear's configuration, in case you are
# wondering why there is no option() for it above.
if (ENABLE_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")

    # needed for check_cxx_source_compiles
    set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address")
endif()

if(WIN32)
    if(MSVC)
        # override CMake default RelWithDebInfo flags. This is important to ensure
        # good performance
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/Zi /O2 /Ob2 /D NDEBUG")
        set(CMAKE_C_FLAGS_RELWITHDEBINFO "/Zi /O2 /Ob2 /D NDEBUG")

        set(MSVC_FLAGS "/bigobj -DNOMINMAX -D_USE_MATH_DEFINES -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D__CRT_NONSTDC_NO_WARNINGS -D_HAS_STD_BYTE=0")
        if (X86)
            set(SIMD_COMPILER_FLAGS "/arch:SSE /arch:SSE2")
        endif()

        if (NOT OSG_FSTREAM_EXPORT_FIXED AND ${MSVC_VERSION} GREATER 1599)
          message(STATUS "For better linking performance, use OSG with fixed fstream header")
          # needed to avoid link errors on multiply-defined standard C++
          # symbols. This issue was fixed in OSG commit 15ec7e2ae7a8b983ecc44e1ce7363a9a9fa7da95
          set( MSVC_LD_FLAGS "/FORCE:MULTIPLE" )
        endif ()
    endif(MSVC)

    set(NOMINMAX 1)
endif(WIN32)

set (BOOST_CXX_FLAGS "-DBOOST_BIMAP_DISABLE_SERIALIZATION -DBOOST_NO_STDLIB_CONFIG -DBOOST_NO_AUTO_PTR -DBOOST_NO_CXX98_BINDERS")

# append the SIMD flags if requested
if (ENABLE_SIMD)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SIMD_COMPILER_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SIMD_COMPILER_FLAGS}")

  # set for multi-configuration generators
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${SIMD_COMPILER_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${SIMD_COMPILER_FLAGS}")

  set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${SIMD_COMPILER_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${SIMD_COMPILER_FLAGS}")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS_C} ${MSVC_FLAGS} -D_REENTRANT")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS_CXX} ${MSVC_FLAGS} -D_REENTRANT ${BOOST_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MSVC_LD_FLAGS}")

add_definitions(-DHAVE_CONFIG_H)
include (CheckPOSIXFeatures)

set(FG_TEST_SUITE_DATA "${CMAKE_SOURCE_DIR}/test_suite/test_data")

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
    "${PROJECT_SOURCE_DIR}/src/Include/config_cmake.h.in"
    "${PROJECT_BINARY_DIR}/src/Include/config.h"
)

# Setup flightgearBuildId.h header, each build
include( GenerateBuildInfo )

# global includes: these are used by every single target, so we do set them globally
# even though that's not strictly best practice
include_directories(${PROJECT_BINARY_DIR}/src/Include)
include_directories(${PROJECT_BINARY_DIR}/src) # for version.h
include_directories(${PROJECT_SOURCE_DIR}/src)

add_subdirectory(3rdparty)
add_subdirectory(utils)

add_subdirectory(src)
add_subdirectory(icons)
add_subdirectory(man)
add_subdirectory(package)
add_subdirectory(scripts)

#----------------------------------------------------------------------------
### MSVC startup project - ensure you can just hit build & run in MSVC
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT fgfs)

include(Installation)

# The test suite.
enable_testing()
add_subdirectory(test_suite EXCLUDE_FROM_ALL)
