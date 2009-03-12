// fg_timer.cxx -- time handling routines
//
// Written by Curtis Olson, started June 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <signal.h>    // for timer routines
#include <stdio.h>     // for printf()

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif

#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>  // for get/setitimer, gettimeofday, struct timeval
#endif

#include <simgear/timing/timestamp.hxx>

#include "fg_timer.hxx"


unsigned long int fgSimTime;

#ifdef HAVE_SETITIMER
  static struct itimerval t, ot;
  static void (*callbackfunc)(int multi_loop);


// This routine catches the SIGALRM
void fgTimerCatch( int dummy ) {
    int warning_avoider;

    // get past a compiler warning
    warning_avoider = dummy;

    // ignore any SIGALRM's until we come back from our EOM iteration
    signal(SIGALRM, SIG_IGN);

    // printf("In fgTimerCatch()\n");

    // -1 tells the routine to use default interval rather than
    // something dynamically calculated based on frame rate
    callbackfunc(-1); 

    signal(SIGALRM, fgTimerCatch);
}


// this routine initializes the interval timer to generate a SIGALRM
// after the specified interval (dt)
void fgTimerInit(float dt, void (*f)( int )) {
    int terr;
    int isec;
    int usec;

    callbackfunc = f;

    isec = (int) dt;
    usec = 1000000 * ((int)dt - isec);

    t.it_interval.tv_sec = isec;
    t.it_interval.tv_usec = usec;
    t.it_value.tv_sec = isec;
    t.it_value.tv_usec = usec;
    // printf("fgTimerInit() called\n");
    fgTimerCatch(0);   // set up for SIGALRM signal catch
    terr = setitimer( ITIMER_REAL, &t, &ot );
    if (terr) {
	printf("Error returned from setitimer");
	exit(0);
    }
}
#endif // HAVE_SETITIMER


// This function returns the number of microseconds since the last
// time it was called.
int fgGetTimeInterval( void ) {
    int interval;
    static int inited = 0;
    static SGTimeStamp last;
    SGTimeStamp current;

    
    if ( ! inited ) {
	inited = 1;
	last.stamp();
	interval = 0;
    } else {
        current.stamp();
	interval = (current - last).toUSecs();
	last = current;
    }

    return interval;
}


