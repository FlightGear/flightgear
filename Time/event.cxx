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

#include <string>

#include "Include/compiler.h"

#include STL_ALGORITHM
#include STL_FUNCTIONAL

#ifdef FG_HAVE_STD_INCLUDES
#  include <cstdio>
#  ifdef HAVE_STDLIB_H
#    include <cstdlib>
#  endif
#else
#  include <stdio.h>
#  ifdef HAVE_STDLIB_H
#    include <stdlib.h>
#  endif
#endif

#if defined( HAVE_WINDOWS_H ) && defined(__MWERKS__)
#  include <windows.h>	 // For Metrowerks environment
#  include <winbase.h>	 // There is no ANSI/MSL time function that
                         // contains milliseconds
#endif

#include <Debug/logstream.hxx>

#include "event.hxx"

FG_USING_STD(for_each);
FG_USING_STD(mem_fun);

fgEVENT_MGR global_events;


fgEVENT::fgEVENT( const string& desc,
		  const fgCallback& cb,
		  EventState evt_status,
		  int evt_interval )
    : description(desc),
      event_cb(cb.clone()),
      status(evt_status),
      interval(evt_interval),
      cum_time(0),
      min_time(100000),
      max_time(0),
      count(0)
{
}

#if 0
fgEVENT::fgEVENT( const fgEVENT& evt )
    : description(evt.description),
      event_cb(evt.event_cb),
      status(evt.status),
      interval(evt.interval),
      last_run(evt.last_run),
      current(evt.current),
      next_run(evt.next_run),
      cum_time(evt.cum_time),
      min_time(evt.min_time),
      max_time(evt.max_time),
      count(evt.count)
{
}

fgEVENT&
fgEVENT::operator= ( const fgEVENT& evt )
{
    if ( this != &evt )
    {
	description = evt.description;
	event_cb = evt.event_cb;
	status = evt.status;
	interval = evt.interval;
	last_run = evt.last_run;
	current = evt.current;
	next_run = evt.next_run;
	cum_time = evt.cum_time;
	min_time = evt.min_time;
	max_time = evt.max_time;
	count = evt.count;
    }
    return *this;
}
#endif

fgEVENT::~fgEVENT()
{
    delete event_cb;
}

void
fgEVENT::run()
{
    FG_LOG(FG_EVENT, FG_INFO, "Running " << description );

    // record starting time
    last_run.stamp();

    // run the event
    event_cb->call( (void**)NULL );

    // increment the counter for this event
    count++;

    // update the event status
    status = FG_EVENT_READY;

    // calculate duration and stats
    current.stamp();
    long duration = current - last_run;

    cum_time += duration;

    if ( duration < min_time ) {
	min_time = duration;
    }

    if ( duration > max_time ) {
	max_time = duration;
    }

    // determine the next absolute run time
    next_run =  last_run + interval;
}


// Dump scheduling stats
int
fgEVENT::PrintStats() const
{
    FG_LOG( FG_EVENT, FG_INFO, 
	    "  " << description 
	    << " int=" << interval / 1000.0
	    << " cum=" << cum_time
	    << " min=" << min_time
	    << " max=" <<  max_time
	    << " count=" << count
	    << " ave=" << cum_time / (double)count );
    return 0;
}

// Constructor
fgEVENT_MGR::fgEVENT_MGR( void ) {
}


// Initialize the scheduling subsystem
void fgEVENT_MGR::Init( void ) {
    FG_LOG(FG_EVENT, FG_INFO, "Initializing event manager" );

    run_queue.erase( run_queue.begin(), run_queue.end() );
    event_table.erase( event_table.begin(), event_table.end() );
}


