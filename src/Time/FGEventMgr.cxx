#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <functional>
#include <boost/bind.hpp>

#include "FGEventMgr.hxx"

FGEventMgr::FGEvent::FGEvent()
    : status(FG_EVENT_SUSP),
      interval_ms(0),
      ms_to_go(0),
      cum_time(0),
      min_time(100000),
      max_time(0),
      count(0)
{
}

FGEventMgr::FGEvent::FGEvent( const string& desc,
			      callback_type cb,
			      EventState status_,
			      interval_type ms )
    : description(desc),
      callback(cb),
      status(status_),
      interval_ms(ms),
      ms_to_go(ms),
      cum_time(0),
      min_time(100000),
      max_time(0),
      count(0)
{
}


FGEventMgr::FGEvent::~FGEvent()
{
}

void
FGEventMgr::FGEvent::run()
{
    SGTimeStamp start_time;
    SGTimeStamp finish_time;

    start_time.stamp();

    // run the event
    this->callback();

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

    reset();
}

void
FGEventMgr::FGEvent::print_stats() const
{
    SG_LOG( SG_EVENT, SG_INFO, 
            "  " << description 
            << " int=" << interval_ms / 1000.0
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
    // Scan all events.  Execute any whose interval has expired.
    event_container_type::iterator first = event_table.begin();
    event_container_type::iterator last = event_table.end();
    for(; first != last; ++first)
    {
	if (first->update( dt ))
	{
	    first->run();
	}
    }
}

void
FGEventMgr::Register( const string& desc,
		      callback_type cb,
		      EventState state,
		      interval_type ms )
{
    FGEvent event( desc, cb, state, ms );
    if (state == FG_EVENT_READY)
	event.run();
    event_table.push_back( event );

    SG_LOG( SG_EVENT, SG_INFO, "Registered event " << desc
	    << " to run every " << ms << "ms" );
}

void
FGEventMgr::print_stats() const
{
    SG_LOG( SG_EVENT, SG_INFO, "" );
    SG_LOG( SG_EVENT, SG_INFO, "Event Stats" );
    SG_LOG( SG_EVENT, SG_INFO, "-----------" );

    std::for_each( event_table.begin(), event_table.end(),
		   boost::bind( &FGEvent::print_stats, _1 ) );

    SG_LOG( SG_EVENT, SG_INFO, "" );
}
