// clock.cxx - an electric-powered turn indicator.
// Written by Melchior FRANZ, started 2003.
//
// This file is in the Public Domain and comes with no warranty.
//
// $Id$


#include "clock.hxx"
#include <simgear/timing/sg_time.hxx>
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


Clock::Clock ()
    : _is_serviceable(true),
      _gmt_time_sec(0),
      _offset_sec(0),
      _indicated_sec(0),
      _standstill_offset(0)
{
    _indicated_string[0] = '\0';
}

Clock::~Clock ()
{
}

void
Clock::init ()
{
    _serviceable_node = fgGetNode("/instrumentation/clock/serviceable", true);
    _offset_node      = fgGetNode("/instrumentation/clock/offset-sec", true);
    _sec_node         = fgGetNode("/instrumentation/clock/indicated-sec", true);
    _string_node      = fgGetNode("/instrumentation/clock/indicated-string", true);
}

void
Clock::update (double delta_time_sec)
{
    if (!_serviceable_node->getBoolValue()) {
        if (_is_serviceable) {
            _string_node->setStringValue("");
            _is_serviceable = false;
        }
        return;
    }

    struct tm *t = globals->get_time_params()->getGmt();
    int hour = t->tm_hour;
    int min = t->tm_min;
    int sec = t->tm_sec;

    long gmt = (hour * 60 + min) * 60 + sec;
    int offset = _offset_node->getLongValue();

    if (!_is_serviceable) {
        _standstill_offset -= gmt - _gmt_time_sec;
    } else if (_gmt_time_sec == gmt && _offset_sec == offset)
        return;

    _gmt_time_sec = gmt;
    _offset_sec = offset;

    _indicated_sec = _gmt_time_sec + offset + _standstill_offset;
    _sec_node->setLongValue(_indicated_sec);

    sec += offset;
    while (sec < 0) {
        sec += 60;
        min--;
    }
    while (sec >= 60) {
        sec -= 60;
        min++;
    }
    while (min < 0) {
        min += 60;
        hour--;
    }
    while (min >= 60) {
        min -= 60;
        hour++;
    }
    while (hour < 0)
        hour += 24;
    while (hour >= 24)
        hour -= 24;

    sprintf(_indicated_string, "%02d:%02d:%02d", hour, min, sec);
    _string_node->setStringValue(_indicated_string);
    _is_serviceable = true;
}


// end of clock.cxx
