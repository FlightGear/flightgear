dnl
dnl originally from ncftp 2.3.0
dnl added wi_EXTRA_PDIR and wi_ANSI_C
dnl $Id$
dnl
AC_DEFUN(wi_EXTRA_IDIR, [
incdir="$1"
if test -r $incdir ; then
	case "$CPPFLAGS" in
		*-I${incdir}*)
			# echo "   + already had $incdir" 1>&6
			;;
		*)
			if test "$CPPFLAGS" = "" ; then
				CPPFLAGS="-I$incdir"
			else
				CPPFLAGS="$CPPFLAGS -I$incdir"
			fi
			echo "   + found $incdir" 1>&6
			;;
	esac
fi
])
dnl
dnl
dnl
dnl
AC_DEFUN(wi_EXTRA_LDIR, [
mylibdir="$1"
if test -r $mylibdir ; then
	case "$LDFLAGS" in
		*-L${mylibdir}*)
			# echo "   + already had $mylibdir" 1>&6
			;;
		*)
			if test "$LDFLAGS" = "" ; then
				LDFLAGS="-L$mylibdir"
			else
				LDFLAGS="$LDFLAGS -L$mylibdir"
			fi
			echo "   + found $mylibdir" 1>&6
			;;
	esac
fi
])
dnl
dnl __FP__
dnl
dnl
AC_DEFUN(wi_EXTRA_PDIR, [
progdir="$1"
if test -r $progdir ; then
	case "$PATH" in
		*:${progdir}*)
			# echo "   + already had $progdir" 1>&6
			;;
		*${progdir}:*)
			# echo "   + already had $progdir" 1>&6
			;;
		*)
			if test "$PATH" = "" ; then
				PATH="$progdir"
			else
				PATH="$PATH:$progdir"
			fi
			echo "   + found $progdir" 1>&6
			;;
	esac
