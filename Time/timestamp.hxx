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

#include <time.h>

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


class fgTIMESTAMP {

private:

    long seconds;
    long millis;

public:

    fgTIMESTAMP();
    fgTIMESTAMP( const long s, const long m );
    ~fgTIMESTAMP();

    // Set time to current time
    void stamp();

    fgTIMESTAMP& operator = ( const fgTIMESTAMP& t );

    friend fgTIMESTAMP operator + (const fgTIMESTAMP& t, const long& m);
    friend long operator - (const fgTIMESTAMP& a, const fgTIMESTAMP& b);

    inline long get_seconds() const { return seconds; }
    inline long get_millis() const { return millis; }
};

inline fgTIMESTAMP::fgTIMESTAMP() {
}

inline fgTIMESTAMP::fgTIMESTAMP( const long s, const long m ) {
    seconds = s;
    millis = m;
}

inline fgTIMESTAMP::~fgTIMESTAMP() {
}

inline fgTIMESTAMP& fgTIMESTAMP::operator = (const fgTIMESTAMP& t)
{
    seconds = t.seconds;
    millis = t.millis;
    return *this;
}

inline void fgTIMESTAMP::stamp() {
#if defined( WIN32 )
    unsigned int t;
    t = timeGetTime();
    seconds = 0;
    millis =  t;
#elif defined( HAVE_GETTIMEOFDAY )
    struct timeval current;
    struct timezone tz;
    // fg_timestamp currtime;
    gettimeofday(&current, &tz);
    seconds = current.tv_sec;
    millis = current.tv_usec / 1000;
#elif defined( HAVE_GETLOCALTIME )
    SYSTEMTIME current;
    GetLocalTime(&current);
    seconds = current.wSecond;
    millis = current.wMilliseconds;
#elif defined( HAVE_FTIME )
    struct timeb current;
    ftime(&current);
    seconds = current.time;
    millis = current.millitm;
#else
# error Port me
#endif
}

// difference between time stamps in milliseconds
inline fgTIMESTAMP operator + (const fgTIMESTAMP& t, const long& m) {
#ifdef WIN32
    return fgTIMESTAMP( 0, t.millis + m );
#else
    return fgTIMESTAMP( t.seconds + ( t.millis + m ) / 1000,
			( t.millis + m ) % 1000 );
#endif
}

// difference between time stamps in milliseconds
inline long operator - (const fgTIMESTAMP& a, const fgTIMESTAMP& b)
{
#if defined( WIN32 )
    return a.millis - b.millis;
#else
    return 1000 * (a.seconds - b.seconds) + (a.millis - b.millis);
#endif
}


#endif // _TIMESTAMP_HXX


// $Log$
// Revision 1.2  1998/12/11 20:26:56  curt
// #include tweaks.
//
// Revision 1.1  1998/12/05 14:21:33  curt
// Moved struct fg_timestamp to class fgTIMESTAMP and moved it's definition
// to it's own file, timestamp.hxx.
//