// Register an event with the scheduler.
void
fgEVENT_MGR::Register( const string& desc,
		       const fgCallback& cb,
		       fgEVENT::EventState status, 
		       int interval )
{
    // convert interval specified in milleseconds to usec
    fgEVENT* e = new fgEVENT( desc, cb, status, interval * 1000 );

    FG_LOG( FG_EVENT, FG_INFO, "Registering event: " << desc );

    // Actually run the event
    e->run();

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
void
fgEVENT_MGR::PrintStats()
{
    FG_LOG( FG_EVENT, FG_INFO, "" );
    FG_LOG( FG_EVENT, FG_INFO, "Event Stats" );
    FG_LOG( FG_EVENT, FG_INFO, "-----------" );

    ConstEventIterator first = event_table.begin();
    ConstEventIterator last = event_table.end();
    while ( first != last )
    {
        (*first)->PrintStats();
        ++first;
    }
#if 0 // msvc++ 6.0 barfs at mem_fun()
    for_each( event_table.begin(),
	      event_table.end(),
	      mem_fun( &fgEVENT::PrintStats ) );
#endif
    FG_LOG( FG_EVENT, FG_INFO, "");
}


// Add pending jobs to the run queue and run the job at the front of
// the queue
void fgEVENT_MGR::Process( void ) {
    fgEVENT *e_ptr;
    FGTimeStamp cur_time;
    unsigned int i, size;

    FG_LOG( FG_EVENT, FG_DEBUG, "Processing events" );
    
    // get the current time
    cur_time.stamp();

    FG_LOG( FG_EVENT, FG_DEBUG, 
	    "  Current timestamp = " << cur_time.get_seconds() );

    // printf("Checking if anything is ready to move to the run queue\n");

    // see if anything else is ready to be placed on the run queue
    size = event_table.size();
    // while ( current != last ) {
    for ( i = 0; i < size; i++ ) {
	// e = *current++;
	e_ptr = event_table[i];
	if ( e_ptr->status == fgEVENT::FG_EVENT_READY ) {
	    FG_LOG( FG_EVENT, FG_DEBUG, 
		    "  Item " << i << " current " << cur_time.get_seconds()
		    << " next run @ " << e_ptr->next_run.get_seconds() );
	    if ( ( e_ptr->next_run - cur_time ) <= 0 ) {
	        run_queue.push_back(e_ptr);
		e_ptr->status = fgEVENT::FG_EVENT_QUEUED;
	    }
	}
    }

    // Checking to see if there is anything on the run queue
    // printf("Checking to see if there is anything on the run queue\n");
    if ( run_queue.size() ) {
	// printf("Yep, running it\n");
	e_ptr = run_queue.front();
	run_queue.pop_front();
	e_ptr->run();
    }
}


// Destructor
fgEVENT_MGR::~fgEVENT_MGR( void ) {
    EventIterator first = event_table.begin();
    EventIterator last = event_table.end();
    for ( ; first != last; ++first )
    {
        delete (*first);
    }

    run_queue.erase( run_queue.begin(), run_queue.end() );
    event_table.erase( event_table.begin(), event_table.end() );
}


// $Log$
// Revision 1.15  1999/01/09 13:37:42  curt
// Convert fgTIMESTAMP to FGTimeStamp which holds usec instead of ms.
//
// Revision 1.14  1999/01/07 20:25:32  curt
// Portability changes and updates from Bernie Bright.
//
// Revision 1.13  1998/12/04 01:32:46  curt
// Converted "struct fg_timestamp" to "class fgTIMESTAMP" and added some
// convenience inline operators.
//
// Revision 1.12  1998/11/23 21:49:07  curt
// Borland portability tweaks.
//
// Revision 1.11  1998/11/09 23:41:51  curt
// Log message clean ups.
//
// Revision 1.10  1998/11/07 19:07:13  curt
// Enable release builds using the --without-logging option to the configure
// script.  Also a couple log message cleanups, plus some C to C++ comment
// conversion.
//
// Revision 1.9  1998/11/06 21:18:24  curt
// Converted to new logstream debugging facility.  This allows release
// builds with no messages at all (and no performance impact) by using
// the -DFG_NDEBUG flag.
//
// Revision 1.8  1998/09/15 02:09:29  curt
// Include/fg_callback.hxx
//   Moved code inline to stop g++ 2.7 from complaining.
//
// Simulator/Time/event.[ch]xx
//   Changed return type of fgEVENT::printStat().  void caused g++ 2.7 to
//   complain bitterly.
//
// Minor bugfix and changes.
//
// Simulator/Main/GLUTmain.cxx
//   Added missing type to idle_state definition - eliminates a warning.
//
// Simulator/Main/fg_init.cxx
//   Changes to airport lookup.
//
// Simulator/Main/options.cxx
//   Uses fg_gzifstream when loading config file.
//
// Revision 1.7  1998/08/29 13:11:31  curt
// Bernie Bright writes:
//   I've created some new classes to enable pointers-to-functions and
//   pointers-to-class-methods to be treated like objects.  These objects
//   can be registered with fgEVENT_MGR.
//
//   File "Include/fg_callback.hxx" contains the callback class defns.
//
//   Modified fgEVENT and fgEVENT_MGR to use the callback classes.  Also
//   some minor tweaks to STL usage.
//
//   Added file "Include/fg_stl_config.h" to deal with STL portability
//   issues.  I've added an initial config for egcs (and probably gcc-2.8.x).
//   I don't have access to Visual C++ so I've left that for someone else.
//   This file is influenced by the stl_config.h file delivered with egcs.
//
//   Added "Include/auto_ptr.hxx" which contains an implementation of the
//   STL auto_ptr class which is not provided in all STL implementations
//   and is needed to use the callback classes.
//
//   Deleted fgLightUpdate() which was just a wrapper to call
//   fgLIGHT::Update().
//
//   Modified fg_init.cxx to register two method callbacks in place of the
//   old wrapper functions.
//
// Revision 1.6  1998/08/20 15:12:26  curt
// Tweak ...
//
// Revision 1.5  1998/06/12 00:59:52  curt
// Build only static libraries.
// Declare memmove/memset for Sloaris.
// Rewrote fg_time.c routine to get LST start seconds to better handle
//   Solaris, and be easier to port, and understand the GMT vs. local
//   timezone issues.
//
// Revision 1.4  1998/06/05 18:18:12  curt
// Incorporated some automake conditionals to try to support mktime() correctly
// on a wider variety of platforms.
// Added the declaration of memmove needed by the stl which apparently
// solaris only defines for cc compilations and not for c++ (__STDC__)
//
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
