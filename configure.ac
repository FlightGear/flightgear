dnl Process this file with autoget.sh to produce a working configure
dnl script.

AC_INIT
AC_CONFIG_SRCDIR([src/Aircraft/aircraft.cxx])

dnl Require at least automake 2.52
AC_PREREQ(2.52)

dnl Initialize the automake stuff
dnl set the $host variable based on local machine/os
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE(FlightGear, 1.99.5)

dnl Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_LN_S

# specify the simgear location
AC_ARG_WITH(simgear, [  --with-simgear=PREFIX   Specify the prefix path to SimGear])

if test "x$with_simgear" != "x" ; then
    echo "SimGear prefix path is $with_simgear"
    EXTRA_DIRS="${EXTRA_DIRS} $with_simgear"
    CXXFLAGS="$CXXFLAGS -I$with_simgear"
fi

# specify the plib location
AC_ARG_WITH(plib, [  --with-plib=PREFIX      Specify the prefix path to plib])

if test "x$with_plib" != "x" ; then
    echo "plib prefix is $with_plib"
    EXTRA_DIRS="${EXTRA_DIRS} $with_plib"
fi

# specify the osg location
AC_ARG_WITH(osg, [  --with-osg=PREFIX       Specify the prefix path to osg])

if test "x$with_osg" != "x" ; then
    echo "osg prefix is $with_osg"
    EXTRA_DIRS="${EXTRA_DIRS} $with_osg"
fi

# specify Boost location
AC_ARG_WITH(osg, [  --with-boost=PREFIX       Specify the prefix path to Boost])

if test "x$with_boost" != "x" ; then
    echo "Boost prefix is $with_boost"
    EXTRA_DIRS="${EXTRA_DIRS} $with_boost"
fi

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
    EXTRA_DIRS="${EXTRA_DIRS} $EXTRA_DIR1 $EXTRA_DIR2 /usr/local"
    ;;

esac
wi_EXTRA_DIRS(no, ${EXTRA_DIRS})


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

AC_ARG_ENABLE(sp_fdms, [  --enable-sp-fdms               Include special purpose Flight Models], [enable_sp_fdms="$enableval"] )
if test "x$enable_sp_fdms" != "xno"; then
    AC_DEFINE([ENABLE_SP_FDM], 1, [Define to include special purpose FDMs])
else
    AC_DEFINE([ENABLE_SP_FDM], 0, [Define to include special purpose FDMs])
fi
AM_CONDITIONAL(ENABLE_SP_FDM, test "x$enable_sp_fdms" != "xno")


dnl Thread related checks
# defaults to yes
AC_ARG_WITH(threads, [  --with-threads          Include tile loading threads [default=yes]], [], [with_threads=yes])
if test "x$with_threads" = "xyes"; then
    AC_DEFINE([ENABLE_THREADS], 1, [Define to enable threaded tile paging])
    CXXFLAGS="$CXXFLAGS -D_REENTRANT"
    CFLAGS="$CFLAGS -D_REENTRANT"
fi
AC_CHECK_HEADER(pthread.h)
AM_CONDITIONAL(WITH_THREADS, test "x$with_threads" = "xyes")

dnl Used by JSBSim to conditionally compile in fgfs interface code
AC_DEFINE([FGFS], 1, [Define so that JSBSim compiles in 'library' mode])

# Check for MS Windows environment
AC_CHECK_HEADER(windows.h)

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
AC_SEARCH_LIBS(pthread_create, [pthread c_r])
if test "x$ac_cv_header_pthread_h" = "xyes"; then
  if test "x$ac_cv_search_pthread_exit" = "x-lc_r"; then
    CXXFLAGS="-pthread $CXXFLAGS"
    CFLAGS="-pthread $CFLAGS"
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
AC_SEARCH_LIBS(sqrt, [am ffm fm fastm m])
AC_SEARCH_LIBS(ceil, m)
AC_SEARCH_LIBS(dlclose, dl)

base_LIBS="$LIBS"

dnl Check for SDL or glut if enabled.
AC_ARG_ENABLE(osgviewer, [  --enable-osgviewer                  Configure to use osgViewer(default)], [enable_osgviewer="$enableval"])
AC_ARG_ENABLE(sdl,  [  --enable-sdl                   Configure to use SDL], [enable_sdl="$enableval"])
AC_ARG_ENABLE(glut, [  --enable-glut                  Configure to use GLUT], [enable_glut="$enableval"])
AM_CONDITIONAL(USE_SDL, test "x$enable_sdl" = "xyes")
AM_CONDITIONAL(USE_GLUT, test "x$enable_glut" = "xyes")
if test "x$enable_sdl" != "xyes" -a "x$enable_glut" != "xyes" -a "x$enable_osgviewer" != "xno"; then
   enable_osgviewer="yes"
