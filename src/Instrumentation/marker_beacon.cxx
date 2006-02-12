// marker_beacon.cxx -- class to manage the marker beacons
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>	// snprintf

#include <simgear/compiler.h>
#include <simgear/math/sg_random.h>

#include <Aircraft/aircraft.hxx>
#include <Navaids/navlist.hxx>

#include "marker_beacon.hxx"

#include <string>
SG_USING_STD(string);


// Constructor
FGMarkerBeacon::FGMarkerBeacon(SGPropertyNode *node) :
    need_update(true),
    outer_blink(false),
    middle_blink(false),
    inner_blink(false),
    name("marker-beacon"),
    num(0),
    _time_before_search_sec(0.0)
{
    SGPath path( globals->get_fg_root() );
    SGPath term = path;
    term.append( "Navaids/range.term" );
    SGPath low = path;
    low.append( "Navaids/range.low" );
    SGPath high = path;
    high.append( "Navaids/range.high" );

    term_tbl = new SGInterpTable( term.str() );
    low_tbl = new SGInterpTable( low.str() );
    high_tbl = new SGInterpTable( high.str() );

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
            SG_LOG( SG_INSTR, SG_WARN, 
                    "Error in marker beacon config logic" );
            if ( name.length() ) {
                SG_LOG( SG_INSTR, SG_WARN, "Section = " << name );
            }
        }
    }
}


// Destructor
FGMarkerBeacon::~FGMarkerBeacon() 
{
    delete term_tbl;
    delete low_tbl;
    delete high_tbl;
}


void
FGMarkerBeacon::init ()
{
    string branch;
    branch = "/instrumentation/" + name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );
    // Inputs
    sound_pause = fgGetNode("/sim/sound/pause", false);
    lon_node = fgGetNode("/position/longitude-deg", true);
    lat_node = fgGetNode("/position/latitude-deg", true);
    alt_node = fgGetNode("/position/altitude-ft", true);
    bus_power = fgGetNode("/systems/electrical/outputs/nav[0]", true);
    power_btn = node->getChild("power-btn", 0, true);
    audio_btn = node->getChild("audio-btn", 0, true);
    serviceable = node->getChild("serviceable", 0, true);

    power_btn->setBoolValue( true );
    audio_btn->setBoolValue( true );
    serviceable->setBoolValue( true );

    morse.init();
    beacon.init();
    blink.stamp();
}


void
FGMarkerBeacon::bind ()
{
    string branch;
    branch = "/instrumentation/" + name;

    fgTie((branch + "/inner").c_str(), this,
	  &FGMarkerBeacon::get_inner_blink);

    fgTie((branch + "/middle").c_str(), this,
	  &FGMarkerBeacon::get_middle_blink);

    fgTie((branch + "/outer").c_str(), this,
	  &FGMarkerBeacon::get_outer_blink);
}


void
FGMarkerBeacon::unbind ()
{
    string branch;
    branch = "/instrumentation/" + name;

    fgUntie((branch + "/inner").c_str());
    fgUntie((branch + "/middle").c_str());
    fgUntie((branch + "/outer").c_str());
}


// Update the various nav values based on position and valid tuned in navs
void 
FGMarkerBeacon::update(double dt) 
{
    need_update = false;

    if ( has_power() && serviceable->getBoolValue()
            && !sound_pause->getBoolValue()) {

	// On timeout, scan again
	_time_before_search_sec -= dt;
	if ( _time_before_search_sec < 0 ) {
	    search();
	}
        // marker beacon blinking
        bool light_on = ( outer_blink || middle_blink || inner_blink );
        SGTimeStamp current;
        current.stamp();

        if ( light_on && (current - blink > 400000) ) {
            light_on = false;
            blink.stamp();
        } else if ( !light_on && (current - blink > 100000) ) {
            light_on = true;
            blink.stamp();
        }

        if ( outer_marker ) {
            outer_blink = light_on;
        } else {
            outer_blink = false;
        }

        if ( middle_marker ) {
            middle_blink = light_on;
        } else {
            middle_blink = false;
        }

        if ( inner_marker ) {
            inner_blink = light_on;
        } else {
            inner_blink = false;
        }

        // cout << outer_blink << " " << middle_blink << " "
        //      << inner_blink << endl;
    } else {
        inner_blink = middle_blink = outer_blink = false;
    }
}


