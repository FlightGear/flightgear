dnl Process this file with autogen.sh to produce a working configure
dnl script.

AC_INIT(FlightGear, m4_esyscmd([cat version]), [http://www.flightgear.org])

dnl Ensure touching the version causes autoconf to re-run
AC_SUBST([CONFIGURE_DEPENDENCIES], ['$(top_srcdir)/version'])

AC_CONFIG_SRCDIR([src/Airports/simple.cxx])

dnl Require at least automake 2.52
AC_PREREQ(2.52)

dnl Initialize the automake stuff
dnl set the $host variable based on local machine/os
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([dist-bzip2])

# variables for version.h
AC_DEFINE([HAVE_VERSION_H], 1, [Set if version.h is generated])

# are we running under Hudson? (defines BUILD_ID, BUILD_NUMBER in the environment)
if test "x$BUILD_ID" != "x" ; then
    AC_SUBST([HUDSON_BUILD_ID], $BUILD_ID)
    AC_SUBST([HUDSON_BUILD_NUMBER], $BUILD_NUMBER)
else
    AC_SUBST([HUDSON_BUILD_ID], [none])
    AC_SUBST([HUDSON_BUILD_NUMBER], [none])
fi

AC_CHECK_PROG([HAVE_GIT], git, 1)
if test "x$HAVE_GIT" != "x" ; then 
    # git might be installed, but we might be building from a tarball
    if git rev-parse ; then
	AC_SUBST([REVISION], `git rev-parse HEAD`)
    else
	AC_SUBST([REVISION], [none])	
    fi
else
    AC_SUBST([REVISION], [none])	
fi

dnl Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_LN_S
AX_BOOST_BASE([1.37.0])

# Show all compiler warnings by default
CXXFLAGS="$CXXFLAGS -Wall"
CFLAGS="$CFLAGS -Wall"

if test "x$BOOST_CPPFLAGS" != "x-I/usr/include" ; then
   CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
fi

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

# specify framework related locations for Mac OS X
case "${host}" in
*-apple-darwin*)

    dnl Thank you Christian Bauer from SheepSaver
    dnl Modified by Tatsuhiro Nishioka for accepting a given framework path
    dnl AC_CHECK_FRAMEWORK($1=NAME, $2=INCLUDES, $3=FRAMEWORK_PATH, $4=ACTION_IF_FOUND) ; 
    AC_DEFUN([AC_CHECK_FRAMEWORK], [
    AS_VAR_PUSHDEF([ac_Framework], [ac_cv_framework_$1])dnl
    AC_CACHE_CHECK([whether compiler supports framework $1],
        ac_Framework, [
        saved_LIBS="$LIBS"
        FRAMEWORKS="$FRAMEWORKS -framework $1"
        if test "$3" = ""; then
            FRAMEWORKS="$FRAMEWORKS $ADD2LD"
        elif test "`echo $FRAMEWORKS | grep -- -F$3`" = ""; then
            FRAMEWORKS="$FRAMEWORKS -F$3"
            CXXFLAGS="$CXXFLAGS -F$3"
            CCFLAGS="$CCFLAGS -F$3"
            CPPFLAGS="$CPPFLAGS -F$3"
            dnl This is needed for AC_TRY_LINK when a framework path is specified
            export DYLD_FRAMEWORK_PATH="${DYLD_FRAMEWORK_PATH}:$3"
        fi
        AC_TRY_LINK(
        [$2], [],
        [AS_VAR_SET(ac_Framework, yes)], [AS_VAR_SET(ac_Framework, no); LIBS="$saved_LIBS"]
        )
    ])
    AS_IF([test AS_VAR_GET(ac_Framework) = yes],
        [AC_DEFINE(AS_TR_CPP(HAVE_FRAMEWORK_$1), 1, [Define if framework $1 is available.])]
    )
    AS_IF([test AS_VAR_GET(ac_Framework) = yes], $4)
    AS_VAR_POPDEF([ac_Framework])dnl
    ])

    # Mac OS X specific configure options
    AC_ARG_WITH(cocoa_framework, [  --with-cocoa-framework         Use the Cocoa rather than Carbon])
    if test "x$with_cocoa_framework" != "x" ; then
        macAPI=Cocoa
        AC_MSG_NOTICE([Using Cocoa framework])
    else
        macAPI=Carbon
        AC_MSG_NOTICE([Using Carbon framework])
    fi

    AC_ARG_WITH(osg_framework, [  --with-osg-framework=PREFIX    Specify the prefix path to osg frameworks [default=standard framework paths]])
    if test "x$with_osg_framework" != "x" ; then
        echo "osg prefix is $with_osg_framework"
	export DYLD_FRAMEWORK_PATH="$DYLD_FRAMEWORK_PATH:$with_osg_framework"
    fi

    AC_ARG_WITH(plib_framework, [  --with-plib-framework=PREFIX   Specify the prefix path to PLIB framework [default=standard framework paths]])
    if test "x$with_plib_framework" != "x" ; then
        echo "plib prefix is $with_plib_framework"
	export DYLD_FRAMEWORK_PATH="$DYLD_FRAMEWORK_PATH:$with_plib_framework"
    fi

    AC_ARG_WITH(alut_framework, [  --with-alut-framework=PREFIX   Specify the prefix path to ALUT.framework ])
    if test "x$with_alut_framework" != "x"; then
        echo "ALUT framework prefix is $with_alut_framework"
    fi

    ;;
esac

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

# Specify whether we want to compile ATCDCL.
# default to with_atcdcl=yes
AC_ARG_ENABLE(atcdcl, [  --enable-atcdcl                Compile and link the deprecated atc/ai module], [], [enable_atcdcl="$enableval"] )
if test "x$enable_atcdcl" = "xyes"; then
    AC_DEFINE([ENABLE_ATCDCL], 1, [Define to include old ATC/AI module])
else
    AC_DEFINE([ENABLE_ATCDCL], 0, [Define to include old ATC/AI module])
fi
AM_CONDITIONAL(ENABLE_ATCDCL, test "x$enable_atcdcl" = "xyes")

dnl EXPERIMENTAL generic event driven input device
# defaults to no
AC_ARG_WITH(eventinput, [  --with-eventinput       Include event driven input (EXPERIMENTAL) [default=no]], [], [with_eventinput=no])
if test "x$with_eventinput" = "xyes"; then
    AC_DEFINE([WITH_EVENTINPUT], 1, [Define to enable generic event driven input device])
    case "${host}" in
    dnl OS specific sources for event driven input
    dnl Linux and Mac OS X are supported at this moment
    *-apple-darwin*)
        eventinput_EXTRA_OBJS="FGMacOSXEventInput.o"
        eventinput_INCUDES=""
        eventinput_LIBS=""
        ;;
    *linux*)
        dnl
        dnl FIXME: include paths for dbus are hard-coded at this moment.
        dnl FIXME: these must be found in configure
        dnl
        eventinput_EXTRA_OBJS="FGLinuxEventInput.o"
        eventinput_INCLUDES="-I/usr/include/dbus-1.0 -I/usr/lib64/dbus-1.0/include -I/usr/lib/dbus-1.0/include"
        eventinput_LIBS="-ldbus-1 -lhal"
        ;;
    *)
        echo "Event driven input is not supported on ${host}."
        ;;
    esac
    AC_SUBST(eventinput_EXTRA_OBJS)
    AC_SUBST(eventinput_INCLUDES)
    AC_SUBST(eventinput_LIBS)
