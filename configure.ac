dnl Process this file with autoget.sh to produce a working configure
dnl script.
dnl
dnl $Id$

AC_INIT
AC_CONFIG_SRCDIR([src/Aircraft/aircraft.cxx])

dnl Require at least automake 2.52
AC_PREREQ(2.52)

dnl Initialize the automake stuff
AM_INIT_AUTOMAKE(FlightGear, 0.9.2)

dnl Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_LN_S

# specify the simgear location
AC_ARG_WITH(simgear, [  --with-simgear=PREFIX   Specify the prefix path to simgear])

if test "x$with_simgear" != "x" ; then
    echo "SimGear prefix path is $with_simgear"
    EXTRA_DIRS="${EXTRA_DIRS} $with_simgear"
fi

dnl set the $host variable based on local machine/os
AC_CANONICAL_HOST

case "${host}" in
*-*-irix*)
    if test "$CXX" = "CC"; then
        AR="CC -ar"
        ARFLAGS="-o"
        CXXFLAGS="$CXXFLAGS -I$with_simgear/include/simgear/compatibility"
    else
        AR="ar"
        ARFLAGS="cru"
    fi
    ;;
*)
    AR="ar"
    ARFLAGS="cru"
    ;;
esac
AC_SUBST(AR)
AC_SUBST(ARFLAGS)

dnl set logging; default value of with_logging=yes
AC_ARG_WITH(logging, [  --with-logging          Include logging output (default)])
if test "x$with_logging" = "xno" ; then
    AC_DEFINE([FG_NDEBUG], 1, [Define for no logging output])
fi

# Specify if we want to build with Multiplayer support
# default to with_network=yes
AC_ARG_WITH(multiplayer, [  --with-multiplayer      Include default multiplayer support])
AC_ARG_WITH(network_olk, [  --with-network-olk      Include Oliver's multi-pilot networking support [default=no]])

if test "x$with_multiplayer" = "xno" -a "x$with_network_olk" = "xno"; then
  echo "Building without any kind of multiplayer support"

elif test "x$with_multiplayer" = "xno"; then
# echo "Building without default multiplayer support"
  echo "Building with Oliver's multi-pilot network support"
  AC_DEFINE([FG_NETWORK_OLK], 1, [Define to build with Oliver's networking])

else
  echo "Building with default multiplayer support"
# echo "Building without Oliver's multi-pilot network support"
  AC_DEFINE([FG_MPLAYER_AS], 1, [Define to build with default multiplayer support])
fi

AM_CONDITIONAL(ENABLE_NETWORK_OLK, test "x$with_network_olk" != "xno" -a "x$with_multiplayer" = "xno")
AM_CONDITIONAL(ENABLE_MPLAYER_AS, test "x$with_multiplayer" != "xno")


# Specify if we want to use WeatherCM instead of FGEnvironment.
# default to with_weathercm=no
AC_ARG_WITH(weathercm, [  --with-weathercm        Use WeatherCM instead of FGEnvironment])
if test "x$with_weathercm" = "xyes" ; then
    echo "Building with WeatherCM"
    AC_DEFINE([FG_WEATHERCM], 1,
              [Define to build with Christian Mayer's weather code])
else
    echo "Building with FGEnvironment"
fi
AM_CONDITIONAL(ENABLE_WEATHERCM, test "x$with_weathercm" = "xyes")

dnl Thread related checks
AC_ARG_WITH(threads, [  --with-threads          Include tile loading threads [default=no]])
if test "x$with_threads" = "xyes"; then
    AC_DEFINE([ENABLE_THREADS], 1, [Define to enable threaded tile paging])
    CXXFLAGS="$CXXFLAGS -D_REENTRANT"
    CFLAGS="$CFLAGS -D_REENTRANT"
fi
AC_CHECK_HEADER(pthread.h)
AM_CONDITIONAL(WITH_THREADS, test "x$with_threads" = "xyes")

# specify the plib location
AC_ARG_WITH(plib, [  --with-plib=PREFIX      Specify the prefix path to plib])

if test "x$with_plib" != "x" ; then
    echo "plib prefix is $with_plib"
    EXTRA_DIRS="${EXTRA_DIRS} $with_plib"
fi

# specify the metakit location
AC_ARG_WITH(metakit, [  --with-metakit=PREFIX   Specify the prefix path to metakit])

if test "x$with_metakit" != "x" ; then
    echo "metakit prefix is $with_metakit"
    EXTRA_DIRS="${EXTRA_DIRS} $with_metakit"
fi

dnl Used by JSBSim to conditionally compile in fgfs interface code
AC_DEFINE([FGFS], 1, [Define so that JSBSim compiles in 'library' mode])

dnl Used to controll whether clouds3d should be compiled in or not
AC_DEFINE([FG_USE_CLOUDS_3D], 1, [Define to use 3D cloud support])
AM_CONDITIONAL(FG_USE_CLOUDS_3D, test "x" = "x")

# Check for MS Windows environment
AC_CHECK_HEADER(windows.h)

dnl Determine an extra directories to add to include/lib search paths
case "${host}" in
*-apple-darwin* | *-*-mingw32*)
    echo no EXTRA_DIRS for $host
    ;;

*)
    if test -d /usr/X11R6 ; then
        EXTRA_DIR1="/usr/X11R6"
    fi
    if test -d /opt/X11R6 ; then
        EXTRA_DIR2="/opt/X11R6"
    fi
    EXTRA_DIRS="${EXTRA_DIRS} $EXTRA_DIR1 $EXTRA_DIR2"
    ;;

esac
wi_EXTRA_DIRS(no, ${EXTRA_DIRS})


dnl Using AM_CONDITIONAL is a step out of the protected little 
dnl automake fold so it is potentially dangerous.  But, we are
dnl beginning to run into cases where the standard checks are not
dnl enough.  AM_CONDITIONALS are then referenced to conditionally
dnl build a Makefile.in from a Makefile.am which lets us define custom
dnl includes, compile alternative source files, etc.

dnl Check for external variables daylight and timezone.
AC_EXT_DAYLIGHT
if test "$have_daylight" = yes; then
    AC_DEFINE([HAVE_DAYLIGHT], 1, [Define if system has daylight variable])
fi

AC_EXT_TIMEZONE
if test "$have_timezone" = yes; then
    AC_DEFINE([HAVE_TIMEZONE], 1, [Define if system has timezone variable])
fi

dnl add correct audio libs and configure for audio support
audio_LIBS="-lplibsl -lplibsm"
case "${host}" in
*-*-cygwin* | *-*-mingw32*)
    audio_LIBS="$audio_LIBS -lwinmm"
    ;;
*-apple-darwin*)
    audio_LIBS="$audio_LIBS -framework IOKit -framework CoreFoundation"
    ;;
*-*-irix* )
    audio_LIBS="$audio_LIBS -laudio"
    ;;

esac
AC_SUBST(audio_LIBS)

dnl ENABLE_AUDIO_SUPPORT could be depricated at any time in favor of
dnl just assuming we have audio support on all platform.  We can
dnl depend on plib to hide the details.
AC_DEFINE([ENABLE_AUDIO_SUPPORT], 1, [Define for audio support])

dnl X11 might be installed on Mac OS X or cygwin/mingwin, we don't want
dnl to use it if it is.
case "${host}" in
*-apple-darwin* | *-*-cygwin* | *-*-mingw32*)
    echo no fancy X11 check
    ;;