fi
])
dnl
dnl
dnl If you want to also look for include and lib subdirectories in the
dnl $HOME tree, you supply "yes" as the first argument to this macro.
dnl
dnl If you want to look for subdirectories in include/lib directories,
dnl you pass the names in argument 3, otherwise pass a dash.
dnl
AC_DEFUN(wi_EXTRA_DIRS, [echo "checking for extra include and lib directories..." 1>&6
ifelse([$1], yes, [dnl
b1=`cd .. ; pwd`
b2=`cd ../.. ; pwd`
exdirs="$HOME $j $b1 $b2 $prefix $2"
],[dnl
exdirs="$prefix $2"
])
subexdirs="$3"
if test "$subexdirs" = "" ; then
	subexdirs="-"
fi
for subexdir in $subexdirs ; do
if test "$subexdir" = "-" ; then
	subexdir=""
else
	subexdir="/$subexdir"
fi
for exdir in $exdirs ; do
	if test "$exdir" != "/usr" || test "$subexdir" != ""; then
		incdir="${exdir}/include${subexdir}"
		wi_EXTRA_IDIR($incdir)

		mylibdir="${exdir}/lib${subexdir}"
		wi_EXTRA_LDIR($mylibdir)

		progdir="${exdir}/bin${subexdirr}"
		wi_EXTRA_PDIR($progdir)
	fi
done
done
])
dnl
dnl
dnl
AC_DEFUN(wi_HPUX_CFLAGS,
[AC_MSG_CHECKING(if HP-UX ansi C compiler flags are needed)
AC_REQUIRE([AC_PROG_CC])
os=`uname -s | tr '[A-Z]' '[a-z]'`
ac_cv_hpux_flags=no
if test "$os" = hp-ux ; then
	if test "$ac_cv_prog_gcc" = yes ; then
		if test "$CFLAGS" != "" ; then
			# Shouldn't be in there.
			CFLAGS=`echo "$CFLAGS" | sed 's/-Aa//g'`
		fi
	else
		# If you're not using gcc, then you better have a cc/c89
		# that is usable.  If you have the barebones compiler, it
		# won't work.  The good compiler uses -Aa for the ANSI
		# compatible stuff.
		x=`echo $CFLAGS | grep 'Aa' 2>/dev/null`
		if test "$x" = "" ; then
			CFLAGS="$CFLAGS -Aa"
		fi
		ac_cv_hpux_flags=yes
	fi
	# Also add _HPUX_SOURCE to get the extended namespace.
	x=`echo $CFLAGS | grep '_HPUX_SOURCE' 2>/dev/null`
	if test "$x" = "" ; then
		CFLAGS="$CFLAGS -D_HPUX_SOURCE"
	fi
fi
AC_MSG_RESULT($ac_cv_hpux_flags)
])
dnl
dnl
dnl
AC_DEFUN(wi_CFLAGS, [AC_REQUIRE([AC_PROG_CC])
wi_HPUX_CFLAGS
	if test "$CFLAGS" = "" ; then
		CFLAGS="-O"
	elif test "$ac_cv_prog_gcc" = "yes" ; then
		case "$CFLAGS" in
			*"-g -O"*)
				#echo "using -g as default gcc CFLAGS" 1>&6
				CFLAGS=`echo $CFLAGS | sed 's/-g\ -O/-O/'`
				;;
			*"-O -g"*)
				# Leave the -g, but remove all -O options.
				#echo "using -g as default gcc CFLAGS" 1>&6
				CFLAGS=`echo $CFLAGS | sed 's/-O\ -g/-O/'`
				;;
		esac
	fi
])
dnl
dnl
dnl
AC_DEFUN(wi_PROTOTYPES, [
AC_MSG_CHECKING(if the compiler supports function prototypes)
AC_TRY_COMPILE(,[extern void exit(int status);],[wi_cv_prototypes=yes
AC_DEFINE(PROTOTYPES)],wi_cv_prototypes=no)
AC_MSG_RESULT($wi_cv_prototypes)
])
dnl
dnl
dnl
AC_DEFUN(wi_ANSI_C, [
AC_MSG_CHECKING(ANSI-style function definitions)
AC_TRY_COMPILE(,[int blubb(int x) { return 0; }],[wi_cv_ansi_funcs=yes
AC_DEFINE(ANSI_FUNCS)],wi_cv_ansi_funcs=no)
AC_MSG_RESULT($wi_cv_ansi_funcs)
])
dnl
dnl
dnl
AC_DEFUN(wi_HEADER_SYS_SELECT_H, [
# See if <sys/select.h> is includable after <sys/time.h>
if test "$ac_cv_header_sys_time_h" = no ; then
AC_CHECK_HEADERS(sys/time.h sys/select.h)
else
AC_CHECK_HEADERS(sys/select.h)
fi
if test "$ac_cv_header_sys_select_h" = yes ; then
	AC_MSG_CHECKING([if <sys/select.h> is compatible with <sys/time.h>])
	selecth=yes
	if test "$ac_cv_header_sys_time_h" = yes ; then
		AC_TRY_COMPILE([#include <sys/time.h>
		#include <sys/select.h>],[
		fd_set a;
		struct timeval tmval;

		tmval.tv_sec = 0;],selecth=yes,selecth=no)

		if test "$selecth" = yes ; then
			AC_DEFINE(CAN_USE_SYS_SELECT_H)
		fi
	fi
	AC_MSG_RESULT($selecth)
fi
])
dnl
dnl
dnl
AC_DEFUN(wi_LIB_RESOLV, [
# See if we could access two well-known sites without help of any special
# libraries, like resolv.

AC_TRY_RUN([
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
 
main()
{
	struct hostent *hp1, *hp2;
	int result;

	hp1 = gethostbyname("gatekeeper.dec.com");
	hp2 = gethostbyname("ftp.ncsa.uiuc.edu");
	result = ((hp1 != (struct hostent *) 0) && (hp2 != (struct hostent *) 0));
	exit(! result);
}],look_for_resolv=no,look_for_resolv=yes,look_for_resolv=yes)

AC_MSG_CHECKING([if we need to look for -lresolv])
AC_MSG_RESULT($look_for_resolv)
if test "$look_for_resolv" = yes ; then
AC_CHECK_LIB(resolv,main)
else
	ac_cv_lib_resolv=no
fi
])
dnl
dnl
dnl

AC_DEFUN(wi_LIB_NSL, [
AC_MSG_CHECKING(if we can use -lnsl)
ac_save_LIBS="$LIBS";
LIBS="$LIBS -lnsl";
AC_CACHE_VAL(r_cv_use_libnsl, [
AC_TRY_RUN(
main() { if (getpwuid(getuid())) exit(0); exit(-1); },
nc_cv_use_libnsl=yes, nc_cv_use_libnsl=no, nc_cv_use_libnsl=no)
])
if test "$nc_cv_use_libnsl" = "no"; then LIBS="$ac_save_LIBS"; fi
AC_MSG_RESULT($nc_cv_use_libnsl)
])dnl

dnl
dnl
dnl

