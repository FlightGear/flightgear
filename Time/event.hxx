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
// (Log is kept at end of this file)


#ifndef _EVENT_HXX
#define _EVENT_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <deque.h>      // STL double ended queue
#include <list.h>       // STL list

#include "fg_time.hxx"


#define FG_EVENT_SUSP 0
#define FG_EVENT_READY 1
#define FG_EVENT_QUEUED 2


typedef struct {
    char description[256];

    void (*event)( void );  // pointer to function
    int status;       // status flag

    long interval;    // interval in ms between each iteration of this event

    fg_timestamp last_run;
    fg_timestamp current;
    fg_timestamp next_run;

    long cum_time;    // cumulative processor time of this event
    long min_time;    // time of quickest execution
    long max_time;    // time of slowest execution
    long count;       // number of times executed
} fgEVENT;


class fgEVENT_MGR {

    // Event table
    deque < fgEVENT > event_table;

    // Run Queue
    list < fgEVENT * > run_queue;

public:

    // Constructor
    fgEVENT_MGR ( void );

    // Initialize the scheduling subsystem
    void Init( void );

    // Register an event with the scheduler
    void Register(char *desc, void (*event)( void ), int status, 
			 int interval);

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


// $Log$
// Revision 1.2  1998/05/22 21:14:54  curt
// Rewrote event.cxx in C++ as a class using STL for the internal event list
// and run queue this removes the arbitrary list sizes and makes things much
// more dynamic.  Because this is C++-classified we can now have multiple
// event_tables if we'd ever want them.
//
// Revision 1.1  1998/04/24 00:52:26  curt
// Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
// Fog color fixes.
// Separated out lighting calcs into their own file.
//
// Revision 1.4  1998/04/21 17:01:43  curt
// Fixed a problems where a pointer to a function was being passed around.  In
// one place this functions arguments were defined as ( void ) while in another
// place they were defined as ( int ).  The correct answer was ( int ).
//
// Prepairing for C++ integration.
//
// Revision 1.3  1998/01/22 02:59:43  curt
// Changed #ifdef FILE_H to #ifdef _FILE_H
//
// Revision 1.2  1998/01/19 18:40:39  curt
// Tons of little changes to clean up the code and to remove fatal errors
// when building with the c++ compiler.
//
// Revision 1.1  1997/12/30 04:19:22  curt
// Initial revision.

