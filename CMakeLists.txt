cmake_minimum_required (VERSION 2.6)

include (CheckFunctionExists)
include (CheckCSourceCompiles)
include (CheckCXXSourceCompiles)
include (CheckIncludeFile)
include (CPack)

project(FlightGear)

file(READ version FLIGHTGEAR_VERSION)

#packaging
SET(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
SET(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README")

# We have some custom .cmake scripts not in the official distribution.
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")

# autoconf compatibility
set(PKGLIBDIR "foo")

option(LOGGING "Set to OFF to build FlightGear without logging" ON)

option(SP_FDMS "Set to ON to build FlightGear with special-purpose FDMs" OFF)
option(ENABLE_UIUC_MODEL "Set to ON to build FlightGear with UIUCModel FDM" ON)
option(ENABLE_LARCSIM "Set to ON to build FlightGear with LaRCsim FDM" ON)
option(ENABLE_YASIM "Set to ON to build FlightGear with YASIM FDM" ON)
option(ENABLE_JSBSIM "Set to ON to build FlightGear with JSBSim FDM" ON)

option(ATCDCL "Set to ON to build FlightGear with Dave Luff's ATC code" OFF)
option(EVENT_INPUT "Set to ON to build FlightGear with event-based Input support" OFF)

if(LOGGING)
	# nothing
else()
	set(FG_NDEBUG 1)
endif()

if(${SP_FDMS})
	set(ENABLE_SP_FDM 1)
endif()

if(ATCDCL)
	set(ENABLE_ATCDCL 1)
endif()

if(EVENT_INPUT)
	message(STATUS "checking event-based Input")
	IF(APPLE)
		
	elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
		
	else()
		message(WARNING "event input is not supported on this platform yet")
	endif()
else(EVENT_INPUT)
	set(ENABLE_PLIB_JOYSTICK 1)
endif(EVENT_INPUT)

# check required dependencies
if (MSVC)
    # on MSVC, Olaf reports that the serialization library is required at
    # link time. No one has you explained why, unfortunately.
    set(Boost_USE_STATIC_LIBS        ON)
    set(Boost_USE_MULTITHREADED      ON)
    set(Boost_USE_STATIC_RUNTIME    OFF)
    find_package(Boost REQUIRED COMPONENTS serialization)
else (MSVC)
    find_package(Boost REQUIRED)
endif (MSVC)

find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenAL REQUIRED)
find_package(ALUT REQUIRED)
find_package(OpenSceneGraph 2.8.2 REQUIRED osgText osgSim osgDB osgParticle osgFX osgUtil osgViewer osgGA)
find_package(PLIB REQUIRED puaux pu js fnt)
find_package(SimGear 2.0.0 REQUIRED)

check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)

# definition depends on OSG version
set(CMAKE_REQUIRED_INCLUDES ${OPENSCENEGRAPH_INCLUDE_DIRS})

check_cxx_source_compiles(
    "#include <osg/CullSettings>
    int main()
    {
       osg::CullSettings::VariablesMask mask = osg::CullSettings::CLEAR_MASK;
       return 0;
    }
    "
    HAVE_CULLSETTINGS_CLEAR_MASK)

# library required by simgear
# XXX This should go in a module and only run if simgear is not shared.

if(HAVE_UNISTD_H)
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_INCLUDE_PATH})
check_cxx_source_compiles(
   "#include <unistd.h>
    #if !defined(_POSIX_TIMERS) || (0 >= _POSIX_TIMERS)
    #error clock_gettime is not supported
    #endif

    int main() { return 0; }
    "
    HAVE_CLOCK_GETTIME)    
endif(HAVE_UNISTD_H)

set(RT_LIBRARY "")
if(HAVE_CLOCK_GETTIME)
check_function_exists(clock_gettime CLOCK_GETTIME_IN_LIBC)
if(NOT CLOCK_GETTIME_IN_LIBC)
check_library_exists(rt clock_gettime "" HAVE_RT)
if(HAVE_RT)
set(RT_LIBRARY rt)
endif(HAVE_RT)
endif(NOT CLOCK_GETTIME_IN_LIBC)
endif(HAVE_CLOCK_GETTIME)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(WARNING_FLAGS -Wall)
endif(CMAKE_COMPILER_IS_GNUCXX)

if(WIN32)

    if(MSVC)
        # turn off various warnings
        # foreach(warning 4244 4251 4267 4275 4290 4786 4305 4996)
        #     SET(WARNING_FLAGS "${WARNING_FLAGS} /wd${warning}")
        # endforeach(warning)
        
        set(MSVC_FLAGS "-DNOMINMAX -D_USE_MATH_DEFINES -D_CRT_SECURE_NO_WARNINGS -D__CRT_NONSTDC_NO_WARNINGS")
    endif(MSVC)

    set(NOMINMAX 1)
endif(WIN32)    

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS} ${MSVC_FLAGS} -D_REENTRANT")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS} ${MSVC_FLAGS} -D_REENTRANT")

include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS} 
    ${Boost_INCLUDE_DIRS} 
	${ZLIB_INCLUDE_DIR} 
    ${ALUT_INCLUDE_DIR}
	${OPENAL_INCLUDE_DIR}
	${SIMGEAR_INCLUDE_DIR}
	${PLIB_INCLUDE_DIR} )

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_BINARY_DIR}/src/Include)

add_definitions(-DHAVE_CONFIG_H)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/src/Include/config_cmake.h.in"
  "${PROJECT_BINARY_DIR}/src/Include/config.h"
  )
  
add_subdirectory(src)
add_subdirectory(utils)
