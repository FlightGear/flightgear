Getting started with CMake
==========================

[For Windows build instructions see README.msvc]

(These instructions apply to Unix-like systems, including Cygwin and Mac. To
build using Visual Studio or some other IDE supported by CMake, most of the
information below still applies)

Always compile in a separate directory to the code. For example, if the
code (eg, from Git) is at /home/curt/projects/flightgear, you might create
/home/curt/projects/fgbuild. Change into the new directory, and run

    cmake ../flightgear

To generate standard Unix Makefiles in fgbuild.

Probably you want to specify an install prefix:

    cmake ../flightgear -DCMAKE_INSTALL_PREFIX=/usr

Note the install prefix is automatically searched for required libraries
and header files, so if you install PLIB, OpenSceneGraph and SimGear to the
same prefix, most configuration options are unnecessary.

If for some reason you have a dependency (or several) at a different prefix,
you can specify one or more via CMAKE_PREFIX_PATH:

    cmake ../flightgear -DCMAKE_PREFIX_PATH="/opt/local;/opt/fgfs"

(note the use of semi-colons to specify multiple prefix paths)

Standard prefixes are searched automatically (/usr, /usr/local, /opt/local)

Most dependencies also expose an environment variable to specify their
installation directory explicitly eg OSG_DIR or PLIBDIR. Any of the methods
described above will work, but specifying an INSTALL_PREFIX or PREFIX_PATH is
usually simpler.

By default, we select a release build. To create a debug build, use

    cmake ../flightgear -DCMAKE_BUILD_TYPE=Debug

(or MinSizeRel, or RelWithDbg)

Debug builds will automatically use corresponding debug builds of required
libraries, if they are available. For example you can install debug builds of
SimGear and OpenSceneGraph, and a debug FlightGear build will use them.

(Debug builds of libraries have the 'd' suffix by default - Release builds
have no additional suffix)

Note most IDE projects (eg Xcode and Visual Studio) support building all the
build types from the same project, so you can omit the CMAKE_BUILD_TYPE option
when running cmake, and simply pick the build configuration as normal in the
IDE.

It's common to have several build directories with different build
configurations, eg

    /home/curt/projects/flightgear (the git clone)
    /home/curt/projects/fgdebug
    /home/curt/projects/fgrelease
    /home/curt/projects/fg-with-svn-osg

To set an optional feature, do

    cmake ../flightgear -DFEATURE_NAME=ON

(or 'OFF' to disable )

To see the variables that can be configured / are currently defined, you can
run one of the GUI front ends, or the following command:

    cmake ../flightgear -L

Add 'A' to see all the options (including advanced options), or 'H' to see
the help for each option (similar to running configure --help under autoconf):

    cmake ../flightgear -LH


Build Targets
=============

For a Unix makefile build, 'make dist', 'make uninstall' and 'make test' are
all available and should work as expected. 'make clean' is also as normal,
but there is *no* 'make distclean' target. The equivalent is to completely
remove your build directory, and start with a fresh one.

Adding new files to the build

Add source files to the SOURCES list, and headers to the HEADERS list. Note
technically you only need to add source files, but omitting headers confuses
project generation and distribution / packaging targets.

For target conditional files, you can append to the SOURCES or HEADERS lists
inside an if() test, for example:

    if(APPLE)
        list(APPEND SOURCES extraFile1.cxx extraFile2.cxx)
    endif()

Setting include directories

In any CMakeList.txt, you can do the following:

    include_directories(${PROJECT_SOURCE_DIR}/some/path)

For example, this can be done in particular subdirectory, or at the project
root, or an intermediate level.

Setting target specific compile flags, includes or defines

Use set_target_property(), for example

    set_target_property(fgfs PROPERTIES
            COMPILE_DEFINITIONS FOO BAR=1)

You can set a property on an individual source file:

    set_property(SOURCE myfile.cxx PROPERTY COMPILE_FLAGS "-Wno-unsigned-compare")

Detecting Features / Libraries

For most standard libraries (Gtk, wxWidget, Python, GDAL, Qt, libXml, Boost),
cmake provides a standard helper. To see the available modules, run:

     cmake --help-module-list

In the root CMakeLists file, use a statement like:

    find_package(OpenGL REQUIRED)

Each package helper sets various variables such aaa_FOUND, aaa_INCLUDE_DIR,
and aaa_LIBRARY. Depending on the complexity of the package, these variables
might have different names (eg, OPENSCENEGRAPH_LIBRARIES).

If there's no standard helper for a library you need, find a similar one, copy
it to CMakeModules/FindABC.cmake, and modify the code to fit. Generally this
is pretty straightforward. The built-in modules reside in the Cmake 'share'
directory, eg /usr/share/cmake/modules on Unix systems.

Note libraries support by pkg-config can be handled directly, with no need
to create a custom FindABC helper.

Adding a new executable target

    add_executable(myexecutable ${SOURCES} ${HEADERS})
    target_link_libraries(myexecutable .... libraries ... )
    install(TARGETS myexecutable RUNTIME DESTINATION bin)

(If the executable should not be installed, omit the final line above)

If you add an additional line

    add_test(testname ${EXECUTABLE_OUTPUT_PATH}/myexecutable)

Then running 'make test' will run your executable as a unit test. The
executable should return either a success or failure result code.