*)
    AC_PATH_XTRA
    ;;

esac

dnl Checks for libraries.

dnl Thread related checks
AC_CHECK_LIB(pthread, pthread_exit)
if test "x$ac_cv_lib_pthread_pthread_exit" != "xyes" \
   -a "x$ac_cv_header_pthread_h" = "xyes"; then
    dnl FreeBSD: System has pthread.h, but -lpthread library check
    dnl fails.  See if we need -pthread instead of -lpthread and look
    dnl for the functions in libc_r.
    save_CXXFLAGS="$CXXFLAGS"
    save_CFLAGS="$CFLAGS"
    CXXFLAGS="-pthread $CXXFLAGS"
    CFLAGS="-pthread $CFLAGS"
    save_LIBS=$LIBS
    AC_CHECK_LIB(c_r, pthread_exit)
    if test "x$ac_cv_lib_c_r_pthread_exit" != "xyes"; then
        CXXFLAGS=$save_CXXFLAGS
        CFLAGS=$save_CFLAGS
    fi
    LIBS=$save_LIBS
fi

thread_LIBS="$LIBS"
LIBS=""

dnl search for network related libraries
AC_SEARCH_LIBS(inet_addr, xnet)
AC_SEARCH_LIBS(socket, socket)
AC_SEARCH_LIBS(main, nsl)

network_LIBS="$LIBS"
LIBS=""

dnl check for some default libraries
AC_SEARCH_LIBS(cos, m)
AC_SEARCH_LIBS(dlclose, dl)

base_LIBS="$LIBS"

