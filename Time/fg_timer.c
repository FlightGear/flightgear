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


#include <signal.h>    /* for timer routines */
#include <stdio.h>     /* for printf() */

#ifdef USE_FTIME
#  include <sys/timeb.h> /* for ftime() and struct timeb */
#else
#  include <sys/time.h>  /* for get/setitimer, gettimeofday, struct timeval */
#endif /* USE_FTIME */

#include "fg_timer.h"


unsigned long int fgSimTime;

#ifdef USE_ITIMER
  static struct itimerval t, ot;
  static void (*callbackfunc)(int multi_loop);


/* This routine catches the SIGALRM */
void fgTimerCatch() {
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
void fgTimerInit(float dt, void (*f)()) {
    int terr;
    int isec;
    float usec;

    callbackfunc = f;

    isec = (int) dt;
    usec = 1000000* (dt - (float) isec);

    t.it_interval.tv_sec = isec;
    t.it_interval.tv_usec = usec;
    t.it_value.tv_sec = isec;
    t.it_value.tv_usec = usec;
    /* printf("fgTimerInit() called\n"); */
    fgTimerCatch();   /* set up for SIGALRM signal catch */
    terr = setitimer( ITIMER_REAL, &t, &ot );
    if (terr) {
	printf("Error returned from setitimer");
	exit(0);
    }
}
#endif /* HAVE_ITIMER */


/* This function returns the number of milleseconds since the last
   time it was called. */
int fgGetTimeInterval() {
    int interval;
    static int inited = 0;

#ifdef USE_FTIME
    static struct timeb last;
    static struct timeb current;
#else
    static struct timeval last;
    static struct timeval current;
    static struct timezone tz;
#endif /* USE_FTIME */

    if ( ! inited ) {
	inited = 1;

#ifdef USE_FTIME
	ftime(&last);
#else
	gettimeofday(&last, &tz);
#endif /* USE_FTIME */

	interval = 0;
    } else {

#ifdef USE_FTIME
	ftime(&current);
	interval = 1000 * (current.time - last.time) + 
	    (current.millitm - last.millitm);
#else
	gettimeofday(&current, &tz);
	interval = 1000000 * (current.tv_sec - last.tv_sec) + 
	    (current.tv_usec - last.tv_usec);
	interval /= 1000;  /* convert back to milleseconds */
#endif /* USE_FTIME */

	last = current;
    }

    return(interval);
}


/* $Log$
/* Revision 1.7  1997/12/30 13:06:58  curt
/* A couple lighting tweaks ...
/*
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
