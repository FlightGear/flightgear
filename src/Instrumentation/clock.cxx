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


Clock::Clock ( SGPropertyNode *node )
    : _is_serviceable(true),
      _gmt_time_sec(0),
      _offset_sec(0),
      _indicated_sec(0),
      _standstill_offset(0),
      name("clock"),
      num(0)
{
    _indicated_string[0] = '\0';

    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        string cname = child->getName();
        string cval = child->getStringValue();
        if ( cname == "name" ) {
            name = cval;
        } else if ( cname == "number" ) {
            num = child->getIntValue();
        } else {
            SG_LOG( SG_AUTOPILOT, SG_WARN, "Error in clock config logic" );
            if ( name.length() ) {
                SG_LOG( SG_AUTOPILOT, SG_WARN, "Section = " << name );
            }
        }
    }
}

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
    string branch;
    branch = "/instrumentation/" + name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );
    _serviceable_node = node->getChild("serviceable", 0, true);
    _offset_node = node->getChild("offset-sec", 0, true);
    _sec_node = node->getChild("indicated-sec", 0, true);
    _string_node = node->getChild("indicated-string", 0, true);

    _serviceable_node->setBoolValue(true);
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