AC_DEFUN(nc_PATH_PROG_ZCAT, [
AC_PATH_PROG(GZCAT,gzcat)
AC_PATH_PROG(ZCAT,zcat)
if test "x$GZCAT" = x ; then
	if test "x$ZCAT" != x ; then
		# See if zcat is really gzcat.  gzcat has a --version option, regular
		# zcat does not.
		AC_MSG_CHECKING(if zcat is really gzcat in disguise)
		if $ZCAT --version 2> /dev/null ; then
			AC_DEFINE_UNQUOTED(GZCAT, "$ZCAT")
			AC_MSG_RESULT(yes)
		else
			AC_MSG_RESULT(no)
		fi
	fi
else
	AC_DEFINE_UNQUOTED(GZCAT, "$GZCAT")
fi

if test "x$ZCAT" != x ; then
	AC_DEFINE_UNQUOTED(ZCAT, "$ZCAT")
fi
])
dnl
dnl
dnl
AC_DEFUN(wi_SYSV_EXTRA_DIRS, [
# Use System V because their curses extensions are required.  This must
# be done early so we use the -I and -L in the library checks also.
# This is mostly a Solaris/SunOS hack.  Note that doing this will also
# use all of the other System V libraries and headers.

AC_MSG_CHECKING(for alternative System V libraries)
if test -f /usr/5include/curses.h ; then
	CPPFLAGS="$CPPFLAGS -I/usr/5include"
	LDFLAGS="$LDFLAGS -L/usr/5lib"
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi
])
dnl
dnl
dnl
AC_DEFUN(wi_DEFINE_UNAME, [
# Get first 127 chars of all uname information.  Some folks have
# way too much stuff there, so grab only the first 127.
unam=`uname -a 2>/dev/null | cut -c1-127`
if test "$unam" != "" ; then
	AC_DEFINE_UNQUOTED(UNAME, "$unam")
fi
])
dnl
dnl
dnl
AC_DEFUN(wi_READLINE_WITH_NCURSES, [
# Readline and Ncurses could both define "backspace".
# Warn about this if we have both things in our definitions list.

if test "$ac_cv_lib_readline" = yes && test "$ac_cv_lib_ncurses" = yes ; then

AC_MSG_CHECKING(if readline and ncurses will link together)
j="$LIBS"
LIBS="-lreadline -lncurses"
AC_TRY_LINK(,[
readline("prompt");
endwin();
],k=yes,k=no)
if test "$k" = no ; then
	AC_MSG_RESULT(no)
	# Remove '-lreadline' from LIBS.
	LIBS=`echo $j | sed s/-lreadline//g`
	ac_cv_lib_readline=no
	AC_WARN([The versions of GNU readline and ncurses you have installed on this system
can't be used together, because they use the same symbol, backspace. If
possible, recompile one of the libraries with -Dbackspace=back_space, then
re-run configure.])

else
	AC_MSG_RESULT(yes)
	LIBS="$j"
fi

fi
])
dnl
dnl
dnl

dnl AC_EXT_DAYLIGHT
dnl Check for an external variable daylight. Stolen from w3c-libwww.
AC_DEFUN(AC_EXT_DAYLIGHT,
[ AC_MSG_CHECKING(int daylight variable)
AC_TRY_COMPILE([#include <time.h>], [return daylight;],
  have_daylight=yes,
  have_daylight=no)
AC_MSG_RESULT($have_daylight)
])dnl

dnl AC_EXT_TIMEZONE
dnl Check for an external variable timezone. Stolen from tcl-8.0.
AC_DEFUN(AC_EXT_TIMEZONE,
[
#
# Its important to include time.h in this check, as some systems (like convex)
# have timezone functions, etc.
#
have_timezone=no
AC_MSG_CHECKING([long timezone variable])
AC_TRY_COMPILE([#include <time.h>],
        [extern long timezone;
         timezone += 1;
         exit (0);],
        [have_timezone=yes
         AC_MSG_RESULT(yes)],
         AC_MSG_RESULT(no))

#
# On some systems (eg IRIX 6.2), timezone is a time_t and not a long.
#
if test "$have_timezone" = no; then
   AC_MSG_CHECKING([time_t timezone variable])
   AC_TRY_COMPILE([#include <time.h>],
        [extern time_t timezone;
         timezone += 1;
         exit (0);],
        [have_timezone=yes
         AC_MSG_RESULT(yes)],
         AC_MSG_RESULT(no))
fi
])dnl
