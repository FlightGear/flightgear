// timestamp.hxx -- class for managing a timestamp (seconds & milliseconds.)
//
// Written by Curtis Olson, started December 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@flightgear.org
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifndef _TIMESTAMP_HXX
#define _TIMESTAMP_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <simgear/compiler.h>

#ifdef FG_HAVE_STD_INCLUDES
#  include <ctime>
#else
#  include <time.h>
#endif

#ifdef HAVE_SYS_TIMEB_H
#  include <sys/timeb.h> // for ftime() and struct timeb
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h>    // for gettimeofday()
#endif
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>  // for get/setitimer, gettimeofday, struct timeval
#endif

// -dw- want to use metrowerks time.h
#ifdef macintosh
#  include <time.h>
#  include <timer.h>
#endif

#ifdef WIN32
#  include <windows.h>
#  if defined( __CYGWIN__ ) || defined( __CYGWIN32__ )
#    define NEAR /* */
#    define FAR  /* */
#  endif
#  include <mmsystem.h>
#endif

// MSVC++ 6.0 kuldge - Need forward declaration of friends.
class FGTimeStamp;
FGTimeStamp operator + (const FGTimeStamp& t, const long& m);
long operator - (const FGTimeStamp& a, const FGTimeStamp& b);

class FGTimeStamp {

private:

    long seconds;
    long usec;

public:

    FGTimeStamp();
    FGTimeStamp( const long s, const long m );
    ~FGTimeStamp();

    // Set time to current time
    void stamp();

    FGTimeStamp& operator = ( const FGTimeStamp& t );

    friend FGTimeStamp operator + (const FGTimeStamp& t, const long& m);
    friend long operator - (const FGTimeStamp& a, const FGTimeStamp& b);

    inline long get_seconds() const { return seconds; }
    // inline long get_usec() const { return usec; }
};

inline FGTimeStamp::FGTimeStamp() {
}

inline FGTimeStamp::FGTimeStamp( const long s, const long u ) {
    seconds = s;
    usec = u;
}

inline FGTimeStamp::~FGTimeStamp() {
}

inline FGTimeStamp& FGTimeStamp::operator = (const FGTimeStamp& t)
{
    seconds = t.seconds;
    usec = t.usec;
    return *this;
}

inline void FGTimeStamp::stamp() {
#if defined( WIN32 )
    unsigned int t;
    t = timeGetTime();
    seconds = 0;
    usec =  t * 1000;
#elif defined( HAVE_GETTIMEOFDAY )
    struct timeval current;
    struct timezone tz;
    // fg_timestamp currtime;
    gettimeofday(&current, &tz);
    seconds = current.tv_sec;
    usec = current.tv_usec;
#elif defined( HAVE_GETLOCALTIME )
    SYSTEMTIME current;
    GetLocalTime(&current);
    seconds = current.wSecond;
    usec = current.wMilliseconds * 1000;
#elif defined( HAVE_FTIME )
    struct timeb current;
    ftime(&current);
    seconds = current.time;
    usec = current.millitm * 1000;
// -dw- uses time manager
#elif defined( macintosh )
    UnsignedWide ms;
    Microseconds(&ms);
	
    seconds = ms.lo / 1000000;
    usec = ms.lo - ( seconds * 1000000 );
#else
# error Port me
#endif
}

// increment the time stamp by the number of microseconds (usec)
inline FGTimeStamp operator + (const FGTimeStamp& t, const long& m) {
#ifdef WIN32
    return FGTimeStamp( 0, t.usec + m );
#else
    return FGTimeStamp( t.seconds + ( t.usec + m ) / 1000000,
			( t.usec + m ) % 1000000 );
#endif
}

// difference between time stamps in microseconds (usec)
inline long operator - (const FGTimeStamp& a, const FGTimeStamp& b)
{
#if defined( WIN32 )
    return a.usec - b.usec;
#else
    return 1000000 * (a.seconds - b.seconds) + (a.usec - b.usec);
#endif
}


#endif // _TIMESTAMP_HXX


