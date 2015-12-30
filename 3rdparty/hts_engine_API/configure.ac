#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(hts_engine_API, 1.10, hts-engine-users@lists.sourceforge.net, hts_engine_API)
AC_CONFIG_AUX_DIR([config])
AC_COPYRIGHT(Copyright 2001-2015 Nagoya Institute of Technology)
AC_COPYRIGHT(Copyright 2001-2008 Tokyo Institute of Technology)
AM_INIT_AUTOMAKE

# Checks for C compiler
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_RANLIB
AN_MAKEVAR([AR], [AC_PROG_AR])
AN_PROGRAM([ar], [AC_PROG_AR])
AC_DEFUN([AC_PROG_AR], [AC_CHECK_TOOL(AR, ar, :)])
AC_PROG_AR


# Checks for libraries.
AC_CHECK_LIB([m], [log])


# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h])


# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([sqrt strchr strrchr strstr])


# Checks for embedded device compile
AC_ARG_ENABLE(embedded, [  --enable-embedded   turn on compiling for embedded devices (default=no)],,enable_embedded=no)
AC_MSG_CHECKING(whether to enable compiling for embedded devices)
if test x$enable_embedded = xyes; then
	AC_MSG_RESULT(yes)
	AC_DEFINE(HTS_EMBEDDED)
else
	AC_MSG_RESULT(no)
fi


# Checks for using festival
AC_ARG_ENABLE(festival, [  --enable-festival   use memory allocation/free functions of speech tools (default=no)],,enable_festival=no)
AC_MSG_CHECKING(whether to use memory allocation/free functions of speech tools)
if test x$enable_festival = xyes; then
	AC_MSG_RESULT(yes)
	AC_DEFINE(FESTIVAL)
else
	AC_MSG_RESULT(no)
fi


AC_CANONICAL_HOST
AC_C_BIGENDIAN


# Checks library for windows audio devices
case "$host_os" in
 *win32* | *wince* | *cygwin* | *mingw* )
	AC_HAVE_LIBRARY([winmm],,AC_MSG_ERROR(No winmm))
	;;
 *)
	;;
esac


AC_CONFIG_FILES([Makefile bin/Makefile lib/Makefile])

AC_OUTPUT

