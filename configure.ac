dnl Process this file with aclocal ; automake -a ; autoconf to produce a 
dnl working configure script.

#
# $Id$
#

AC_INIT
AC_CONFIG_SRCDIR([src/Aircraft/aircraft.cxx])

# Require at least automake 2.52
AC_PREREQ(2.52)

# Initialize the automake stuff
AM_INIT_AUTOMAKE(FlightGear, 0.9.1)

# Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_LN_S

# Initialize libtool
# AM_PROG_LIBTOOL

# Initialize maintainer mode
# AM_MAINTAINER_MODE

# This is needed for AC_TRY_COMPILE later
# AC_ISC_POSIX

# Check to see if this `configure' is being run in the `Cygwin32' environment
AC_EGREP_CPP(yes,
[#ifdef __MINGW32__
 yes
 #endif
],is_mingw=yes, is_mingw=no)

echo "IS_MINGW = $is_mingw"
AM_CONDITIONAL(IS_MINGW, test "x$is_mingw" = "xyes")

AC_EGREP_CPP(yes,
[#ifdef __CYGWIN__
 yes
 #endif
],is_cygwin=yes, is_cygwin=no)

echo "IS_CYGWIN = $is_cygwin"
AM_CONDITIONAL(IS_CYGWIN, test "x$is_cygwin" = "xyes")

# Used by JSBSim
AC_DEFINE([FGFS], 1, [Define so that JSBSim compiles in 'library' mode])


# Used on the Irix platform
AR="ar"
ARFLAGS="cru"
OS=`uname -s`
if test "$OS" = "IRIX" -o "$OS" = "IRIX64"; then
    if test "$CXX" = "CC"; then
        AR="CC -ar"
        ARFLAGS="-o"
    fi
fi
AC_SUBST(AR)
AC_SUBST(ARFLAGS)


# Specify if we want logging (testing build) or not (release build)
# set logging default value
# with_logging=yes
AC_ARG_WITH(logging, [  --with-logging          Include logging output (default)])
if test "x$with_logging" = "xno" ; then
    AC_DEFINE([FG_NDEBUG], 1, [Define for no logging output])
fi

