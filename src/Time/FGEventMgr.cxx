//
// FGEventMgr.cxx -- Event Manager
//
// Written by Bernie Bright, started April 2002.
//
// Copyright (C) 2002  Curtis L. Olson  - curt@me.umn.edu
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
#include <simgear/debug/logstream.hxx>

#include "FGEventMgr.hxx"

FGEventMgr::FGEvent::FGEvent()
    : name_(""),
      callback_(0),
      repeat_value_(0),
      initial_value_(0),
      ms_to_go_(0),
      cum_time(0),
      min_time(100000),
      max_time(0),
      count(0)
{
}

FGEventMgr::FGEvent::FGEvent( const char* name,
			      fgCallback* cb,
			      interval_type repeat_value,
			      interval_type initial_value )
    : name_(name),
      callback_(cb),
      repeat_value_(repeat_value),
      initial_value_(initial_value),
      //ms_to_go_(repeat_value_),
      cum_time(0),
      min_time(100000),
      max_time(0),
      count(0)
{
    if (initial_value_ < 0)
    {
	this->run();
	ms_to_go_ = repeat_value_;
    }
    else
    {
	ms_to_go_ = initial_value_;
    }
}


FGEventMgr::FGEvent::~FGEvent()
{
    //delete callback_;
}

void
FGEventMgr::FGEvent::run()
{
    SGTimeStamp start_time;
    SGTimeStamp finish_time;

    start_time.stamp();

    // run the event
    (*callback_)();

    finish_time.stamp();

    ++count;

    unsigned long duration = finish_time - start_time;

    cum_time += duration;

    if ( duration < min_time ) {
	min_time = duration;
    }

    if ( duration > max_time ) {
	max_time = duration;
    }
}

void
FGEventMgr::FGEvent::print_stats() const
{
    SG_LOG( SG_EVENT, SG_INFO, 
            "  " << name_
            << " int=" << repeat_value_ / 1000.0
            << " cum=" << cum_time
            << " min=" << min_time
            << " max=" <<  max_time
            << " count=" << count
            << " ave=" << cum_time / (double)count );
}

FGEventMgr::FGEventMgr()
{
}

FGEventMgr::~FGEventMgr()
{
}

void
FGEventMgr::init()
{
    SG_LOG( SG_EVENT, SG_INFO, "Initializing event manager" );

    event_table.clear();
}

void
FGEventMgr::bind()
{
}

void
FGEventMgr::unbind()
{
}

void
FGEventMgr::update( int dt )
{
    if (dt < 0)
    {
	SG_LOG( SG_GENERAL, SG_ALERT,
		"FGEventMgr::update() called with negative delta T" );
	return;
    }

    int min_value = 0;
    event_container_type::iterator first = event_table.begin();
    event_container_type::iterator last = event_table.end();
    event_container_type::iterator event = event_table.end();

    // Scan all events.  Run one whose interval has expired.
    while (first != last)
    {
	if (first->update( dt ))
	{
	    if (first->value() < min_value)
	    {
		// Select event with largest negative value.
		// Its been waiting longest.
		min_value = first->value();
		event = first;
	    }
	}
	++first;
    }

    if (event != last)
    {
	event->run();

	if (event->repeat_value() > 0)
	{
	    event->reset();
	}
	else
	{
	    SG_LOG( SG_GENERAL, SG_DEBUG, "Deleting event " << event->name() );
	    event_table.erase( event );
	}
    }
}

void
FGEventMgr::Register( const FGEvent& event )
{
    event_table.push_back( event );

    SG_LOG( SG_EVENT, SG_INFO, "Registered event " << event.name()
	    << " to run every " << event.repeat_value() << "ms" );
}

void
FGEventMgr::print_stats() const
{
    SG_LOG( SG_EVENT, SG_INFO, "" );
    SG_LOG( SG_EVENT, SG_INFO, "Event Stats" );
    SG_LOG( SG_EVENT, SG_INFO, "-----------" );

    event_container_type::const_iterator first = event_table.begin();
    event_container_type::const_iterator last = event_table.end();
    for (; first != last; ++first)
    {
	first->print_stats();
    }

    SG_LOG( SG_EVENT, SG_INFO, "" );
}
