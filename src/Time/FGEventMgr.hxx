// FGEventMgr.hxx -- Flight Gear periodic event scheduler
//
// Written by Curtis Olson, started December 1997.
// Modified by Bernie Bright, April 2002.
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

#ifndef FG_EVENT_MGR_H_INCLUDED
#define FG_EVENT_MGR_H_INCLUDED 1

#include <simgear/compiler.h>
#include <simgear/timing/timestamp.hxx>

#include <Main/fgfs.hxx>
#include <Include/fg_callback.hxx>

#include <vector>
#include <string>

SG_USING_STD(vector);
SG_USING_STD(string);

/**
 * 
 */
class FGEventMgr : public FGSubsystem
{
public:

    /**
     * 
     */
    enum EventState {
	FG_EVENT_SUSP   = 0,
	FG_EVENT_READY  = 1,
	FG_EVENT_QUEUED = 2
    };

    typedef int interval_type;

private:

    /**
     * 
     */
    class FGEvent
    {
    public:
	/**
	 * 
	 */
	FGEvent();

	FGEvent( const char* desc,
		 fgCallback* cb,
		 EventState status,
		 interval_type ms );

	/**
	 * 
	 */
	~FGEvent();

	/**
	 * 
	 */
	void reset()
	{
	    status_ = FG_EVENT_READY;
	    ms_to_go_ = ms_;
	}

	/**
	 * Execute this event's callback.
	 */
	void run();

	bool is_ready() const { return status_ == FG_EVENT_READY; }

	string name() const { return name_; }
	interval_type interval() const { return ms_; }

	/**
	 * Display event statistics.
	 */
	void print_stats() const;

	/**
	 * Update the elapsed time for this event.
	 * @param dt_ms elapsed time in milliseconds.
	 * @return true if elapsed time has expired.
	 */
	bool update( int dt_ms )
	{
	    if (status_ != FG_EVENT_READY)
		return false;

	    ms_to_go_ -= dt_ms;
	    return ms_to_go_ <= 0;
	}

    private:
	string name_;
	fgCallback* callback_;
	EventState status_;
	interval_type ms_;
	int ms_to_go_;

	unsigned long cum_time;    // cumulative processor time of this event
	unsigned long min_time;    // time of quickest execution
	unsigned long max_time;    // time of slowest execution
	unsigned long count;       // number of times executed
    };

public:
    FGEventMgr();
    ~FGEventMgr();

    /**
     * Initialize the scheduling subsystem.
     */
    void init();

    void bind();

    void unbind();

    /*
     * Update the elapsed time for all events.
     * @param dt elapsed time in milliseconds.
     */
    void update( int dt );

    /**
     * Register a function to be executed every 'interval' milliseconds.
     * @param desc A brief description of this callback for logging.
     * @param cb The callback function to be executed.
     * @param state
     * @param interval Callback repetition rate in milliseconds.
     */
    void Register( const char* name,
		   void (*cb)(),
		   interval_type interval_ms,
		   EventState state = FG_EVENT_READY )
    {
	this->Register( FGEvent( name, new fgFunctionCallback(cb), state, interval_ms ) );
    }

    template< class Obj >
    void Register( const char* name,
		   Obj* obj, void (Obj::*pmf)() const,
		   interval_type interval_ms,
		   EventState state = FG_EVENT_READY )
    {
	this->Register( FGEvent( name, new fgMethodCallback<Obj>(obj,pmf), state, interval_ms ) );
    }

    template< class Obj >
    void Register( const char* name,
		   Obj* obj, void (Obj::*pmf)(),
		   interval_type interval_ms,
		   EventState state = FG_EVENT_READY )
    {
	this->Register( FGEvent( name, new fgMethodCallback<Obj>(obj,pmf), state, interval_ms ) );
    }

    /**
     * Display statistics for all registered events.
     */
    void print_stats() const;

private:
    void Register( const FGEvent& event );

private:

    typedef vector< FGEvent > event_container_type;

    // Registered events.
    event_container_type event_table;
};

extern FGEventMgr global_events;

#endif //FG_ EVENT_MGR_H_INCLUDED
