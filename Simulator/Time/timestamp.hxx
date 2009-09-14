//
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
// (Log is kept at end of this file)


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

#include "Include/compiler.h"
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

#ifdef  WIN32
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
#else
# error Port me
#endif
}

// difference between time stamps in microseconds (usec)
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


// $Log$
// Revision 1.5  1999/02/02 20:13:43  curt
// MSVC++ portability changes by Bernie Bright:
//
// Lib/Serial/serial.[ch]xx: Initial Windows support - incomplete.
// Simulator/Astro/stars.cxx: typo? included <stdio> instead of <cstdio>
// Simulator/Cockpit/hud.cxx: Added Standard headers
// Simulator/Cockpit/panel.cxx: Redefinition of default parameter
// Simulator/Flight/flight.cxx: Replaced cout with FG_LOG.  Deleted <stdio.h>
// Simulator/Main/fg_init.cxx:
// Simulator/Main/GLUTmain.cxx:
// Simulator/Main/options.hxx: Shuffled <fg_serial.hxx> dependency
// Simulator/Objects/material.hxx:
// Simulator/Time/timestamp.hxx: VC++ friend kludge
// Simulator/Scenery/tile.[ch]xx: Fixed using std::X declarations
// Simulator/Main/views.hxx: Added a constant
//
// Revision 1.4  1999/01/09 13:37:46  curt
// Convert fgTIMESTAMP to FGTimeStamp which holds usec instead of ms.
//
// Revision 1.3  1999/01/07 20:25:39  curt
// Portability changes and updates from Bernie Bright.
//
// Revision 1.2  1998/12/11 20:26:56  curt
// #include tweaks.
//
// Revision 1.1  1998/12/05 14:21:33  curt
// Moved struct fg_timestamp to class fgTIMESTAMP and moved it's definition
// to it's own file, timestamp.hxx.
//