static bool check_beacon_range( double lon_rad, double lat_rad, double elev_m,
                                FGNavRecord *b )
{
    Point3D aircraft = sgGeodToCart( Point3D(lon_rad, lat_rad, elev_m) );
    Point3D station = Point3D( b->get_x(), b->get_y(), b->get_z() );
    // cout << "    aircraft = " << aircraft << " station = " << station 
    //      << endl;

    double d = aircraft.distance3Dsquared( station ); // meters^2
    // cout << "  distance = " << d << " (" 
    //      << FG_ILS_DEFAULT_RANGE * SG_NM_TO_METER 
    //         * FG_ILS_DEFAULT_RANGE * SG_NM_TO_METER
    //      << ")" << endl;
    
    // cout << "  range = " << sqrt(d) << endl;

    // cout << "elev = " << elev * SG_METER_TO_FEET
    //      << " current->get_elev() = " << current->get_elev() << endl;
    double elev_ft = elev_m * SG_METER_TO_FEET;
    double delev = elev_ft - b->get_elev_ft();

    // max range is the area under r = 2.4 * alt or r^2 = 4000^2 - alt^2
    // whichever is smaller.  The intersection point is 1538 ...
    double maxrange2;	// feet^2
    if ( delev < 1538.0 ) {
        maxrange2 = 2.4 * 2.4 * delev * delev;
    } else if ( delev < 4000.0 ) {
        maxrange2 = 4000 * 4000 - delev * delev;
    } else {
        maxrange2 = 0.0;
    }
    maxrange2 *= SG_FEET_TO_METER * SG_FEET_TO_METER; // convert to meter^2
    // cout << "delev = " << delev << " maxrange = " << maxrange << endl;

    // match up to twice the published range so we can model
    // reduced signal strength
    if ( d < maxrange2 ) {
        return true;
    } else {
        return false;
    }
}

// Update current nav/adf radio stations based on current postition
void FGMarkerBeacon::search() 
{

    // reset search time
    _time_before_search_sec = 1.0;

    static fgMkrBeacType last_beacon = NOBEACON;

    double lon_rad = lon_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double lat_rad = lat_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double elev_m = alt_node->getDoubleValue() * SG_FEET_TO_METER;

    ////////////////////////////////////////////////////////////////////////
    // Beacons.
    ////////////////////////////////////////////////////////////////////////

    // get closest marker beacon
    FGNavRecord *b
	= globals->get_mkrlist()->findClosest( lon_rad, lat_rad, elev_m );

    // cout << "marker beacon = " << b << " (" << b->get_type() << ")" << endl;

    fgMkrBeacType beacon_type = NOBEACON;
    bool inrange = false;
    if ( b != NULL ) {
        if ( b->get_type() == 7 ) {
            beacon_type = OUTER;
        } else if ( b->get_type() == 8 ) {
            beacon_type = MIDDLE;
        } else if ( b->get_type() == 9 ) {
            beacon_type = INNER;
        }
        inrange = check_beacon_range( lon_rad, lat_rad, elev_m, b );
        // cout << "  inrange = " << inrange << endl;
    }

    outer_marker = middle_marker = inner_marker = false;

    if ( b == NULL || !inrange || !has_power() || !serviceable->getBoolValue() )
    {
	// cout << "no marker" << endl;
	globals->get_soundmgr()->stop( "outer-marker" );
	globals->get_soundmgr()->stop( "middle-marker" );
	globals->get_soundmgr()->stop( "inner-marker" );
    } else if ( beacon_type == OUTER ) {
	outer_marker = true;
	// cout << "OUTER MARKER" << endl;
        if ( last_beacon != OUTER ) {
            if ( ! globals->get_soundmgr()->exists( "outer-marker" ) ) {
                SGSoundSample *sound = beacon.get_outer();
                if ( sound ) {
                    sound->set_volume( 0.3 );
                    globals->get_soundmgr()->add( sound, "outer-marker" );
                }
            }
        }
        if ( audio_btn->getBoolValue() ) {
            if ( !globals->get_soundmgr()->is_playing("outer-marker") ) {
                globals->get_soundmgr()->play_looped( "outer-marker" );
            }
        } else {
            globals->get_soundmgr()->stop( "outer-marker" );
        }
    } else if ( beacon_type == MIDDLE ) {
	middle_marker = true;
	// cout << "MIDDLE MARKER" << endl;
	if ( last_beacon != MIDDLE ) {
	    if ( ! globals->get_soundmgr()->exists( "middle-marker" ) ) {
		SGSoundSample *sound = beacon.get_middle();
                if ( sound ) {
		    sound->set_volume( 0.3 );
		    globals->get_soundmgr()->add( sound, "middle-marker" );
                }
	    }
        }
        if ( audio_btn->getBoolValue() ) {
            if ( !globals->get_soundmgr()->is_playing("middle-marker") ) {
                globals->get_soundmgr()->play_looped( "middle-marker" );
            }
        } else {
            globals->get_soundmgr()->stop( "middle-marker" );
        }
    } else if ( beacon_type == INNER ) {
	inner_marker = true;
	// cout << "INNER MARKER" << endl;
	if ( last_beacon != INNER ) {
	    if ( ! globals->get_soundmgr()->exists( "inner-marker" ) ) {
		SGSoundSample *sound = beacon.get_inner();
                if ( sound ) {
		    sound->set_volume( 0.3 );
		    globals->get_soundmgr()->add( sound, "inner-marker" );
                }
	    }
        }
        if ( audio_btn->getBoolValue() ) {
            if ( !globals->get_soundmgr()->is_playing("inner-marker") ) {
                globals->get_soundmgr()->play_looped( "inner-marker" );
            }
        } else {
            globals->get_soundmgr()->stop( "inner-marker" );
        }
    }

    if ( inrange ) {
        last_beacon = beacon_type;
    } else {
        last_beacon = NOBEACON;
    }
}
