// marker_beacon.cxx -- class to manage the marker beacons
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - curt@flightgear.org
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
#include <Navaids/mkrbeacons.hxx>
#include <Time/FGEventMgr.hxx>

#include "marker_beacon.hxx"

#include <string>
SG_USING_STD(string);


/**
 * Boy, this is ugly!  Make the VOR range vary by altitude difference.
 */
static double kludgeRange ( double stationElev, double aircraftElev,
			    double nominalRange)
{
				// Assume that the nominal range (usually
				// 50nm) applies at a 5,000 ft difference.
				// Just a wild guess!
  double factor = ((aircraftElev*SG_METER_TO_FEET) - stationElev) / 5000.0;
  double range = fabs(nominalRange * factor);

				// Clamp the range to keep it sane; for
				// now, never less than 25% or more than
				// 500% of nominal range.
  if (range < nominalRange/4.0) {
    range = nominalRange/4.0;
  } else if (range > nominalRange*5.0) {
    range = nominalRange*5.0;
  }

  return range;
}


// Constructor
FGMarkerBeacon::FGMarkerBeacon() :
    lon_node(fgGetNode("/position/longitude-deg", true)),
    lat_node(fgGetNode("/position/latitude-deg", true)),
    alt_node(fgGetNode("/position/altitude-ft", true)),
    need_update(true),
    outer_blink(false),
    middle_blink(false),
    inner_blink(false)
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
    morse.init();
    beacon.init();
    blink.stamp();
}


void
FGMarkerBeacon::bind ()
{

    fgTie("/radios/marker-beacon/inner", this,
	  &FGMarkerBeacon::get_inner_blink);

    fgTie("/radios/marker-beacon/middle", this,
	  &FGMarkerBeacon::get_middle_blink);

    fgTie("/radios/marker-beacon/outer", this,
	  &FGMarkerBeacon::get_outer_blink);
}


void
FGMarkerBeacon::unbind ()
{
    fgUntie("/radios/marker-beacon/inner");
    fgUntie("/radios/marker-beacon/middle");
    fgUntie("/radios/marker-beacon/outer");
}


// Update the various nav values based on position and valid tuned in navs
void 
FGMarkerBeacon::update(double dt) 
{
    need_update = false;

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

    // cout << outer_blink << " " << middle_blink << " " << inner_blink << endl;
}


// Update current nav/adf radio stations based on current postition
void FGMarkerBeacon::search() 
{
    static FGMkrBeacon::fgMkrBeacType last_beacon = FGMkrBeacon::NOBEACON;

    double lon = lon_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double lat = lat_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double elev = alt_node->getDoubleValue() * SG_FEET_TO_METER;

    ////////////////////////////////////////////////////////////////////////
    // Beacons.
    ////////////////////////////////////////////////////////////////////////

    FGMkrBeacon::fgMkrBeacType beacon_type
	= current_beacons->query( lon * SGD_RADIANS_TO_DEGREES,
				  lat * SGD_RADIANS_TO_DEGREES, elev );

    outer_marker = middle_marker = inner_marker = false;

    if ( beacon_type == FGMkrBeacon::OUTER ) {
	outer_marker = true;
	// cout << "OUTER MARKER" << endl;
#ifdef ENABLE_AUDIO_SUPPORT
	if ( last_beacon != FGMkrBeacon::OUTER ) {
	    if ( ! globals->get_soundmgr()->exists( "outer-marker" ) ) {
		FGSimpleSound *sound = beacon.get_outer();
		sound->set_volume( 0.3 );
		globals->get_soundmgr()->add( sound, "outer-marker" );
	    }
	    if ( !globals->get_soundmgr()->is_playing("outer-marker") ) {
		globals->get_soundmgr()->play_looped( "outer-marker" );
	    }
	}
#endif
    } else if ( beacon_type == FGMkrBeacon::MIDDLE ) {
	middle_marker = true;
	// cout << "MIDDLE MARKER" << endl;
#ifdef ENABLE_AUDIO_SUPPORT
	if ( last_beacon != FGMkrBeacon::MIDDLE ) {
	    if ( ! globals->get_soundmgr()->exists( "middle-marker" ) ) {
		FGSimpleSound *sound = beacon.get_middle();
		sound->set_volume( 0.3 );
		globals->get_soundmgr()->add( sound, "middle-marker" );
	    }
	    if ( !globals->get_soundmgr()->is_playing("middle-marker") ) {
		globals->get_soundmgr()->play_looped( "middle-marker" );
	    }
	}
#endif
    } else if ( beacon_type == FGMkrBeacon::INNER ) {
	inner_marker = true;
	// cout << "INNER MARKER" << endl;
#ifdef ENABLE_AUDIO_SUPPORT
	if ( last_beacon != FGMkrBeacon::INNER ) {
	    if ( ! globals->get_soundmgr()->exists( "inner-marker" ) ) {
		FGSimpleSound *sound = beacon.get_inner();
		sound->set_volume( 0.3 );
		globals->get_soundmgr()->add( sound, "inner-marker" );
	    }
	    if ( !globals->get_soundmgr()->is_playing("inner-marker") ) {
		globals->get_soundmgr()->play_looped( "inner-marker" );
	    }
	}
#endif
    } else {
	// cout << "no marker" << endl;
#ifdef ENABLE_AUDIO_SUPPORT
	globals->get_soundmgr()->stop( "outer-marker" );
	globals->get_soundmgr()->stop( "middle-marker" );
	globals->get_soundmgr()->stop( "inner-marker" );
#endif
    }
    last_beacon = beacon_type;
}
