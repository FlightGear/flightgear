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


#if defined ( __sun__ ) || defined ( __sgi )
extern "C" void *memmove(void *, const void *, size_t);
extern "C" void *memset(void *, int, size_t);
#endif


#include <deque>        // STL double ended queue
#include <list>         // STL list
#include <string>

#include "Include/fg_stl_config.h"

#ifdef _FG_NEED_AUTO_PTR
#  include "Include/auto_ptr.hxx"
#else
#  include <memory>
#endif

#include "Include/fg_callback.hxx"

#ifdef NEEDNAMESPACESTD
using namespace std;
#endif

#include "fg_time.hxx"


class fgEVENT
{
public:
    enum EventState
    {
	FG_EVENT_SUSP   = 0,
	FG_EVENT_READY  = 1,
	FG_EVENT_QUEUED = 2
    };

    fgEVENT(); // Required by deque<>.

    fgEVENT( const string& desc,
	     const fgCallback& cb,
	     EventState _status,
	     int _interval );

    fgEVENT( const fgEVENT& evt );

    ~fgEVENT();

    void run();

    void PrintStats() const;

public:

    string description;

    // The callback object.
    // We wrap it in an auto_ptr<> because deque<> calls our copy ctor
    // and dtor when inserting and removing.
    auto_ptr<fgCallback> event_cb;

    EventState status;       // status flag

    long interval;    // interval in ms between each iteration of this event

    fg_timestamp last_run;
    fg_timestamp current;
    fg_timestamp next_run;

    long cum_time;    // cumulative processor time of this event
    long min_time;    // time of quickest execution
    long max_time;    // time of slowest execution
    long count;       // number of times executed
};


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


// $Log$
// Revision 1.10  1998/09/08 21:41:06  curt
// Added constructor for fgEVENT.
//
// Revision 1.9  1998/09/02 14:37:45  curt
// Renamed struct -> class.
//
// Revision 1.8  1998/08/29 13:11:32  curt
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
// Revision 1.7  1998/07/30 23:48:54  curt
// Sgi build tweaks.
// Pause support.
//
// Revision 1.6  1998/07/24 21:42:25  curt
// Output message tweaks.
//
// Revision 1.5  1998/07/13 21:02:07  curt
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.4  1998/06/12 00:59:52  curt
// Build only static libraries.
// Declare memmove/memset for Sloaris.
// Rewrote fg_time.c routine to get LST start seconds to better handle
//   Solaris, and be easier to port, and understand the GMT vs. local
//   timezone issues.
//
// Revision 1.3  1998/06/03 00:48:12  curt
// No .h for STL includes.
//
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