# Specify if we want to build with Oliver's networking support
# default to with_network=yes
AC_ARG_WITH(network_olk, [  --with-network-olk      Include Oliver's multi-pilot networking support])
if test "x$with_network_olk" = "xno" ; then
    echo "Building without Oliver's multi-pilot network support"
else
    echo "Building with Oliver's multi-pilot network support"
    AC_DEFINE([FG_NETWORK_OLK], 1, [Define to build with Oliver's networking])
fi
AM_CONDITIONAL(ENABLE_NETWORK_OLK, test "x$with_network_olk" != "xno")


# Specify if we want to use WeatherCM instead of FGEnvironment.
# default to with_weathercm=no
AC_ARG_WITH(new-environment, [  --with-weathercm        Use WeatherCM instead of FGEnvironment])
if test "x$with_weathercm" = "xyes" ; then
    echo "Building with WeatherCM"
    AC_DEFINE([FG_WEATHERCM], 1,
              [Define to build with Christian Mayer's weather code])
else
    echo "Building with FGEnvironment"
fi
AM_CONDITIONAL(ENABLE_WEATHERCM, test "x$with_weathercm" = "xyes")

# Let the Win32 user specify if they want to build with the SGI
# opengl.dll as opposed to the more standard openg32.dll
AC_ARG_WITH(sgi-opengl, [  --with-sgi-opengl       Build against SGI's opengl.dll glu.dll and glut.dll])

dnl Thread related checks
AC_ARG_WITH(threads, [  --with-threads          Include tile loading threads [default=no]])
if test "x$with_threads" = "xyes"; then
    AC_DEFINE([ENABLE_THREADS], 1, [Define to enable threaded tile paging])
    CXXFLAGS="$CXXFLAGS -D_REENTRANT"
    CFLAGS="$CFLAGS -D_REENTRANT"
    AC_CHECK_HEADER(pthread.h)
fi
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

# specify the simgear location
AC_ARG_WITH(simgear, [  --with-simgear=PREFIX   Specify the prefix path to simgear])

if test "x$with_simgear" != "x" ; then
    echo "SimGear prefix path is $with_simgear"
    EXTRA_DIRS="${EXTRA_DIRS} $with_simgear"
fi

# Check for MS Windows environment
AC_CHECK_HEADER(windows.h)

# extra library and include directories
if test "x$ac_cv_header_windows_h" != "xyes" ; then
    EXTRA_DIRS="${EXTRA_DIRS} /usr/X11R6"

    if test -d /opt/X11R6 ; then
        EXTRA_DIRS="${EXTRA_DIRS} /opt/X11R6"
    fi
else
    if test  "x$is_cygwin" = "xyes" ; then
        EXTRA_DIRS="${EXTRA_DIRS} /usr/local"
    fi
fi

wi_EXTRA_DIRS(no, ${EXTRA_DIRS})

# Using AM_CONDITIONAL is a step out of the protected little 
# automake fold so it is potentially dangerous.  But, we are
# beginning to run into cases where the standard checks are not
# enough.  AM_CONDITIONALS are then referenced to conditionally
# build a Makefile.in from a Makefile.am which lets us define custom
# includes, compile alternative source files, etc.

# Check for external variables daylight and timezone.
AC_EXT_DAYLIGHT
if test "$have_daylight" = yes; then
    AC_DEFINE([HAVE_DAYLIGHT], 1, [Define if system has daylight variable])
fi
# AM_CONDITIONAL(HAVE_DAYLIGHT, test "$have_daylight" = yes )

AC_EXT_TIMEZONE
if test "$have_timezone" = yes; then
    AC_DEFINE([HAVE_TIMEZONE], 1, [Define if system has timezone variable])
fi
# AM_CONDITIONAL(HAVE_TIMEZONE, test "$have_timezone" = yes )

# Check for audio support
AC_MSG_CHECKING(for audio support)
audio_LIBS=""
if test -r /usr/include/soundcard.h \
	-o -r /usr/include/linux/soundcard.h \
	-o -r /usr/include/machine/soundcard.h \
	-o -r /usr/include/audio.h \
	-o "x$ac_cv_header_windows_h" = "xyes" \
        -o "$HOSTTYPE" = "macintosh"; then

    AC_DEFINE([ENABLE_AUDIO_SUPPORT], 1, [Define for audio support])
    audio_LIBS="-lplibsl -lplibsm"
    AC_MSG_RESULT(yes)
    if test -r /usr/include/audio.h; then
        audio_LIBS="$audio_LIBS -laudio"
    fi

    if test "x$ac_cv_header_windows_h" = "xyes"; then
        audio_LIBS="$audio_LIBS -lwinmm"
    fi

    if test "$HOSTTYPE" = "macintosh"; then
	audio_LIBS="$audio_LIBS -framework Carbon"
    fi
else
    AC_MSG_RESULT(no)
fi
AC_SUBST(audio_LIBS)

# Check for X11 (fancy)
if test "x$no_x" != "xyes"; then
    AC_PATH_XTRA
fi

# Checks for libraries.

null_LIBS="$LIBS"

AC_CHECK_LIB(m, cos)

dnl MINGW requires libwsock32
if test  "x$is_mingw" = "xyes" ; then
    LIBS="$LIBS -lws2_32"
fi

base_LIBS="$LIBS"

dnl Mesa >= 3.2 requires pthreads
AC_CHECK_LIB(pthread, pthread_exit)
AC_CHECK_LIB(socket, socket)

dnl Skip X library tests.
if test "x$no_x" != "xyes"; then
    AC_CHECK_LIB(X11, XCreateWindow)
    AC_CHECK_LIB(Xext, XShmCreateImage)
    AC_CHECK_LIB(Xi, XGetExtensionVersion)
    AC_CHECK_LIB(ICE, IceOpenConnection)
    AC_CHECK_LIB(SM, SmcOpenConnection)
    AC_CHECK_LIB(Xt, XtMalloc)
    AC_CHECK_LIB(Xmu, XmuLookupStandardColormap)
fi

# check for OpenGL related libraries

if test "$HOSTTYPE" = "macintosh"; then
    LIBS="$LIBS -framework OpenGL -framework GLUT -lobjc"
elif test "x$ac_cv_header_windows_h" != "xyes" ; then
    # Reasonable stuff for non-windoze variants ... :-)

    AC_CHECK_LIB(GLcore, glNewList)
    if test "x$ac_cv_lib_GLcore_glNewList" = "xno" ; then
	# if no GLcore, check for GL
	AC_CHECK_LIB(GL, glNewList)
	if test "x$ac_cv_lib_GL_glNewList" = "xno" ; then
	    # if no GL, check for MesaGL
	    AC_CHECK_LIB(MesaGL, glNewList)
	fi
    else
	# if GLcore found, then also check for GL
	AC_CHECK_LIB(GL, glXCreateContext)
    fi

    # check for xmesa.h and if found enable XMESA / FX
    AC_CHECK_HEADER(GL/fxmesa.h)
    if test "x$ac_cv_header_GL_fxmesa_h" = "xyes"; then
	save_LIBS="$LIBS"
	AC_CHECK_LIB(GL, XMesaSetFXmode)
	if test "x$ac_cv_lib_GL_XMesaSetFXmode" = "xyes" ; then
	    AC_DEFINE([XMESA], 1, [Define for Mesa FX mode])
            AC_DEFINE([FX], 1, [Define for Mesa FX mode])
	else
	    AC_CHECK_LIB(MesaGL, XMesaSetFXmode)
	    if test "x$ac_cv_lib_MesaGL_XMesaSetFXmode" = "xyes" ; then
                AC_DEFINE([XMESA], 1, [Define for Mesa FX mode])
                AC_DEFINE([FX], 1, [Define for Mesa FX mode])
	    fi
	fi
	LIBS="$save_LIBS"
    fi

    AC_CHECK_LIB(GLU, gluLookAt)
    if test "x$ac_cv_lib_GLU_gluLookAt" = "xno" ; then
	# if no GLU, check for MesaGLU
	AC_CHECK_LIB(MesaGLU, gluLookAt)
    fi

    # check for glut
    AC_CHECK_LIB(glut, glutGetModifiers)

    # test for glutGameModeString, but avoid adding glut a second time into
    # the list of libraries
    save_LIBS="$LIBS"
    AC_CHECK_LIB(glut, glutGameModeString)
    LIBS="$save_LIBS"
else
    # Win32 is a little wierd because it has to try to handle the various
    # winbloze-isms.  We'll just do this manually for now.

    echo Win32 specific hacks...
    AC_DEFINE([WIN32], 1, [Define if building on a Win32 platform])
    AC_DEFINE([NOMINMAX], 1, [Define for Win32 platforms])

    # just define these to true and hope for the best
    ac_cv_lib_glut_glutGetModifiers="yes"
    ac_cv_lib_glut_glutGameModeString="yes"

    if test "x$with_sgi_opengl" = "xyes" ; then
        echo "Building with glut.dll, glu.dll, and opengl.dll"
	WIN32_GLUT=glut
	WIN32_GLU=glu
	WIN32_OPENGL=opengl
    else 
	echo "Building with glut32.dll, glu32.dll, and opengl32.dll"
	WIN32_GLUT=glut32
	WIN32_GLU=glu32
	WIN32_OPENGL=opengl32
    fi

    LIBS="$LIBS -l${WIN32_GLUT} -l${WIN32_GLU} -l${WIN32_OPENGL}"
    LIBS="$LIBS -luser32 -lgdi32"
    echo "Will link apps with $LIBS"
fi

if test "x$ac_cv_lib_glut_glutGetModifiers" = "xno"; then
    echo 
    echo "Unable to find the necessary OpenGL or GLUT libraries."
    echo "See config.log for automated test details and results ..."
    exit 1
fi

if test "x$ac_cv_lib_glut_glutGameModeString" = "xno"; then
    echo
    echo "Your version of glut doesn't support game mode."
    echo "You need to install the latest version.  If your distribution doesn't"
    echo "provide a newer version, you can get the latest source code from:"
    echo
    echo "    http://reality.sgi.com/opengl/glut3/glut3.html"
    exit 1
fi

opengl_LIBS="$LIBS"
LIBS="$base_LIBS"

AC_SUBST(base_LIBS)
AC_SUBST(opengl_LIBS)

AM_CONDITIONAL(HAVE_XWINDOWS, test "x$ac_cv_lib_X11_XCreateWindow" = "xyes" )

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

# If we get here then plib is available, so force use of plib joystick lib
AC_DEFINE([ENABLE_PLIB_JOYSTICK], 1, [Define to enable plib joystick support])

# Do we have a new CVS plib version with jsJoystick::getName?
echo -n "Testing for Plib joystick getName support: "
AC_EGREP_HEADER( getName, plib/js.h,
                 plib_js_has_getname=yes, plib_js_has_getname=no )
echo $plib_js_has_getname
if test $plib_js_has_getname = 'yes'; then
    AC_DEFINE([FG_PLIB_JOYSTICK_GETNAME], 1,
              [Define to enable plib joystick name support])
fi

# Check for the presence of SimGear
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

AC_MSG_CHECKING([for simgear 0.3.0 or newer])
AC_TRY_RUN([
#include <stdio.h>

#include <simgear/version.h>

#define STRINGIFY(X) XSTRINGIFY(X)
#define XSTRINGIFY(X) #X

#define MIN_MAJOR 0
#define MIN_MINOR 3
#define MIN_MICRO 0

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

# Check for MetaKit
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
LIBS="$saved_LIBS"

# Check if SimGear was built with Norman JPEG factory support
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

# Check for system installed zlib
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

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS( \
	fcntl.h getopt.h malloc.h memory.h stdlib.h sys/param.h sys/stat.h \
        sys/time.h sys/timeb.h unistd.h windows.h winbase.h values.h )

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS( ftime gettimeofday timegm memcpy bcopy mktime strstr rand \
	random setitimer getitimer signal GetLocalTime rint getrusage )

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
	src/Navaids/Makefile \
	src/Network/Makefile \
	src/NetworkOLK/Makefile \
	src/Objects/Makefile \
	src/Scenery/Makefile \
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

if test "x$with_logging" != "x"; then
    echo "Debug messages: $with_logging"
else
    echo "Debug messages: yes"
fi

echo -n "Automake version: ($AUTO_MAKE_VERSION) "
automake --version | head -1

echo "New plib joystick name support: $plib_js_has_getname"

if test "x$with_weathercm" != "x"; then
    echo "WeatherCM instead of FGEnvironment: $with_weathercm"
else
    echo "Using FGEnvironment"
fi

if test "x$with_threads" = "xyes"; then
    echo "threads: yes"
else
    echo "threads: no"
fi
