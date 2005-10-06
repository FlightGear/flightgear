// test the systems mktime() function


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <math.h>
#include <stdio.h>
#include <time.h>

#ifdef HAVE_SYS_TIMEB_H
#  include <sys/types.h>
#  include <sys/timeb.h> // for ftime() and struct timeb
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h>    // for gettimeofday()
#endif
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>  // for get/setitimer, gettimeofday, struct timeval
#endif

#define LST_MAGIC_TIME_1998 890481600

// For now we assume that if daylight is not defined in
// /usr/include/time.h that we have a machine with a BSD behaving
// mktime()
#if !defined(HAVE_DAYLIGHT)
#  define MK_TIME_IS_GMT 1
#endif


// Fix up timezone if using ftime()
long int fix_up_timezone( long int timezone_orig ) {
#if !defined( HAVE_GETTIMEOFDAY ) && defined( HAVE_FTIME )
    // ftime() needs a little extra help finding the current timezone
    struct timeb current;
    ftime(&current);
    return( current.timezone * 60 );
#else
    return( timezone_orig );
#endif
}


// Return time_t for Sat Mar 21 12:00:00 GMT
//
// I believe the mktime() has a SYSV vs. BSD behavior difference.
//
// The BSD style mktime() is nice because it returns its result
// assuming you have specified the input time in GMT
//
// The SYSV style mktime() is a pain because it returns its result
// assuming you have specified the input time in your local timezone.
// Therefore you have to go to extra trouble to convert back to GMT.
//
// If you are having problems with incorrectly positioned astronomical
// bodies, this is a really good place to start looking.

time_t get_start_gmt(int year) {
    struct tm mt;

    mt.tm_mon = 2;
    mt.tm_mday = 21;
    mt.tm_year = year;
    mt.tm_hour = 12;
    mt.tm_min = 0;
    mt.tm_sec = 0;
    mt.tm_isdst = -1; // let the system determine the proper time zone

#if defined( HAVE_TIMEGM ) 
    return ( timegm(&mt) );
#elif defined( MK_TIME_IS_GMT )
    return ( mktime(&mt) );
#else // ! defined ( MK_TIME_IS_GMT )

    // timezone seems to work as a proper offset for Linux & Solaris
#  if defined( __linux__ ) || defined(__sun) || defined( __CYGWIN__ )
#   define TIMEZONE_OFFSET_WORKS 1
#  endif

#if defined(__CYGWIN__)
#define TIMEZONE _timezone
#else
#define TIMEZONE timezone
#endif

    time_t start = mktime(&mt);

    printf("start1 = %ld\n", start);
    printf("start2 = %s", ctime(&start));
    printf("(tm_isdst = %d)\n", mt.tm_isdst);

    TIMEZONE = fix_up_timezone( TIMEZONE );
	
#  if defined( TIMEZONE_OFFSET_WORKS )
    printf("start = %ld, timezone = %ld\n", start, TIMEZONE);
    return( start - TIMEZONE );
#  else // ! defined( TIMEZONE_OFFSET_WORKS )

    daylight = mt.tm_isdst;
    if ( daylight > 0 ) {
	daylight = 1;
    } else if ( daylight < 0 ) {
	printf("OOOPS, problem in fg_time.cxx, no daylight savings info.\n");
    }

    long int offset = -(TIMEZONE / 3600 - daylight);

    printf("  Raw time zone offset = %ld\n", TIMEZONE);
    printf("  Daylight Savings = %d\n", daylight);
    printf("  Local hours from GMT = %ld\n", offset);
    
    long int start_gmt = start - TIMEZONE + (daylight * 3600);
    
    printf("  March 21 noon (CST) = %ld\n", start);

    return ( start_gmt );
#  endif // ! defined( TIMEZONE_OFFSET_WORKS )
#endif // ! defined ( MK_TIME_IS_GMT )
}


int main() {
    time_t start_gmt;

    start_gmt = get_start_gmt(98);


    if ( start_gmt == LST_MAGIC_TIME_1998 ) {
	printf("Time test = PASSED\n\n");
#ifdef HAVE_TIMEGM
	printf("You have timegm() which is just like mktime() except that\n");
	printf("it explicitely expects input in GMT ... lucky you!\n");
#elif MK_TIME_IS_GMT
	printf("You don't seem to have timegm(), but mktime() seems to\n");
	printf("assume input is GMT on your system ... I guess that works\n");
#else
	printf("mktime() assumes local time zone on your system, but we can\n");
	printf("compensate just fine.\n");
#endif
    } else {
	printf("Time test = FAILED\n\n");
	printf("There is likely a problem with mktime() on your system.\n");
        printf("This will cause the sun/moon/stars/planets to be in the\n");
	printf("wrong place in the sky and the rendered time of day will be\n");
	printf("incorrect.\n\n");
	printf("Please report this to http://www.flightgear.org/~curt so we can work to fix\n");
	printf("the problem on your platform.\n");
    }
}