fi
AM_CONDITIONAL(WITH_EVENTINPUT, test "x$with_eventinput" = "xyes")

dnl Thread related checks
# defaults to yes
AC_ARG_WITH(threads, [  --with-threads          Include tile loading threads [default=yes]], [], [with_threads=yes])
if test "x$with_threads" = "xyes"; then
    AC_DEFINE([ENABLE_THREADS], 1, [Define to enable threaded tile paging])
    CXXFLAGS="$CXXFLAGS -D_REENTRANT"
    CFLAGS="$CFLAGS -D_REENTRANT"
fi
AC_CHECK_HEADER(pthread.h)

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
    LIBS="-lsgthreads $LIBS"
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
AC_SEARCH_LIBS(clock_gettime, rt)

base_LIBS="$LIBS"

dnl Check for SDL or glut if enabled.
AC_ARG_ENABLE(osgviewer, [  --enable-osgviewer             Configure to use osgViewer (default)], [enable_osgviewer="$enableval"])
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

    LIBS="$LIBS -framework GLUT -framework OpenGL -framework AGL -framework $macAPI -lobjc"
    LDFLAGS="$LDFLAGS -headerpad_max_install_names"
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
    AC_SEARCH_LIBS(alGenBuffers, [ openal32 openal 'openal -ldsound -lwinmm' ] )
    AC_SEARCH_LIBS(alutInit, [ openal32 ALut alut ] )
    LIBS="$LIBS -lwinmm -ldsound -ldxguid -lole32"
    openal_LIBS="$LIBS"
    OPENAL_OK="$ac_cv_search_alGenBuffers"
    ALUT_OK="$ac_cv_search_alutInit"
    ;;

