// event.cxx -- Flight Gear periodic event scheduler
//
// Written by Curtis Olson, started December 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if defined( HAVE_WINDOWS_H ) && defined(__MWERKS__)
#  include <windows.h>	 // For Metrowerks environment
#  include <winbase.h>	 // There is no ANSI/MSL time function that
                         // contains milliseconds
#endif

#include <Debug/fg_debug.h>

#include "event.hxx"


fgEVENT_MGR global_events;


// run a specified event
static void RunEvent(fgEVENT *e) {
    long duration;

    printf("Running %s\n", e->description);

    // record starting time
    timestamp(&(e->last_run));

    // run the event
    (*e->event)();

    // increment the counter for this event
    e->count++;

    // update the event status
    e->status = FG_EVENT_READY;

    // calculate duration and stats
    timestamp(&(e->current));
    duration = timediff(&(e->last_run), &(e->current));

    e->cum_time += duration;

    if ( duration < e->min_time ) {
	e->min_time = duration;
    }

    if ( duration > e->max_time ) {
	e->max_time = duration;
    }

    // determine the next absolute run time
    timesum(&(e->next_run), &(e->last_run), e->interval);
}


// Constructor
fgEVENT_MGR::fgEVENT_MGR( void ) {
}


// Initialize the scheduling subsystem
void fgEVENT_MGR::Init( void ) {
    printf("Initializing event manager\n");
    // clear event table
    while ( event_table.size() ) {
	event_table.pop_front();
    }

    // clear run queue
    while ( run_queue.size() ) {
	run_queue.pop_front();
    }
}


// Register an event with the scheduler
void fgEVENT_MGR::Register(char *desc, void (*event)( void ), int status, 
		    int interval)
{
    fgEVENT e;

    printf("Registering event: %s\n", desc);

    if ( strlen(desc) < 256 ) {
	strcpy(e.description, desc);
    } else {
	strncpy(e.description, desc, 255);
	e.description[255] = '\0';
    }

    e.event = event;
    e.status = status;
    e.interval = interval;

    e.cum_time = 0;
    e.min_time = 100000;
    e.max_time = 0;
    e.count = 0;

    // Actually run the event
    RunEvent(&e);

    // Now add to event_table
    event_table.push_back(e);
}


// Update the scheduling parameters for an event
void fgEVENT_MGR::Update( void ) {
}


// Delete a scheduled event
void fgEVENT_MGR::Delete( void ) {
}


// Temporarily suspend scheduling of an event
void fgEVENT_MGR::Suspend( void ) {
}


// Resume scheduling and event
void fgEVENT_MGR::Resume( void ) {
}


// Dump scheduling stats
void fgEVENT_MGR::PrintStats( void ) {
    deque < fgEVENT > :: iterator current = event_table.begin();
    deque < fgEVENT > :: iterator last    = event_table.end();
    fgEVENT e;

    printf("\n");
    printf("Event Stats\n");
    printf("-----------\n");

    while ( current != last ) {
	e = *current++;
	printf("  %-20s int=%.2fs cum=%ld min=%ld max=%ld count=%ld ave=%.2f\n",
	       e.description, 
	       e.interval / 1000.0,
	       e.cum_time, e.min_time, e.max_time, e.count, 
	       e.cum_time / (double)e.count);
    }

    printf("\n");
}


// Add pending jobs to the run queue and run the job at the front of
// the queue
void fgEVENT_MGR::Process( void ) {
    fgEVENT *e_ptr;
    fg_timestamp cur_time;
    unsigned int i, size;

    fgPrintf(FG_EVENT, FG_DEBUG, "Processing events\n");
    
    // get the current time
    timestamp(&cur_time);

    fgPrintf( FG_EVENT, FG_DEBUG, 
	      "  Current timestamp = %ld\n", cur_time.seconds);

    // printf("Checking if anything is ready to move to the run queue\n");

    // see if anything else is ready to be placed on the run queue
    size = event_table.size();
    // while ( current != last ) {
    for ( i = 0; i < size; i++ ) {
	// e = *current++;
	e_ptr = &event_table[i];
	if ( e_ptr->status == FG_EVENT_READY ) {
	    fgPrintf(FG_EVENT, FG_DEBUG, 
		     "  Item %d, current %d, next run @ %ld\n", 
		     i, cur_time.seconds, e_ptr->next_run.seconds);
	    if ( timediff(&cur_time, &(e_ptr->next_run)) <= 0) {
	        run_queue.push_back(e_ptr);
		e_ptr->status = FG_EVENT_QUEUED;
	    }
	}
    }

    // Checking to see if there is anything on the run queue
    // printf("Checking to see if there is anything on the run queue\n");
    if ( run_queue.size() ) {
	// printf("Yep, running it\n");
	e_ptr = run_queue.front();
	run_queue.pop_front();
	RunEvent(e_ptr);
    }
}


// Destructor
fgEVENT_MGR::~fgEVENT_MGR( void ) {
}


void fgEventPrintStats( void ) {
    global_events.PrintStats();
}


// $Log$
// Revision 1.3  1998/05/22 21:14:53  curt
// Rewrote event.cxx in C++ as a class using STL for the internal event list
// and run queue this removes the arbitrary list sizes and makes things much
// more dynamic.  Because this is C++-classified we can now have multiple
// event_tables if we'd ever want them.
//
// Revision 1.2  1998/04/25 22:06:33  curt
// Edited cvs log messages in source files ... bad bad bad!
//
// Revision 1.1  1998/04/24 00:52:26  curt
// Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
// Fog color fixes.
// Separated out lighting calcs into their own file.
//
// Revision 1.13  1998/04/18 04:14:08  curt
// Moved fg_debug.c to it's own library.
//
// Revision 1.12  1998/04/09 18:40:13  curt
// We had unified some of the platform disparate time handling code, and
// there was a bug in timesum() which calculated a new time stamp based on
// the current time stamp + offset.  This hosed the periodic event processing
// logic because you'd never arrive at the time the event was scheduled for.
// Sky updates and lighting changes are handled via this event mechanism so
// they never changed ... it is fixed now.
//
// Revision 1.11  1998/04/03 22:12:55  curt
// Converting to Gnu autoconf system.
// Centralized time handling differences.
//
// Revision 1.10  1998/03/14 00:28:34  curt
// replaced a printf() with an fgPrintf().
//
// Revision 1.9  1998/01/31 00:43:44  curt
// Added MetroWorks patches from Carmen Volpe.
//
// Revision 1.8  1998/01/27 00:48:05  curt
// Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
// system and commandline/config file processing code.
//
// Revision 1.7  1998/01/19 19:27:19  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.6  1998/01/19 18:40:39  curt
// Tons of little changes to clean up the code and to remove fatal errors
// when building with the c++ compiler.
//
// Revision 1.5  1998/01/06 01:20:27  curt
// Tweaks to help building with MSVC++
//
// Revision 1.4  1997/12/31 17:46:50  curt
// Tweaked fg_time.c to be able to use ftime() instead of gettimeofday()
//
// Revision 1.3  1997/12/30 22:22:42  curt
// Further integration of event manager.
//
// Revision 1.2  1997/12/30 20:47:58  curt
// Integrated new event manager with subsystem initializations.
//
// Revision 1.1  1997/12/30 04:19:22  curt
// Initial revision.
