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
		 interval_type repeat_value,
		 interval_type initial_value );

	/**
	 * 
	 */
	~FGEvent();

	/**
	 * 
	 */
	void reset()
	{
	    ms_to_go_ = repeat_value_;
	}

	/**
	 * Execute this event's callback.
	 */
	void run();

	string name() const { return name_; }
	interval_type repeat_value() const { return repeat_value_; }
	int value() const { return ms_to_go_; }

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
	    ms_to_go_ -= dt_ms;
	    return ms_to_go_ <= 0;
	}

    private:
	string name_;
	fgCallback* callback_;
	interval_type repeat_value_;
	interval_type initial_value_;
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
     * @param dt elapsed time in seconds.
     */
    void update( double dt );

    /**
     * Register a free standing function to be executed some time in the future.
     * @param desc A brief description of this callback for logging.
     * @param cb The callback function to be executed.
     * @param repeat_value repetition rate in milliseconds.
     * @param initial_value initial delay value in milliseconds.  A value of
     * -1 means run immediately.
     */
    template< typename Fun >
    void Register( const char* name,
		   const Fun& f,
		   interval_type repeat_value,
		   interval_type initial_value = -1 )
    {
	this->Register( FGEvent( name,
				 make_callback(f),
				 repeat_value,
				 initial_value ) );
    }

    template< class ObjPtr, typename MemFn >
    void Register( const char* name,
		   const ObjPtr& p,
		   MemFn pmf,
		   interval_type repeat_value,
		   interval_type initial_value = -1 )
    {
	this->Register( FGEvent( name,
				 make_callback(p,pmf),
				 repeat_value,
				 initial_value ) );
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