dnl check for glut location
AC_CHECK_HEADER(GL/glut.h)
if test "x$ac_cv_header_GL_glut_h" = "xyes"; then
    AC_DEFINE([GLUT_H], <GL/glut.h>, [Define as glut.h include location])
else
    AC_CHECK_HEADER(GLUT/glut.h)
    if test "x$ac_cv_header_GLUT_glut_h" = "xyes"; then
        AC_DEFINE([GLUT_H], <GLUT/glut.h>, [Define as glut.h include location])
    else
        echo "Neither GL/glut.h nor GLUT/glut.h found.  Cannot continue"
        exit
    fi
fi

dnl check for OpenGL related libraries
case "${host}" in
*-*-cygwin* | *-*-mingw32*)
    dnl CygWin under Windoze.

    echo Win32 specific hacks...
    AC_DEFINE([WIN32], 1, [Define for Win32 platforms])
    AC_DEFINE([NOMINMAX], 1, [Define for Win32 platforms])

    LIBS="$LIBS -lglut32 -lglu32 -lopengl32"
    LIBS="$LIBS -luser32 -lgdi32"

    dnl add -lwsock32 for mingwin
    case "${host}" in
    *-*-mingw32*)
        echo "********** BUILDING FOR MINGW32 *************"
        base_LIBS="$base_LIBS -lws2_32"
        ;;
    esac

    echo "Will link apps with $LIBS"
    ;;


*-apple-darwin*)
    dnl Mac OS X

    LIBS="$LIBS -framework GLUT -framework OpenGL -framework Carbon -lobjc"
    ;;

*)
    dnl X-Windows based machines

    dnl Skip X11 library tests if requested.
    if test "x$no_x" != "xyes"; then
        AC_SEARCH_LIBS(XCreateWindow, X11)
        AC_SEARCH_LIBS(XShmCreateImage, Xext)
        AC_SEARCH_LIBS(XGetExtensionVersion, Xi)
        AC_SEARCH_LIBS(IceOpenConnection, ICE)
        AC_SEARCH_LIBS(SmcOpenConnection, SM)
        AC_SEARCH_LIBS(XtMalloc, Xt)
        AC_SEARCH_LIBS(XmuLookupStandardColormap, Xmu)
    fi

    AC_SEARCH_LIBS(glNewList, [ GL GLcore MesaGL ])
    if test "x$ac_cv_search_glNewList" = "x-lGLcore"; then
        dnl if GLcore found, then also check for GL
        AC_SEARCH_LIBS(glXCreateContext, GL)
    fi

    dnl if using mesa, check for xmesa.h
    if test "x$ac_cv_search_glNewList" = "x-lMesaGL"; then
        AC_CHECK_HEADER(GL/fxmesa.h)
        if test "x$ac_cv_header_GL_fxmesa_h" = "xyes"; then
            AC_DEFINE([XMESA], 1, [Define for fxmesa])
            AC_DEFINE([FX], 1, [Define for fxmesa])
        fi
    fi

    AC_SEARCH_LIBS(gluLookAt, [ GLU MesaGLU ])
    AC_SEARCH_LIBS(glutGetModifiers, [ glut freeglut ])
    ;;

esac

opengl_LIBS="$LIBS"
LIBS="$base_LIBS"

AC_SUBST(base_LIBS)
AC_SUBST(opengl_LIBS)
AC_SUBST(thread_LIBS)
AC_SUBST(network_LIBS)

# The following are C++ items that need to be tested for with the c++
# compiler

AC_LANG_PUSH(C++)

# Check for "plib" without which we cannot go on
AC_CHECK_HEADER(plib/ul.h)
if test "x$ac_cv_header_plib_ul_h" != "xyes"; then
    echo
    echo "You *must* have the plib library installed on your system to build"
    echo "the FGFS simulator!"
    echo
    echo "Please see README.plib for more details."
    echo
    echo "configure aborted."
    exit
fi

AC_MSG_CHECKING([for plib 1.6.0 or newer])
AC_TRY_RUN([
#include <plib/ul.h>

#define MIN_PLIB_VERSION 160

int main() {
    int major, minor, micro;

    if ( PLIB_VERSION < MIN_PLIB_VERSION ) {
	 return -1;
    }

    return 0;
}

],
  AC_MSG_RESULT(yes),
  [AC_MSG_RESULT(wrong version);
   AC_MSG_ERROR([Install plib 1.6.0 or later first...])],
  AC_MSG_RESULT(yes)
)

dnl If we get here then plib is available, so force use of plib
dnl joystick lib
AC_DEFINE([ENABLE_PLIB_JOYSTICK], 1, [Define to enable plib joystick support])