*-apple-darwin*)
    dnl Mac OS X

    # Mac OS X has OpenAL.framework, but no ALUT, by default, so we
    # require use of a non-Apple ALUT.framework which we provide
    openal_LIBS="-framework IOKit -framework OpenAL"
    
    AC_CHECK_FRAMEWORK(OpenAL, [#include <OpenAL/al.h>], "", [OPENAL_OK="yes"])
    AC_CHECK_FRAMEWORK(ALUT, [#include <ALUT/alut.h>], $with_alut_framework, [
        ALUT_OK="yes"
        openal_LIBS="$openal_LIBS -framework ALUT"
    ])
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

AC_MSG_CHECKING([for SimGear 2.0.0 or newer])
AC_TRY_RUN([
#include <stdio.h>

#include <simgear/version.h>

#define STRINGIFY(X) XSTRINGIFY(X)
#define XSTRINGIFY(X) #X

#define MIN_MAJOR 2
#define MIN_MINOR 0
#define MIN_MICRO 0

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
case "${host}" in
*-apple-darwin*)
    if test "x$with_plib_framework" != "x"; then
        AC_CHECK_FRAMEWORK(PLIB, [#include <plib/ul.h>], $with_plib_framework, no)
        plib_FRAMEWORK="$FRAMEWORKS"
        AC_SUBST(plib_FRAMEWORK)
        FRAMEWORKS=""
    fi
    ;;
esac
AM_CONDITIONAL(HAVE_FRAMEWORK_PLIB, test "x$ac_cv_framework_PLIB" != "x")

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

echo "$DYLD_FRAMEWORK_PATH"
AC_MSG_CHECKING([for plib 1.8.5 or newer])
AC_TRY_RUN([
#include <plib/ul.h>

#define MIN_PLIB_VERSION 185

int main() {
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

# Find the OSG libraries.  Note special handling for OS X frameworks
if test "x$with_osg_framework" = "x"; then
    if test "x$enable_osgdebug" = "xyes"; then
        AC_CHECK_LIB(OpenThreadsd,OpenThreadsGetVersion, , [AC_MSG_ERROR(OpenThreads library not found.)],)
        AC_CHECK_LIB(osgd,osgGetVersion, , [AC_MSG_ERROR(OpenSceneGraph library not found.)],)
        AC_CHECK_LIB(osgUtild,osgUtilGetVersion, , [AC_MSG_ERROR(OpenSceneGraph utility library not found.)],)
        AC_CHECK_LIB(osgDBd,osgDBGetVersion, , [AC_MSG_ERROR(OpenSceneGraph database library not found.)],)
        AC_CHECK_LIB(osgTextd,osgTextGetVersion, , [AC_MSG_ERROR(OpenSceneGraph Text library not found.)],)
        AC_CHECK_LIB(osgGAd,osgGAGetVersion, , [AC_MSG_ERROR(OpenSceneGraph GUI Abstraction library not found.)],)
        AC_CHECK_LIB(osgViewerd,osgViewerGetVersion, , [AC_MSG_ERROR(OpenSceneGraph Viewer library not found.)],)
        AC_CHECK_LIB(osgSimd,osgSimGetVersion, , [AC_MSG_ERROR(OpenSceneGraph simulation library not found.)],)
        AC_CHECK_LIB(osgParticled,osgParticleGetVersion, , [AC_MSG_ERROR(OpenSceneGraph Particle library not found.)],)
        AC_CHECK_LIB(osgFXd, osgFXGetVersion, , [AC_MSG_ERROR(OpenSceneGraph FX library not found.)],)
    else
        AC_CHECK_LIB(OpenThreads,OpenThreadsGetVersion, , [AC_MSG_ERROR(OpenThreads library not found.)],)
        AC_CHECK_LIB(osg,osgGetVersion, , [AC_MSG_ERROR(OpenSceneGraph library not found.)],)
        AC_CHECK_LIB(osgUtil,osgUtilGetVersion, , [AC_MSG_ERROR(OpenSceneGraph utility library not found.)],)
        AC_CHECK_LIB(osgDB,osgDBGetVersion, , [AC_MSG_ERROR(OpenSceneGraph database library not found.)],)
        AC_CHECK_LIB(osgText,osgTextGetVersion, , [AC_MSG_ERROR(OpenSceneGraph Text library not found.)],)
        AC_CHECK_LIB(osgGA,osgGAGetVersion, , [AC_MSG_ERROR(OpenSceneGraph GUI Abstraction library not found.)],)
        AC_CHECK_LIB(osgViewer,osgViewerGetVersion, , [AC_MSG_ERROR(OpenSceneGraph Viewer library not found.)],)
        AC_CHECK_LIB(osgSim,osgSimGetVersion, , [AC_MSG_ERROR(OpenSceneGraph simulation library not found.)],)
        AC_CHECK_LIB(osgParticle,osgParticleGetVersion, , [AC_MSG_ERROR(OpenSceneGraph Particle library not found.)],)
        AC_CHECK_LIB(osgFX, osgFXGetVersion, , [AC_MSG_ERROR(OpenSceneGraph FX library not found.)],)
    fi
else
    # Checking osg frameworks.
    AC_CHECK_FRAMEWORK(osgViewer, [#include <osgViewer/Version>], $with_osg_framework)
    AC_CHECK_FRAMEWORK(osgGA, [#include <osgGA/Version>], $with_osg_framework)
    AC_CHECK_FRAMEWORK(osgText, [#include <osgText/Version>], $with_osg_framework)
    AC_CHECK_FRAMEWORK(osgFX, [#include <osgFX/AnisotropicLighting>], $with_osg_framework)
    AC_CHECK_FRAMEWORK(osgUtil, [#include <osgUtil/Version>], $with_osg_framework)
    AC_CHECK_FRAMEWORK(osgDB, [#include <osgDB/Version>], $with_osg_framework)
    AC_CHECK_FRAMEWORK(osgSim, [#include <osgSim/Version>], $with_osg_framework)
    AC_CHECK_FRAMEWORK(osgParticle, [#include <osgParticle/Version>], $with_osg_framework)
    AC_CHECK_FRAMEWORK(osg, [#include <osg/Version>], $with_osg_framework)
    osg_FRAMEWORKS="$FRAMEWORKS"
    FRAMEWORKS=""
    AC_CHECK_FRAMEWORK(OpenThreads, [#include <OpenThreads/Version>], $with_osg_framework)
    openthreads_FRAMEWORK="$FRAMEWORKS"
    AC_SUBST(osg_FRAMEWORKS)
    AC_SUBST(openthreads_FRAMEWORK)
fi
AM_CONDITIONAL(HAVE_FRAMEWORK_OSG, test "x$ac_cv_framework_osg" != "x")

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


AC_MSG_CHECKING([checking for osg::CullSettings::CLEAR_MASK])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include <osg/CullSettings>]],
    [[osg::CullSettings::VariablesMask mask = osg::CullSettings::CLEAR_MASK;]])],
  [AC_DEFINE([HAVE_CULLSETTINGS_CLEAR_MASK],1,[define if OSG has CullSettings::CLEAR_MASK])
   AC_MSG_RESULT([yes])],
  [AC_MSG_RESULT([no])])

# Special handling for static version of OSG
if test -n "`echo "$CPPFLAGS" "$CXXFLAGS" "$CFLAGS" | grep "\-DOSG_LIBRARY_STATIC" 2>/dev/null`" ; then
    saved_LIBS="$LIBS"
    LIBS=""
    #***** Check image support (needed for static build) *****
    AC_SEARCH_LIBS([jpeg_read_header], [jpeg], [CPPFLAGS="$CPPFLAGS -DOSG_JPEG_ENABLED"])
    AC_SEARCH_LIBS([png_read_image], [png 'png -lz'], [CPPFLAGS="$CPPFLAGS -DOSG_PNG_ENABLED"])
    AC_SEARCH_LIBS([TIFFOpen], [tiff], [CPPFLAGS="$CPPFLAGS -DOSG_TIFF_ENABLED"])
    img_LIBS="$LIBS"
    LIBS="$saved_LIBS"

    if test "x$with_osg" != "x" ; then
        osg_prefix="$with_osg"
    else
        osg_prefix="/usr"
    fi
    PKG_CHECK_EXISTS([openscenegraph], [osg_version="`$PKG_CONFIG --modversion openscenegraph 2>/dev/null`"])
    LIBS="-L$osg_prefix/lib/osgPlugins-$osg_version -losgdb_bmp -losgdb_dds -losgdb_hdr -losgdb_pic -losgdb_pnm -losgdb_rgb -losgdb_tga \
                                                          -losgdb_3ds -losgdb_ac -losgdb_ive -losgdb_osg -losgdb_txf \
                                                          -losgVolume -losgTerrain $LIBS -losgUtil"

    if test -n "`echo "$CPPFLAGS" "$CXXFLAGS" "$CFLAGS" | grep "\-DOSG_JPEG_ENABLED" 2>/dev/null`" ; then
      LIBS="$LIBS -losgdb_jpeg"
    fi
    if test -n "`echo "$CPPFLAGS" "$CXXFLAGS" "$CFLAGS" | grep "\-DOSG_PNG_ENABLED" 2>/dev/null`" ; then
      LIBS="$LIBS -losgdb_png"
    fi
    if test -n "`echo "$CPPFLAGS" "$CXXFLAGS" "$CFLAGS" | grep "\-DOSG_TIFF_ENABLED" 2>/dev/null`" ; then
      LIBS="$LIBS -losgdb_tiff"
    fi

    LIBS="$LIBS $img_LIBS"
fi

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
LIBS="`apr-1-config --link-ld`"
CPPFLAGS="-I/usr/include/subversion-1 `apr-1-config --includes`"
AC_CHECK_HEADERS([svn_client.h])
if test "x$ac_cv_header_svn_client_h" != "xyes"; then
  echo "TerraSync will shell out for command line subversion"
  svn_LIBS=""
  svn_CPPFLAGS=""
else
  echo "TerraSync will use integrated subversion library"
  AC_SEARCH_LIBS(svn_client_checkout, svn_client-1)
  AC_SEARCH_LIBS(svn_cmdline_init, svn_subr-1)
  svn_LIBS=$LIBS
  svn_CPPFLAGS=$CPPFLAGS
  AC_SUBST(svn_LIBS)
  AC_SUBST(svn_CPPFLAGS)
fi
LIBS=$save_LIBS
CPPFLAGS=$save_CPPFLAGS

AC_MSG_CHECKING([for feenableexcept])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#define _GNU_SOURCE
#include <fenv.h>]], [[feenableexcept(FE_DIVBYZERO)]])],
[AC_DEFINE([HAVE_FEENABLEEXCEPT], 1, [define if system has fenableexcept])],
AC_MSG_RESULT([yes]),
AC_MSG_RESULT([no]))

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
	utils/fgviewer/Makefile
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

if test "x$with_eventinput" = "xyes"; then
    echo "event input: yes"
else
    echo "event input: no"
fi

if test "x$enable_sp_fdms" != "xno"; then
    echo "Include special purpose flight models: yes"
else
    echo "Include special purpose flight models: no"
fi

if test "x$enable_atcdcl" = "xyes"; then
    echo "Build depricated ATC/AI module: yes"
else
    echo "Build depricated ATC/AI module: no"
fi

