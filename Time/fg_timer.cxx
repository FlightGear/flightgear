/**************************************************************************
 * fg_timer.c -- time handling routines
 *
 * Written by Curtis Olson, started June 1997.
 *
 * Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <signal.h>    /* for timer routines */
#include <stdio.h>     /* for printf() */

#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>  /* for get/setitimer, gettimeofday, struct timeval */
#endif

#include "fg_time.hxx"
#include "fg_timer.hxx"


unsigned long int fgSimTime;

#ifdef HAVE_SETITIMER
  static struct itimerval t, ot;
  static void (*callbackfunc)(int multi_loop);


/* This routine catches the SIGALRM */
void fgTimerCatch( int dummy ) {
    int warning_avoider;

    // get past a compiler warning
    warning_avoider = dummy;

    /* ignore any SIGALRM's until we come back from our EOM iteration */
    signal(SIGALRM, SIG_IGN);

    /* printf("In fgTimerCatch()\n"); */

    /* -1 tells the routine to use default interval rather than something
       dynamically calculated based on frame rate */
    callbackfunc(-1); 

    signal(SIGALRM, fgTimerCatch);
}


/* this routine initializes the interval timer to generate a SIGALRM after
 * the specified interval (dt) */
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
    /* printf("fgTimerInit() called\n"); */
    fgTimerCatch(0);   /* set up for SIGALRM signal catch */
    terr = setitimer( ITIMER_REAL, &t, &ot );
    if (terr) {
	printf("Error returned from setitimer");
	exit(0);
    }
}
#endif /* HAVE_SETITIMER */


/* This function returns the number of milleseconds since the last
   time it was called. */
int fgGetTimeInterval( void ) {
    int interval;
    static int inited = 0;
    static fg_timestamp last;
    fg_timestamp current;

    if ( ! inited ) {
	inited = 1;
	timestamp(&last);
	interval = 0;
    } else {
        timestamp(&current);
	interval = timediff(&last, &current);
	last.seconds = current.seconds;
	last.millis = current.millis;
    }

    return(interval);
}


/* $Log$
/* Revision 1.2  1998/04/25 20:24:03  curt
/* Cleaned up initialization sequence to eliminate interdependencies
/* between sun position, lighting, and view position.  This creates a
/* valid single pass initialization path.
/*
 * Revision 1.1  1998/04/24 00:52:29  curt
 * Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
 * Fog color fixes.
 * Separated out lighting calcs into their own file.
 *
 * Revision 1.12  1998/04/21 17:01:44  curt
 * Fixed a problems where a pointer to a function was being passed around.  In
 * one place this functions arguments were defined as ( void ) while in another
 * place they were defined as ( int ).  The correct answer was ( int ).
 *
 * Prepairing for C++ integration.
 *
 * Revision 1.11  1998/04/03 22:12:56  curt
 * Converting to Gnu autoconf system.
 * Centralized time handling differences.
 *
 * Revision 1.10  1998/01/31 00:43:45  curt
 * Added MetroWorks patches from Carmen Volpe.
 *
 * Revision 1.9  1998/01/19 19:27:21  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.8  1998/01/19 18:40:39  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.7  1997/12/30 13:06:58  curt
 * A couple lighting tweaks ...
 *
 * Revision 1.6  1997/07/12 02:13:04  curt
 * Add ftime() support for those that don't have gettimeofday()
 *
 * Revision 1.5  1997/06/26 19:08:38  curt
 * Restructuring make, adding automatic "make dep" support.
 *
 * Revision 1.4  1997/06/25 15:39:49  curt
 * Minor changes to compile with rsxnt/win32.
 *
 * Revision 1.3  1997/06/17 16:52:04  curt
 * Timer interval stuff now uses gettimeofday() instead of ftime()
 *
 * Revision 1.2  1997/06/17 03:41:10  curt
 * Nonsignal based interval timing is now working.
 * This would be a good time to look at cleaning up the code structure a bit.
 *
 * Revision 1.1  1997/06/16 19:24:20  curt
 * Initial revision.
 *
 */
