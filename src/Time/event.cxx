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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#ifdef FG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include STL_ALGORITHM
#include STL_FUNCTIONAL
#include STL_STRING

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

#include <simgear/debug/logstream.hxx>

#include "event.hxx"

FG_USING_STD(for_each);
FG_USING_STD(mem_fun);
FG_USING_STD(string);

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


