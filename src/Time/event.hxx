// event.hxx -- Flight Gear periodic event scheduler
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


#ifndef _EVENT_HXX
#define _EVENT_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <simgear/compiler.h>

#include <Include/fg_callback.hxx>

#include <deque>        // STL double ended queue
#include <list>         // STL list
#include STL_STRING

#include "timestamp.hxx"

FG_USING_STD(deque);
FG_USING_STD(list);
FG_USING_STD(string);


class fgEVENT
{
public:
    enum EventState
    {
	FG_EVENT_SUSP   = 0,
	FG_EVENT_READY  = 1,
	FG_EVENT_QUEUED = 2
    };

    friend class fgEVENT_MGR;

    fgEVENT() {} // Required by deque<>.

    fgEVENT( const string& desc,
	     const fgCallback& cb,
	     EventState evt_status,
	     int evt_interval );

    ~fgEVENT();

    void run();

//     void PrintStats() const;
    int PrintStats() const;

private:
    // not defined
    fgEVENT( const fgEVENT& evt );
    fgEVENT& operator= ( const fgEVENT& evt );

private:

    string description;

    // The callback object.
    fgCallback* event_cb;

    EventState status;       // status flag

    long interval;    // interval in ms between each iteration of this event

    FGTimeStamp last_run;
    FGTimeStamp current;
    FGTimeStamp next_run;

    long cum_time;    // cumulative processor time of this event
    long min_time;    // time of quickest execution
    long max_time;    // time of slowest execution
    long count;       // number of times executed
};


class fgEVENT_MGR {

    // Event table
    typedef deque < fgEVENT* >             EventContainer;
    typedef EventContainer::iterator       EventIterator;
    typedef EventContainer::const_iterator ConstEventIterator;

    EventContainer event_table;

    // Run Queue
    typedef list < fgEVENT * > RunContainer;

    RunContainer run_queue;

public:

    // Constructor
    fgEVENT_MGR ( void );

    // Initialize the scheduling subsystem
    void Init( void );

    // Register an event with the scheduler
    void Register( const string& desc, void (*event)( void ),
		   fgEVENT::EventState status, int interval) {
	Register( desc, fgFunctionCallback(event), status, interval );
    }

    void Register( const string& desc,
		   const fgCallback& cb,
		   fgEVENT::EventState status, 
		   int interval );

    // Update the scheduling parameters for an event
    void Update( void );

    // Delete a scheduled event
    void Delete( void );

    // Temporarily suspend scheduling of an event
    void Suspend( void );

    // Resume scheduling and event
    void Resume( void );

    // Dump scheduling stats
    void PrintStats( void );

    // Add pending jobs to the run queue and run the job at the front
    // of the queue
    void Process( void );

    // Destructor
    ~fgEVENT_MGR ( void );
};


// Wrapper to dump scheduling stats
void fgEventPrintStats( void );

extern fgEVENT_MGR global_events;


#endif // _EVENT_HXX


