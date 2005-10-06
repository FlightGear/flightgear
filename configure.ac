dnl Process this file with autoget.sh to produce a working configure
dnl script.

AC_INIT
AC_CONFIG_SRCDIR([src/Aircraft/aircraft.cxx])

dnl Require at least automake 2.52
AC_PREREQ(2.52)

dnl Initialize the automake stuff
dnl set the $host variable based on local machine/os
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE(FlightGear, 0.9.8)

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

case "${host}" in
*-*-irix*)
    if test "$CXX" = "CC"; then
        AR="CC -ar"
        ARFLAGS="-o"
        CXXFLAGS="$CXXFLAGS -c99 -I$with_simgear/include/simgear/compatibility"
        CFLAGS="$CFLAGS -c99"
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
AC_ARG_WITH(multiplayer, [  --with-multiplayer      Include multiplayer support [default=yes]])

if test "x$with_multiplayer" = "xno"; then
  echo "Building without any kind of multiplayer support"
else
  echo "Building with multiplayer support"
  AC_DEFINE([FG_MPLAYER_AS], 1, [Define to build with multiplayer support])
fi
AM_CONDITIONAL(ENABLE_MPLAYER_AS, test "x$with_multiplayer" != "xno")

AC_ARG_ENABLE(sp_fdms, [  --enable-sp-fdms              Include special purpose Flight Models], [enable_sp_fdms="$enableval"] )
AC_DEFINE([ENABLE_SP_FMDS], test "x$enable_sp_fdms" = "xyes", [Define to include special purpose FDMs])
AM_CONDITIONAL(ENABLE_SP_FDM, test "x$enable_sp_fdms" != "xno")


dnl Thread related checks
# defaults to yes
with_threads=yes
AC_ARG_WITH(threads, [  --with-threads          Include tile loading threads [default=yes]])
if test "x$with_threads" != "xyes"; then
    AC_DEFINE([ENABLE_THREADS], 0, [Define to enable threaded tile paging])
else
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

dnl Used by JSBSim to conditionally compile in fgfs interface code
AC_DEFINE([FGFS], 1, [Define so that JSBSim compiles in 'library' mode])

# Check for MS Windows environment
AC_CHECK_HEADER(windows.h)

dnl Determine an extra directories to add to include/lib search paths
case "${host}" in
*-apple-darwin* | *-*-mingw32*)
    echo no EXTRA_DIRS for $host
    ;;

*-*-cygwin*)
    if test -d /usr/local ; then
        EXTRA_DIRS="${EXTRA_DIRS} /usr/local"
    fi
    ;;

*)
    if test -d /usr/X11R6 ; then
        EXTRA_DIR1="/usr/X11R6"
    fi
    if test -d /opt/X11R6 ; then
        EXTRA_DIR2="/opt/X11R6"
    fi
    EXTRA_DIRS="${EXTRA_DIRS} $EXTRA_DIR1 $EXTRA_DIR2 /usr/local/"
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


dnl add joystick support libraries
dnl search for FreeBSD library
AC_SEARCH_LIBS(hid_init, usbhid)
joystick_LIBS="$LIBS"
LIBS=""


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
AC_SEARCH_LIBS(pthread_cancel, [pthread c_r])
if test "x$ac_cv_header_pthread_h" = "xyes"; then
  if test "x$ac_cv_search_pthread_exit" = "x-lc_r"; then
    CXXFLAGS="-pthread $CXXFLAGS"
    CFLAGS="-pthread $FLAGS"
  fi
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
AC_SEARCH_LIBS(cos, [fastm m])
AC_SEARCH_LIBS(dlclose, dl)

base_LIBS="$LIBS"

dnl Check for SDL if enabled.
AC_ARG_ENABLE(sdl, [  --enable-sdl                  Configure to use SDL instead of GLUT], [enable_sdl="$enableval"])
AM_CONDITIONAL(USE_SDL, test "x$enable_sdl" = "xyes")
if test "x$enable_sdl" = "xyes"; then
    AC_DEFINE([PU_USE_SDL], 1, [Define to use SDL])
else
    AC_DEFINE([PU_USE_GLUT], 1, [Define to use glut])
fi

dnl check for OpenGL related libraries
case "${host}" in
*-*-cygwin* | *-*-mingw32*)
    dnl CygWin under Windoze.

    echo Win32 specific hacks...
    AC_DEFINE([WIN32], 1, [Define for Win32 platforms])
    AC_DEFINE([NOMINMAX], 1, [Define for Win32 platforms])

    if test "x$enable_sdl" = "xyes"; then
        AC_SEARCH_LIBS(SDL_Init, SDL)
    else
        LIBS="$LIBS -lglut32"
    fi

    LIBS="$LIBS -lglu32 -lopengl32 -luser32 -lgdi32"
    joystick_LIBS="$joystick_LIBS -lwinmm"

    dnl add -lwsock32 for mingwin
    case "${host}" in
    *-*-mingw32*)
        echo "********** BUILDING FOR MINGW32 *************"
        network_LIBS="$network_LIBS -lws2_32"
        ;;
    esac

    echo "Will link apps with $LIBS"
    ;;


