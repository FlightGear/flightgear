/**************************************************************************
 * event.c -- Flight Gear periodic event scheduler
 *
 * Written by Curtis Olson, started December 1997.
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


#include <config.h>

#include <string.h>
#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if defined( HAVE_WINDOWS_H ) && defined(__MWERKS__)
#  include <windows.h>	 /* For Metrowerks environment */
#  include <winbase.h>	 /* There is no ANSI/MSL time function that */
                         /* contains milliseconds */
#endif

#include "fg_time.h"

#include <Main/fg_debug.h>
#include <Time/event.h>


#define MAX_EVENTS 100    /* size of event table */
#define MAX_RUN_QUEUE 100 /* size of run queue */


struct fgEVENT {
    char description[256];

    void (*event)( void );  /* pointer to function */
    int status;       /* status flag */

    long interval;    /* interval in ms between each iteration of this event */

    fg_timestamp last_run;
    fg_timestamp current;
    fg_timestamp next_run;

    long cum_time;    /* cumulative processor time of this event */
    long min_time;    /* time of quickest execution */
    long max_time;    /* time of slowest execution */
    long count;       /* number of times executed */
};


/* Event table */
struct fgEVENT events[MAX_EVENTS];
int event_ptr;


/* Run Queue */
int queue[MAX_RUN_QUEUE];
int queue_front;
int queue_end;


/* initialize the run queue */
void initq( void ) {
    queue_front = queue_end = 0;
}


/* return queue empty status */
int emptyq( void ) {
    if ( queue_front == queue_end ) {
	return(1);
    } else {
	return(0);
    }
}


/* return queue full status */
int fullq( void ) {
    if ( (queue_end + 1) % MAX_RUN_QUEUE == queue_front ) {
	return(1);
    } else {
	return(0);
    }
}


/* add a member to the back of the queue */
void addq(int ptr) {
    if ( !fullq() ) {
	queue[queue_end] = ptr;
	events[ptr].status = FG_EVENT_QUEUED;

	queue_end = (queue_end + 1) % MAX_RUN_QUEUE;
    } else {
	printf("RUN QUEUE FULL!!!\n");
    }

    /* printf("Queued function %d (%d %d)\n", ptr, queue_front, queue_end); */
}


/* remove a member from the front of the queue */
int popq( void ) {
    int ptr;

    if ( emptyq() ) {
	printf("PANIC:  RUN QUEUE IS EMPTY!!!\n");
	ptr = 0;
    } else {
	ptr = queue[queue_front];
	/* printf("Popped position %d = %d\n", queue_front, ptr); */
	queue_front = (queue_front + 1) % MAX_RUN_QUEUE;
    }

    return(ptr);
}


/* run a specified event */
void fgEventRun(int ptr) {
    struct fgEVENT *e;
    long duration;

    e = &events[ptr];
    
    printf("Running %s\n", e->description);

    /* record starting time */
    timestamp(&(e->last_run));

    /* run the event */
    (*e->event)();

    /* increment the counter for this event */
    e->count++;

    /* update the event status */
    e->status = FG_EVENT_READY;

    /* calculate duration and stats */
    timestamp(&(e->current));
    duration = timediff(&(e->last_run), &(e->current));

    e->cum_time += duration;

    if ( duration < e->min_time ) {
	e->min_time = duration;
    }

    if ( duration > e->max_time ) {
	e->max_time = duration;
    }

    /* determine the next absolute run time */
    timesum(&(e->next_run), &(e->last_run), e->interval);
}


/* Initialize the scheduling subsystem */
void fgEventInit( void ) {
    printf("Initializing event manager\n");
    event_ptr = 0;
    initq();
}


/* Register an event with the scheduler, returns a pointer into the
 * event table */
int fgEventRegister(char *desc, void (*event)( void ), int status, 
		    int interval)
{
    struct fgEVENT *e;

    e = &events[event_ptr];

    printf("Registering event: %s\n", desc);

    if ( strlen(desc) < 256 ) {
	strcpy(e->description, desc);
    } else {
	strncpy(e->description, desc, 255);
	e->description[255] = '\0';
    }

    e->event = event;
    e->status = status;
    e->interval = interval;

    e->cum_time = 0;
    e->min_time = 100000;
    e->max_time = 0;
    e->count = 0;

    /* Actually run the event */
    fgEventRun(event_ptr);

    event_ptr++;

    return(event_ptr - 1);
}


