#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
// #include <functional>
// #include <algorithm>

#include "FGEventMgr.hxx"

FGEventMgr::FGEvent::FGEvent()
    : name_(""),
      callback_(0),
      status_(FG_EVENT_SUSP),
      ms_(0),
      ms_to_go_(0),
      cum_time(0),
      min_time(100000),
      max_time(0),
      count(0)
{
}

FGEventMgr::FGEvent::FGEvent( const char* name,
			      fgCallback* cb,
			      EventState status,
			      interval_type ms )
    : name_(name),
      callback_(cb),
      status_(status),
      ms_(ms),
      ms_to_go_(ms),
      cum_time(0),
      min_time(100000),
      max_time(0),
      count(0)
{
    if (status == FG_EVENT_READY)
	this->run();
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
    callback_->call(0);

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
            "  " << name_
            << " int=" << ms_ / 1000.0
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
FGEventMgr::Register( const FGEvent& event )
{
    event_table.push_back( event );

    SG_LOG( SG_EVENT, SG_INFO, "Registered event " << event.name()
	    << " to run every " << event.interval() << "ms" );
}

void
FGEventMgr::print_stats() const
{
    SG_LOG( SG_EVENT, SG_INFO, "" );
    SG_LOG( SG_EVENT, SG_INFO, "Event Stats" );
    SG_LOG( SG_EVENT, SG_INFO, "-----------" );

#if 1
    event_container_type::const_iterator first = event_table.begin();
    event_container_type::const_iterator last = event_table.end();
    for (; first != last; ++first)
    {
	first->print_stats();
    }
#else
    // #@!$ MSVC can't handle const member functions.
    std::for_each( event_table.begin(), event_table.end(),
        std::mem_fun_ref( &FGEvent::print_stats ) );
#endif
    SG_LOG( SG_EVENT, SG_INFO, "" );
}