fi
if test \( "x$enable_osgviewer" = "xyes" \
   -a \( "x$enable_sdl" = "xyes" -o "x$enable_glut" = "xyes" \) \) \
   -o \( "x$enable_sdl" = "xyes" -a "x$enable_glut" = "xyes" \); then
   echo " Only one of --enable-osgviewer, --enable-sdl, or --enable -glut may"
   echo " be supplied."
   exit
fi
AC_DEFINE([PU_USE_NONE], 1, [Define to use application's pu callbacks])

AC_ARG_ENABLE(osgdebug, [  --enable-osgdebug              Use OSG debug libraries], [enable_osgdebug="$enableval"])

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
OPENAL_OK="no"
ALUT_OK="no"
case "${host}" in
*-*-cygwin* | *-*-mingw32*)
    dnl CygWin under Windoze.
    INCLUDES="$INCLUDES -I/usr/local/include/"
    LIBS="$LIBS -L/usr/local/lib"
    AC_SEARCH_LIBS(alGenBuffers, [ openal32 openal ] )
    AC_SEARCH_LIBS(alutInit, [ openal32 ALut alut ] )
    LIBS="$LIBS -lwinmm -ldsound -ldxguid -lole32"
    openal_LIBS="$LIBS"
    OPENAL_OK="$ac_cv_search_alGenBuffers"
    ALUT_OK="$ac_cv_search_alutInit"
    ;;

*-apple-darwin*)
    dnl Mac OS X

    LIBS="$LIBS -framework IOKit -framework OpenAL"
    openal_LIBS="$LIBS"
    # not sure how to test if OpenAL exists on MacOS (does it come by default?)
    OPENAL_OK="yes"
    ALUT_OK="yes"
    ;;

*)
    dnl default unix style machines

    save_LIBS=$LIBS
    LIBS="$LIBS $thread_LIBS"
    AC_SEARCH_LIBS(alGenBuffers, openal)
    AC_SEARCH_LIBS(alutInit, [ alut openal ] )
    OPENAL_OK="$ac_cv_search_alGenBuffers"
    ALUT_OK="$ac_cv_search_alutInit"
    openal_LIBS="$LIBS"
    LIBS=$save_LIBS
    ;;

esac

if test "$OPENAL_OK" == "no"; then
    echo
    echo "You *must* have the openal library installed on your system to build"
    echo "SimGear!"
    echo
    echo "Please see README.OpenAL for more details."
    echo
    echo "configure aborted."
    exit
fi

if test "$ALUT_OK" == "no"; then
    echo
    echo "You *must* have the alut library installed on your system to build"
    echo "SimGear!"
    echo
    echo "Please see README.OpenAL for more details."
    echo
    echo "configure aborted."
    exit
fi


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

dnl Check for the presence of SimGear
if test "x$with_simgear" != "x"; then
AC_CHECK_HEADER($with_simgear/include/simgear/version.h, [ac_cv_header_simgear_version_h=yes], [ac_cv_header_simgear_version_h=no])
else
AC_CHECK_HEADER(simgear/version.h)
fi
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

AC_MSG_CHECKING([for SimGear 1.99.5 or newer])
AC_TRY_RUN([
#include <stdio.h>

#include <simgear/version.h>

#define STRINGIFY(X) XSTRINGIFY(X)
#define XSTRINGIFY(X) #X

#define MIN_MAJOR 1
#define MIN_MINOR 99
#define MIN_MICRO 5

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
   AC_MSG_ERROR([Install latest SimGear first...])],
  AC_MSG_RESULT(yes)
)

LIBS="$saved_LIBS"

dnl Check if SimGear was built with Norman's JPEG factory support
if test "x$with_simgear" != "x"; then
AC_CHECK_HEADER($with_simgear/include/simgear/screen/jpgfactory.hxx,[ac_cv_header_simgear_screen_jpgfactory_hxx=yes],[ac_cv_header_simgear_screen_jpgfactory_hxx=no])
else
AC_CHECK_HEADER(simgear/screen/jpgfactory.hxx)
fi
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