/* Update the scheduling parameters for an event */
void fgEventUpdate( void ) {
}


/* Delete a scheduled event */
void fgEventDelete( void ) {
}


/* Temporarily suspend scheduling of an event */
void fgEventSuspend( void ) {
}


/* Resume scheduling and event */
void fgEventResume( void ) {
}


/* Dump scheduling stats */
void fgEventPrintStats( void ) {
    int i;

    if ( event_ptr > 0 ) {
	printf("\n");
	printf("Event Stats\n");
	printf("-----------\n");

	for ( i = 0; i < event_ptr; i++ ) {
	    printf("  %-20s  int=%.2fs cum=%ld min=%ld max=%ld count=%ld ave=%.2f\n",
		   events[i].description, 
		   events[i].interval / 1000.0,
		   events[i].cum_time, 
		   events[i].min_time, events[i].max_time, events[i].count, 
		   events[i].cum_time / (double)events[i].count);
	}
	printf("\n");
    }
}


/* Add pending jobs to the run queue and run the job at the front of
 * the queue */
void fgEventProcess( void ) {
    fg_timestamp current;
    int i;

    fgPrintf(FG_EVENT, FG_DEBUG, "Processing events\n");
    
    /* get the current time */
    timestamp(&current);

    fgPrintf(FG_EVENT, FG_DEBUG, "  Current timestamp = %ld\n", current.seconds);

    /* printf("Checking if anything is ready to move to the run queue\n"); */

    /* see if anything else is ready to be placed on the run queue */
    for ( i = 0; i < event_ptr; i++ ) {
	if ( events[i].status == FG_EVENT_READY ) {
	    fgPrintf(FG_EVENT, FG_DEBUG, 
		     "  Item %d, current %d, next run @ %ld\n", 
		     i, current.seconds, events[i].next_run.seconds);
	    if ( timediff(&current, &(events[i].next_run)) <= 0) {
	        addq(i);
	    }
	}
    }

    /* Checking to see if there is anything on the run queue */
    /* printf("Checking to see if there is anything on the run queue\n"); */
    if ( !emptyq() ) {
	/* printf("Yep, running it\n"); */
	i = popq();
	fgEventRun(i);
    }
}


/* $Log$
/* Revision 1.12  1998/04/09 18:40:13  curt
/* We had unified some of the platform disparate time handling code, and
/* there was a bug in timesum() which calculated a new time stamp based on
/* the current time stamp + offset.  This hosed the periodic event processing
/* logic because you'd never arrive at the time the event was scheduled for.
/* Sky updates and lighting changes are handled via this event mechanism so
/* they never changed ... it is fixed now.
/*
 * Revision 1.11  1998/04/03 22:12:55  curt
 * Converting to Gnu autoconf system.
 * Centralized time handling differences.
 *
 * Revision 1.10  1998/03/14 00:28:34  curt
 * replaced a printf() with an fgPrintf().
 *
 * Revision 1.9  1998/01/31 00:43:44  curt
 * Added MetroWorks patches from Carmen Volpe.
 *
 * Revision 1.8  1998/01/27 00:48:05  curt
 * Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.7  1998/01/19 19:27:19  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.6  1998/01/19 18:40:39  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.5  1998/01/06 01:20:27  curt
 * Tweaks to help building with MSVC++
 *
 * Revision 1.4  1997/12/31 17:46:50  curt
 * Tweaked fg_time.c to be able to use ftime() instead of gettimeofday()
 *
 * Revision 1.3  1997/12/30 22:22:42  curt
 * Further integration of event manager.
 *
 * Revision 1.2  1997/12/30 20:47:58  curt
 * Integrated new event manager with subsystem initializations.
 *
 * Revision 1.1  1997/12/30 04:19:22  curt
 * Initial revision.
 *
 */
