// clock.cxx - an electric-powered turn indicator.
// Written by Melchior FRANZ, started 2003.
//
// This file is in the Public Domain and comes with no warranty.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "clock.hxx"
#include <simgear/timing/sg_time.hxx>
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


Clock::Clock(SGPropertyNode *node) :
    _name(node->getStringValue("name", "clock")),
    _num(node->getIntValue("number", 0)),
    _is_serviceable(true),
    _gmt_time_sec(0),
    _offset_sec(0),
    _indicated_sec(0),
    _indicated_min(0),
    _indicated_hour(0),
    _local_hour(0),
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
    std::string branch;
    branch = "/instrumentation/" + _name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), _num, true );
    _serviceable_node = node->getChild("serviceable", 0, true);
    _offset_node = node->getChild("offset-sec", 0, true);
    _sec_node = node->getChild("indicated-sec", 0, true);
    _min_node = node->getChild("indicated-min", 0, true);
    _hour_node = node->getChild("indicated-hour", 0, true);
    _lhour_node = node->getChild("local-hour", 0, true);
    _string_node = node->getChild("indicated-string", 0, true);
    _string_node1 = node->getChild("indicated-short-string", 0, true);
    _string_node2 = node->getChild("local-short-string", 0, true);
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

    // compute local time zone hour
    int tzoffset_hours = globals->get_time_params()->get_local_offset() / 3600;
    int lhour = hour + tzoffset_hours;
    if (lhour < 0)
        lhour += 24;
    if (lhour >= 24)
        lhour -= 24;

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
    sprintf(_indicated_short_string, "%02d:%02d", hour, min);
    _string_node1->setStringValue(_indicated_short_string);
    sprintf(_local_short_string, "%02d:%02d", lhour, min);
    _string_node2->setStringValue(_local_short_string);
    _is_serviceable = true;

    _indicated_min = min;
    _min_node->setLongValue(_indicated_min);
    _indicated_hour = hour;
    _hour_node->setLongValue(_indicated_hour);
    _local_hour = lhour;
    _lhour_node->setLongValue(_local_hour);
}


// end of clock.cxx