AC_MSG_CHECKING([for plib 1.8.5 or newer])
AC_TRY_RUN([
#include <plib/ul.h>

#define MIN_PLIB_VERSION 185

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
   AC_MSG_ERROR([Install plib 1.8.5 or later first...])],
  AC_MSG_RESULT(yes)
)

dnl If we get here then plib is available, so force use of plib
dnl joystick lib
AC_DEFINE([ENABLE_PLIB_JOYSTICK], 1, [Define to enable plib joystick support])

AC_CHECK_HEADER(osg/Version)
if test "x$ac_cv_header_osg_Version" != "xyes"; then
    echo
    echo "You *must* have the OpenSceneGraph support library installed on your system"
    echo "to build the FGFS simulator!"
    echo
    echo "Please see README.OSG for more details."
    echo
    echo "configure aborted."
    exit
fi

AC_CHECK_HEADER(boost/foreach.hpp)
if test "x$ac_cv_header_boost_foreach_hpp" != "xyes"; then
   echo
   echo "You *must* have the Boost libraries installed."
   echo
   echo "configure aborted."
   exit
fi

# Find the OSG libraries.  Note special handling for OS X frameworks
case "${host}" in
*-apple-darwin*)

    dnl Thank you Christian Bauer from SheepSaver
    dnl AC_CHECK_FRAMEWORK($1=NAME, $2=INCLUDES)
    AC_DEFUN([AC_CHECK_FRAMEWORK], [
    AS_VAR_PUSHDEF([ac_Framework], [ac_cv_framework_$1])dnl
    AC_CACHE_CHECK([whether compiler supports framework $1],
        ac_Framework, [
        saved_LIBS="$LIBS"
        LIBS="$LIBS -framework $1"
        AC_TRY_LINK(
        [$2], [],
        [AS_VAR_SET(ac_Framework, yes)], [AS_VAR_SET(ac_Framework, no); LIBS="$saved_LIBS"]
        )
    ])
    AS_IF([test AS_VAR_GET(ac_Framework) = yes],
        [AC_DEFINE(AS_TR_CPP(HAVE_FRAMEWORK_$1), 1, [Define if framework $1 is available.])]
    )
    AS_VAR_POPDEF([ac_Framework])dnl
    ])

    if test "x$enable_osgdebug" = "xyes"; then
        AC_CHECK_LIB(osgViewerd,osgViewerGetVersion)
        AC_CHECK_LIB(osgGAd,osgGAGetVersion)
        AC_CHECK_LIB(osgTextd,osgTextGetVersion)
        LIBS="$LIBS -losgFXd"
        AC_CHECK_LIB(osgUtild,osgUtilGetVersion)
        AC_CHECK_LIB(osgDBd,osgDBGetVersion)
        AC_CHECK_LIB(osgSimd,osgSimGetVersion)
        AC_CHECK_LIB(osgParticled,osgParticleGetVersion)
        AC_CHECK_LIB(osgd,osgGetVersion)
        AC_CHECK_LIB(OpenThreadsd,OpenThreadsGetVersion)
    else
        AC_CHECK_FRAMEWORK(osgViewer, [#include <osgViewer/Version>])
        AC_CHECK_FRAMEWORK(osgGA, [#include <osgGA/Version>])
        AC_CHECK_FRAMEWORK(osgText, [#include <osgText/Version>])
        AC_CHECK_FRAMEWORK(osgFX, [#include <osgFX/AnisotropicLighting>])
        AC_CHECK_FRAMEWORK(osgUtil, [#include <osgUtil/Version>])
        AC_CHECK_FRAMEWORK(osgDB, [#include <osgDB/Version>])
        AC_CHECK_FRAMEWORK(osgSim, [#include <osgSim/Version>])
        AC_CHECK_FRAMEWORK(osgParticle, [#include <osgParticle/Version>])
        AC_CHECK_FRAMEWORK(osg, [#include <osg/Version>])
        AC_CHECK_FRAMEWORK(OpenThreads, [#include <OpenThreads/Version>])
    fi
    ;;
*)
    if test "x$enable_osgdebug" = "xyes"; then
        AC_CHECK_LIB(OpenThreadsd,OpenThreadsGetVersion)
        AC_CHECK_LIB(osgd,osgGetVersion)
        AC_CHECK_LIB(osgUtild,osgUtilGetVersion)
        AC_CHECK_LIB(osgDBd,osgDBGetVersion)
        AC_CHECK_LIB(osgTextd,osgTextGetVersion)
        AC_CHECK_LIB(osgGAd,osgGAGetVersion)
        AC_CHECK_LIB(osgViewerd,osgViewerGetVersion)
        AC_CHECK_LIB(osgSimd,osgSimGetVersion)
        AC_CHECK_LIB(osgParticled,osgParticleGetVersion)
        LIBS="$LIBS -losgFXd"
    else
        AC_CHECK_LIB(OpenThreads,OpenThreadsGetVersion)
        AC_CHECK_LIB(osg,osgGetVersion)
        AC_CHECK_LIB(osgUtil,osgUtilGetVersion)
        AC_CHECK_LIB(osgDB,osgDBGetVersion)
        AC_CHECK_LIB(osgText,osgTextGetVersion)
        AC_CHECK_LIB(osgGA,osgGAGetVersion)
        AC_CHECK_LIB(osgViewer,osgViewerGetVersion)
        AC_CHECK_LIB(osgSim,osgSimGetVersion)
        AC_CHECK_LIB(osgParticle,osgParticleGetVersion)
        LIBS="$LIBS -losgFX"
    fi
    ;;
esac

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

dnl Check for Subversion library support
save_LIBS=$LIBS
save_CPPFLAGS=$CPPFLAGS
LIBS=""
CPPFLAGS="-I/usr/include/subversion-1 -I/usr/include/apr-1.0"
AC_CHECK_LIB(svn_client-1, svn_client_checkout3)
AC_CHECK_HEADERS([svn_client.h])
if test "x$ac_cv_header_svn_client_h" != "xyes"; then
  echo "TerraSync will shell out for command line subversion"
  svn_LIBS=""
  svn_CPPFLAGS=""
else
  echo "TerraSync will use integrated subversion library"
  AC_SEARCH_LIBS(svn_client_checkout, svn_client-1)
  svn_LIBS=$LIBS
  svn_CPPFLAGS=$CPPFLAGS
  AC_SUBST(svn_LIBS)
  AC_SUBST(svn_CPPFLAGS)
fi
LIBS=$save_LIBS
CPPFLAGS=$save_CPPFLAGS

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
	src/Include/config.h-msvc71 \
	src/Include/config.h-msvc8 \
	src/Aircraft/Makefile \
	src/Airports/Makefile \
	src/ATC/Makefile \
	src/ATCDCL/Makefile \
	src/Autopilot/Makefile \
	src/Cockpit/Makefile \
	src/Cockpit/built_in/Makefile \
	src/Environment/Makefile \
	src/FDM/ExternalNet/Makefile \
	src/FDM/ExternalPipe/Makefile \
	src/FDM/JSBSim/Makefile \
	src/FDM/JSBSim/initialization/Makefile \
	src/FDM/JSBSim/input_output/Makefile \
	src/FDM/JSBSim/math/Makefile \
	src/FDM/JSBSim/models/Makefile \
	src/FDM/JSBSim/models/flight_control/Makefile \
	src/FDM/JSBSim/models/atmosphere/Makefile \
	src/FDM/JSBSim/models/propulsion/Makefile \
	src/FDM/LaRCsim/Makefile \
	src/FDM/SP/Makefile \
	src/FDM/UIUCModel/Makefile \
	src/FDM/YASim/Makefile \
	src/FDM/Makefile \
	src/GUI/Makefile \
	src/Input/Makefile \
	src/Instrumentation/Makefile \
	src/Instrumentation/KLN89/Makefile \
	src/Instrumentation/HUD/Makefile \
	src/Main/Makefile \
	src/Main/runfgfs \
	src/Main/runfgfs.bat \
	src/Model/Makefile \
	src/AIModel/Makefile \
	src/MultiPlayer/Makefile \
	src/Navaids/Makefile \
	src/Network/Makefile \
	src/Scenery/Makefile \
	src/Scripting/Makefile \
	src/Sound/Makefile \
	src/Systems/Makefile \
	src/Time/Makefile \
	src/Traffic/Makefile \
	tests/Makefile \
	utils/Makefile \
	utils/GPSsmooth/Makefile \
	utils/fgadmin/Makefile
	utils/fgadmin/src/Makefile \
	utils/js_server/Makefile \
	utils/Modeller/Makefile \
	utils/propmerge/Makefile \
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
automake --version | head -n 1

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