*-apple-darwin*)
    dnl Mac OS X

    LIBS="$LIBS -framework GLUT -framework OpenGL -framework AGL -framework Carbon -lobjc"
    joystick_LIBS="$joystick_LIBS -framework IOKit -framework CoreFoundation"
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
    if test "x$enable_sdl" = "xyes"; then
        AC_SEARCH_LIBS(SDL_Init, SDL)
    fi
    ;;

esac

opengl_LIBS="$LIBS"
LIBS="$base_LIBS $joystick_LIBS"

dnl check for OpenAL libraries
case "${host}" in
*-*-cygwin* | *-*-mingw32*)
    dnl CygWin under Windoze.

    AC_SEARCH_LIBS(alGenBuffers, openal32)
    AC_SEARCH_LIBS(alutInit, [ openal32 ALut ] )
    LIBS="$LIBS -ldsound -ldxguid -lole32"
    openal_LIBS="$LIBS"
    ;;

*-apple-darwin*)
    dnl Mac OS X

    LIBS="$LIBS -framework IOKit -framework OpenAL"
    openal_LIBS="$LIBS"
    ;;

*)
    dnl default unix style machines

    save_LIBS=$LIBS
    LIBS="$LIBS $thread_LIBS"
    AC_SEARCH_LIBS(alGenBuffers, openal)
    AC_SEARCH_LIBS(alutInit, [ alut openal ] )
    OPENAL_OK="$ac_cv_search_alGenBuffers"
    openal_LIBS="$LIBS"
    LIBS=$save_LIBS
    ;;

esac

LIBS="$base_LIBS"

AC_SUBST(base_LIBS)
AC_SUBST(openal_LIBS)
AC_SUBST(opengl_LIBS)
AC_SUBST(thread_LIBS)
AC_SUBST(network_LIBS)
AC_SUBST(joystick_LIBS)

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

AC_MSG_CHECKING([for plib 1.8.4 or newer])
AC_TRY_RUN([
#include <plib/ul.h>

#define MIN_PLIB_VERSION 184

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
   AC_MSG_ERROR([Install plib 1.8.0 or later first...])],
  AC_MSG_RESULT(yes)
)

dnl If we get here then plib is available, so force use of plib
dnl joystick lib
AC_DEFINE([ENABLE_PLIB_JOYSTICK], 1, [Define to enable plib joystick support])


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

AC_MSG_CHECKING([for simgear 0.3.8 or newer])
AC_TRY_RUN([
#include <stdio.h>

#include <simgear/version.h>

#define STRINGIFY(X) XSTRINGIFY(X)
#define XSTRINGIFY(X) #X

#define MIN_MAJOR 0
#define MIN_MINOR 3
#define MIN_MICRO 8

int main() {
    int major, minor, micro;

    /* printf("%d.%d.%d or greater, ", MIN_MAJOR, MIN_MINOR, MIN_MICRO); */
    printf("[found %s] ... ", STRINGIFY(SIMGEAR_VERSION));

    sscanf( STRINGIFY(SIMGEAR_VERSION), "%d.%d.%d", &major, &minor, &micro );

    if ( (major < MIN_MAJOR) ||
         (major == MIN_MAJOR && minor < MIN_MINOR) ||
         (major == MIN_MAJOR && minor == MIN_MINOR && micro < MIN_MICRO) ) {
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

dnl Check if SimGear was built with Norman's JPEG factory support
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
old_LIBS=$LIBS
LIBS="$base_LIBS $network_LIBS $opengl_LIBS"
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS( [ \
        ftime gettimeofday timegm memcpy bcopy mktime strstr rand mkfifo \
	random drand48 setitimer getitimer signal GetLocalTime rint getrusage \
	truncf ] )
LIBS=$old_LIBS

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
        scripts/perl/Makefile \
        scripts/perl/examples/Makefile \
        scripts/python/Makefile \
	src/Makefile \
	src/Include/Makefile \
	src/Include/version.h \
	src/Include/config.h-msvc6 \
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
	src/FDM/SP/Makefile \
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
	src/AIModel/Makefile \
	src/MultiPlayer/Makefile \
	src/Navaids/Makefile \
	src/Network/Makefile \
	src/Objects/Makefile \
	src/Replay/Makefile \
	src/Scenery/Makefile \
	src/Scripting/Makefile \
	src/Sound/Makefile \
	src/Systems/Makefile \
	src/Time/Makefile \
	src/Traffic/Makefile \
	tests/Makefile \
	utils/Makefile \
	utils/GPSsmooth/Makefile \
	utils/js_server/Makefile \
	utils/Modeller/Makefile \
	utils/TerraSync/Makefile \
	utils/xmlgrep/Makefile \
])
AC_OUTPUT

AC_OUTPUT_COMMANDS([chmod 755 src/Main/runfgfs])

echo ""
echo "Configure Summary"
echo "================="

echo "Prefix: $prefix"

if test "x$with_logging" != "x"; then
    echo "Debug messages: $with_logging"
else
    echo "Debug messages: yes"
fi

echo -n "Automake version: "
automake --version | head -1

if test "x$with_multiplayer" != "xno"; then
    echo "Building with multiplayer support"
fi

if test "x$with_threads" = "xyes"; then
    echo "threads: yes"
else
    echo "threads: no"
fi

if test "x$enable_sp_fdms" != "xno"; then
    echo "Include special purpose flight models: yes"
else
    echo "Include special purpose flight models: no"
fi