dnl Checking for PSL in plib
dnl version of plib, so check for it.
AC_MSG_CHECKING([for plib PSL scripting support])
AC_COMPILE_IFELSE([
#include <plib/psl.h>

int main() {
  // not for running...
  pslProgram program((pslExtension *)0);
  program.compile("int main () {}", "");
  return 0;
}
],
  [AC_MSG_RESULT(yes);
   have_plib_psl=yes],
  AC_MSG_RESULT(no)
)
AM_CONDITIONAL(HAVE_PLIB_PSL, test "x$have_plib_psl" = "xyes")
if test "x$have_plib_psl" = "xyes"; then
   AC_DEFINE([HAVE_PLIB_PSL], 1,
             [Define if plib version is new enough to provide "PSL"])
fi

dnl Check for MetaKit
AC_CHECK_HEADER(mk4.h)
if test "x$ac_cv_header_mk4_h" != "xyes"; then
    echo
    echo "MetaKit library not found."
    echo
    echo "If your OS does not provide an installable package for MetaKit"
    echo "you will have to compile and install it first yourself.  A copy"
    echo "of metakit-$(VERSION).tar.gz is included with SimGear.  You will"
    echo "have to untar this source code, and follow it's included instructions"
    echo "to compile and install on your system."
    echo
    echo "configure aborted."
    exit
fi

AC_MSG_CHECKING([for metakit 2.4.3 or newer])
saved_LIBS="$LIBS"
LIBS="$saved_LIBS -lmk4"
AC_TRY_RUN([
#include <mk4.h>

#define MIN_MK4_VERSION 243

int main() {
    int major, minor, micro;

    if ( d4_MetaKitLibraryVersion < MIN_MK4_VERSION ) {
	 return -1;
    }

    return 0;
}

],
  AC_MSG_RESULT(yes),
  [AC_MSG_RESULT(wrong version);
   AC_MSG_ERROR([

Install metakit 2.4.3 or later first.

Or, the compiler may not be finding your libmk4.so library.
Please check the config.log file for specific details of the
failure if you believe you have the correct metakit version.
Also, look up this issue in the FlightGear FAQ.])],
  AC_MSG_RESULT(yes)
)

dnl Check for the presence of SimGear
AC_CHECK_HEADER(simgear/version.h)
if test "x$ac_cv_header_simgear_version_h" != "xyes"; then
    echo
    echo "You *must* have the SimGear support library installed on your system"
    echo "to build the FGFS simulator!"
    echo
    echo "Please see README.SimGear for more details."
    echo
    echo "configure aborted."
    exit
fi

AC_MSG_CHECKING([for simgear 0.3.3 or newer])
AC_TRY_RUN([
#include <stdio.h>

#include <simgear/version.h>

#define STRINGIFY(X) XSTRINGIFY(X)
#define XSTRINGIFY(X) #X

#define MIN_MAJOR 0
#define MIN_MINOR 3
#define MIN_MICRO 3

int main() {
    int major, minor, micro;

    printf("%d.%d.%d or greater... ", MIN_MAJOR, MIN_MINOR, MIN_MICRO);

    sscanf( STRINGIFY(SIMGEAR_VERSION), "%d.%d.%d", &major, &minor, &micro );

    if ( major < MIN_MAJOR ) {
	 return -1;
    } else if ( major == MIN_MAJOR && minor < MIN_MINOR ) {
	 return -1;
    } else if ( major == MIN_MAJOR && minor == MIN_MINOR && micro < MIN_MICRO ){
	 return -1;
    }

    return 0;
}

],
  AC_MSG_RESULT(yes),
  [AC_MSG_RESULT(wrong version);
   AC_MSG_ERROR([Install latest simgear first...])],
  AC_MSG_RESULT(yes)
)

LIBS="$saved_LIBS"

dnl Check if SimGear was built with Norman JPEG factory support
AC_CHECK_HEADER(simgear/screen/jpgfactory.hxx)
if test "x$ac_cv_header_simgear_screen_jpgfactory_hxx" = "xyes"; then
    AC_CHECK_LIB(jpeg, jpeg_start_compress)
    if test "x$ac_cv_lib_jpeg_jpeg_start_compress" != "xyes" ; then
        echo
        echo "The JPEG factory code was built and installed with SimGear."
        echo "However it appears the libjpeg is no longer installed."
        echo "You need to install libjpeg or remove jpgfactory support from"
        echo "SimGear"
        echo
        echo "libjpeg is available at :"
        echo "  ftp://ftp.uu.net in the directory graphics/jpeg"
	exit 1
    fi
    AC_DEFINE([FG_JPEG_SERVER], 1,
              [Define to build with jpeg screen shot server])
