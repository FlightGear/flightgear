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


#include <string.h>
#include <stdio.h>

#ifdef USE_FTIME
#  include <stdlib.h>
#  include <sys/timeb.h> /* for ftime() and struct timeb */
#else
#  include <sys/time.h>  /* for get/setitimer, gettimeofday, struct timeval */
#endif /* USE_FTIME */


#include <Time/event.h>


#define MAX_EVENTS 100    /* size of event table */
#define MAX_RUN_QUEUE 100 /* size of run queue */


struct fgEVENT {
    char description[256];

    void (*event)( void );  /* pointer to function */
    int status;       /* status flag */

    long interval;    /* interval in ms between each iteration of this event */
                      
#ifdef USE_FTIME
    struct timeb last_run;    /* absolute time for last run */
    struct timeb current;     /* current time */
    struct timeb next_run;    /* absolute time for next run */
#else
    struct timeval last_run;  /* absolute time for last run */
    struct timeval current;   /* current time */
    struct timeval next_run;  /* absolute time for next run */
    struct timezone tz;
#endif /* USE_FTIME */

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
#ifdef USE_FTIME
    ftime(&e->last_run);
#else
    gettimeofday(&e->last_run, &e->tz);
#endif /* USE_FTIME */

    /* run the event */
    (*e->event)();

    /* increment the counter for this event */
    e->count++;

    /* update the event status */
    e->status = FG_EVENT_READY;

    /* calculate duration and stats */
#ifdef USE_FTIME
    ftime(&e->current);
    duration = 1000 * (e->current.time - e->last_run.time) + 
	(e->current.millitm - e->last_run.millitm);
#else
    gettimeofday(&e->current, &e->tz);
    duration = 1000000 * (e->current.tv_sec - e->last_run.tv_sec) + 
	(e->current.tv_usec - e->last_run.tv_usec);
    duration /= 1000;  /* convert back to milleseconds */
#endif /* USE_FTIME */

    e->cum_time += duration;

    if ( duration < e->min_time ) {
	e->min_time = duration;
    }

    if ( duration > e->max_time ) {
	e->max_time = duration;
    }

    /* determine the next absolute run time */
#ifdef USE_FTIME
    e->next_run.time = e->last_run.time + 
	(e->last_run.millitm + e->interval) / 1000;
    e->next_run.millitm = (e->last_run.millitm + e->interval) % 1000;
#else
    e->next_run.tv_sec = e->last_run.tv_sec +
	(e->last_run.tv_usec + e->interval * 1000) / 1000000;
    e->next_run.tv_usec = (e->last_run.tv_usec + e->interval * 1000) % 1000000;
#endif /* USE_FTIME */

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
#ifdef USE_FTIME
    struct timeb current;
#else
    struct timeval current;
    struct timezone tz;
#endif /* USE_FTIME */
    int i;

    printf("Processing events\n");
    
    /* get the current time */
#ifdef USE_FTIME
    ftime(&current);
#else
    gettimeofday(&current, &tz);
#endif /* USE_FTIME */

    /* printf("Checking if anything is ready to move to the run queue\n"); */

    /* see if anything else is ready to be placed on the run queue */
    for ( i = 0; i < event_ptr; i++ ) {
	if ( events[i].status == FG_EVENT_READY ) {
#ifdef USE_FTIME
	    if ( current.time > events[i].next_run.time ) {
		addq(i);
	    } else if ( (current.time == events[i].next_run.time) && 
			(current.millitm >= events[i].next_run.millitm) ) {
		addq(i);
	    }
#else
	    if ( current.tv_sec > events[i].next_run.tv_sec ) {
		addq(i);
	    } else if ( (current.tv_sec == events[i].next_run.tv_sec) && 
			(current.tv_usec >= events[i].next_run.tv_usec) ) {
		addq(i);
	    }

#endif /* USE_FTIME */
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
/* Revision 1.8  1998/01/27 00:48:05  curt
/* Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
/* system and commandline/config file processing code.
/*
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
