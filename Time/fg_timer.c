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
#include <time.h>      /* for get/setitimer */
#include <sys/timeb.h> /* for ftime() and struct timeb */


#include "fg_timer.h"


unsigned long int fgSimTime;
static struct itimerval t, ot;
static void (*callbackfunc)();


/* This routine catches the SIGALRM */
void fgTimerCatch() {
    /* ignore any SIGALRM's until we come back from our EOM iteration */
    signal(SIGALRM, SIG_IGN);

    /* printf("In fgTimerCatch()\n"); */

    callbackfunc();

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


/* This function returns the number of milleseconds since the last
   time it was called. */
int fgGetTimeInterval() {
    static struct timeb last;
    static struct timeb current;
    static int inited = 0;

    int interval;

    if ( ! inited ) {
	inited = 1;
	ftime(&last);
	interval = 0;
    } else {
	ftime(&current);
	interval = 1000 * (current.time - last.time) + 
	    (current.millitm - last.millitm);
    }

    last = current;
    return(interval);
}


/* $Log$
/* Revision 1.1  1997/06/16 19:24:20  curt
/* Initial revision.
/*
 */