fi
AM_CONDITIONAL(ENABLE_JPEG_SERVER, test "x$ac_cv_header_simgear_screen_jpgfactory_hxx" = "xyes")

AC_LANG_POP

dnl Check for system installed zlib
AC_CHECK_HEADER(zlib.h)
if test "x$ac_cv_header_zlib_h" != "xyes"; then
    echo
    echo "zlib library not found."
    echo
    echo "If your OS does not provide an installable package for zlib"
    echo "you will have to compile and install it first yourself.  A copy"
    echo "of zlib-1.1.4.tar.gz is included with SimGear.  You will"
    echo "have to untar this source code, and follow it's included instructions"
    echo "to compile and install on your system."
    echo
    echo "configure aborted."
    echo
fi

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS( \
	fcntl.h getopt.h malloc.h memory.h stdlib.h sys/param.h sys/stat.h \
        sys/time.h sys/timeb.h unistd.h windows.h values.h )

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

dnl Checks for library functions.
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS( \
        ftime gettimeofday timegm memcpy bcopy mktime strstr rand mkfifo \
	random drand48 setitimer getitimer signal GetLocalTime rint getrusage )

AM_CONFIG_HEADER(src/Include/config.h)

AC_CONFIG_FILES([ \
	Makefile \
	man/Makefile \
	man/est-epsilon.1 \
	man/fgfs.1 \
	man/fgjs.1 \
	man/gl-info.1 \
	man/js_demo.1 \
	man/pstest.1 \
        scripts/Makefile \
        scripts/debug/Makefile \
        scripts/python/Makefile \
	src/Makefile \
	src/Include/Makefile \
	src/Include/version.h \
	src/Aircraft/Makefile \
	src/Airports/Makefile \
	src/ATC/Makefile \
	src/Autopilot/Makefile \
	src/Cockpit/Makefile \
	src/Cockpit/built_in/Makefile \
	src/Controls/Makefile \
	src/Environment/Makefile \
	src/FDM/Balloon/Makefile \
	src/FDM/ExternalNet/Makefile \
	src/FDM/ExternalPipe/Makefile \
	src/FDM/JSBSim/Makefile \
	src/FDM/JSBSim/filtersjb/Makefile \
	src/FDM/LaRCsim/Makefile \
	src/FDM/UIUCModel/Makefile \
	src/FDM/YASim/Makefile \
	src/FDM/Makefile \
	src/GUI/Makefile \
	src/Input/Makefile \
	src/Instrumentation/Makefile \
	src/Main/Makefile \
	src/Main/runfgfs \
	src/Main/runfgfs.bat \
	src/Model/Makefile \
	src/MultiPlayer/Makefile \
	src/Navaids/Makefile \
	src/Network/Makefile \
	src/NetworkOLK/Makefile \
	src/Objects/Makefile \
	src/Replay/Makefile \
	src/Scenery/Makefile \
	src/Scripting/Makefile \
	src/Sound/Makefile \
	src/Systems/Makefile \
	src/Time/Makefile \
	src/WeatherCM/Makefile \
	tests/Makefile \
	utils/Makefile \
	utils/TerraSync/Makefile \
])
AC_OUTPUT

AC_OUTPUT_COMMANDS([chmod 755 src/Main/runfgfs])

echo ""
echo "Configure Summary"
echo "================="

echo "Prefix: $prefix"

if test "x$have_plib_psl" = "xyes"; then
    echo "Plib PSL scripting: yes"
else
    echo "Plib PSL scripting: $fg_psl_string_compile"
    echo "  You will not be able to run scripts written in PSL"
    echo "  and some advanced GUI and input features may not"
    echo "  work.  Download and install the latest CVS version"
    echo "  of plib if you would like to enable PSL support."
fi

if test "x$with_logging" != "x"; then
    echo "Debug messages: $with_logging"
else
    echo "Debug messages: yes"
fi

echo -n "Automake version: "
automake --version | head -1

if test "x$with_weathercm" != "x"; then
    echo "WeatherCM instead of FGEnvironment: $with_weathercm"
else
    echo "Using FGEnvironment"
fi

if test "x$with_multiplayer" != "xno"; then
    echo "Using default multiplayer support"
elif test "x$with_network_olk" != "xno"; then
    echo "Using Oliver's multi-pilot network support"
fi

if test "x$with_threads" = "xyes"; then
    echo "threads: yes"
else
    echo "threads: no"
fi
