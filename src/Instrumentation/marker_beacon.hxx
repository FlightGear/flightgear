// marker_beacon.hxx -- class to manage the marker beacons
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - http://www.flightgear.org/~curt
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _FG_MARKER_BEACON_HXX
#define _FG_MARKER_BEACON_HXX


#include <Main/fg_props.hxx>

#include <simgear/compiler.h>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/math/interpolater.hxx>
#include <simgear/timing/timestamp.hxx>

class SGSampleGroup;

class FGMarkerBeacon : public SGSubsystem
{
    SGInterpTable *term_tbl;
    SGInterpTable *low_tbl;
    SGInterpTable *high_tbl;

    // Inputs
    SGPropertyNode_ptr lon_node;
    SGPropertyNode_ptr lat_node;
    SGPropertyNode_ptr alt_node;
    SGPropertyNode_ptr bus_power;
    SGPropertyNode_ptr power_btn;
    SGPropertyNode_ptr audio_btn;
    SGPropertyNode_ptr audio_vol;
    SGPropertyNode_ptr serviceable;
    SGPropertyNode_ptr sound_working;

    bool need_update;

    bool outer_marker;
    bool middle_marker;
    bool inner_marker;

    SGTimeStamp blink;
    bool outer_blink;
    bool middle_blink;
    bool inner_blink;

    string name;
    int num;

    // internal periodic station search timer
    double _time_before_search_sec;

    SGSharedPtr<SGSampleGroup> _sgr;

public:

    enum fgMkrBeacType {
        NOBEACON = 0,
        INNER,
        MIDDLE,
        OUTER
    };

    FGMarkerBeacon(SGPropertyNode *node);
    ~FGMarkerBeacon();

    void init ();
    void bind ();
    void unbind ();
    void update (double dt);

    void search ();

    // Marker Beacon Accessors
    inline bool get_inner_blink () const { return inner_blink; }
    inline bool get_middle_blink () const { return middle_blink; }
    inline bool get_outer_blink () const { return outer_blink; }
    inline bool has_power() const {
        return power_btn->getBoolValue() && (bus_power->getDoubleValue() > 1.0);
    }
};


#endif // _FG_MARKER_BEACON_HXX
