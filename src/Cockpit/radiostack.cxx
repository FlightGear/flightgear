// radiostack.cxx -- class to manage an instance of the radio stack
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
#include <Navaids/ilslist.hxx>
#include <Navaids/mkrbeacons.hxx>
#include <Navaids/navlist.hxx>
#include <Time/FGEventMgr.hxx>

#include "radiostack.hxx"

#include <string>
SG_USING_STD(string);


FGRadioStack *current_radiostack;


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
FGRadioStack::FGRadioStack() :
    lon_node(fgGetNode("/position/longitude-deg", true)),
    lat_node(fgGetNode("/position/latitude-deg", true)),
    alt_node(fgGetNode("/position/altitude-ft", true)),
    need_update(true),
    dme_freq(0.0),
    dme_dist(0.0),
    dme_prev_dist(0.0),
    dme_spd(0.0),
    dme_ete(0.0),
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
    dme_last_time.stamp();
}


// Destructor
FGRadioStack::~FGRadioStack() 
{
    navcom1.unbind();
    navcom2.unbind();
    adf.unbind();
    xponder.unbind();
    unbind();			// FIXME: should be called externally

    delete term_tbl;
    delete low_tbl;
    delete high_tbl;
}


void
FGRadioStack::init ()
{
    navcom1.set_bind_index( 0 );
    navcom1.init();

    navcom2.set_bind_index( 1 );
    navcom2.init();

    adf.init();
    xponder.init();

    morse.init();
    beacon.init();
    blink.stamp();

    search();
    navcom1.search();
    navcom2.search();
    adf.search();
    xponder.search();

    update(0);			// FIXME: use dt
    navcom1.update(0);
    navcom2.update(0);
    adf.update(0);
    xponder.update(0);

    // Search radio database once per second
    global_events.Register( "fgRadioSearch()",
			    current_radiostack, &FGRadioStack::search,
			    1000 );
}

void
FGRadioStack::bind ()
{

				// User inputs
    fgTie("/radios/dme/frequencies/selected-khz", this,
	  &FGRadioStack::get_dme_freq, &FGRadioStack::set_dme_freq);

				// Radio outputs
    fgTie("/radios/dme/in-range", this, &FGRadioStack::get_dme_inrange);

    fgTie("/radios/dme/distance-nm", this, &FGRadioStack::get_dme_dist);

    fgTie("/radios/dme/speed-kt", this, &FGRadioStack::get_dme_spd);

    fgTie("/radios/dme/ete-min", this, &FGRadioStack::get_dme_ete);

    fgTie("/radios/marker-beacon/inner", this,
	  &FGRadioStack::get_inner_blink);

    fgTie("/radios/marker-beacon/middle", this,
	  &FGRadioStack::get_middle_blink);

    fgTie("/radios/marker-beacon/outer", this,
	  &FGRadioStack::get_outer_blink);

    navcom1.set_bind_index( 0 );
    navcom1.bind();
    navcom2.set_bind_index( 1 );
    navcom2.bind();
    adf.bind();
    xponder.bind();
}

void
FGRadioStack::unbind ()
{
    fgUntie("/radios/dme/frequencies/selected-khz");

				// Radio outputs
    fgUntie("/radios/dme/in-range");
    fgUntie("/radios/dme/distance-nm");
    fgUntie("/radios/dme/speed-kt");
    fgUntie("/radios/dme/ete-min");

    fgUntie("/radios/marker-beacon/inner");
    fgUntie("/radios/marker-beacon/middle");
    fgUntie("/radios/marker-beacon/outer");

    navcom1.unbind();
    navcom2.unbind();
    adf.unbind();
    xponder.unbind();
}


// Update the various nav values based on position and valid tuned in navs
void 
FGRadioStack::update(double dt) 
{
    double lon = lon_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double lat = lat_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double elev = alt_node->getDoubleValue() * SG_FEET_TO_METER;

    need_update = false;

    Point3D aircraft = sgGeodToCart( Point3D( lon, lat, elev ) );
    Point3D station;

    navcom1.update( dt );
    navcom2.update( dt );

    ////////////////////////////////////////////////////////////////////////
    // DME.
    ////////////////////////////////////////////////////////////////////////

    if ( dme_valid ) {
	station = Point3D( dme_x, dme_y, dme_z );
	dme_dist = aircraft.distance3D( station ) * SG_METER_TO_NM;
	dme_effective_range = kludgeRange(dme_elev, elev, dme_range);
	if (dme_dist < dme_effective_range * SG_NM_TO_METER) {
            dme_inrange = true;
	} else if (dme_dist < 2 * dme_effective_range * SG_NM_TO_METER) {
            dme_inrange = sg_random() <
                (2 * dme_effective_range * SG_NM_TO_METER - dme_dist) /
                (dme_effective_range * SG_NM_TO_METER);
	} else {
            dme_inrange = false;
	}
	if ( dme_inrange ) {
            SGTimeStamp current_time;
            station = Point3D( dme_x, dme_y, dme_z );
            dme_dist = aircraft.distance3D( station ) * SG_METER_TO_NM;
            current_time.stamp();
            long dMs = (current_time - dme_last_time) / 1000;
				// Update every second
            if (dMs >= 1000) {
                double dDist = dme_dist - dme_prev_dist;
                dme_spd = fabs((dDist/dMs) * 3600000);
				// FIXME: the panel should be able to
				// handle this!!!
                if (dme_spd > 999.0)
                    dme_spd = 999.0;
                dme_ete = fabs((dme_dist/dme_spd) * 60.0);
				// FIXME: the panel should be able to
				// handle this!!!
                if (dme_ete > 99.0)
                    dme_ete = 99.0;
                dme_prev_dist = dme_dist;
                dme_last_time.stamp();
            }
	}
    } else {
        dme_inrange = false;
        dme_dist = 0.0;
        dme_prev_dist = 0.0;
        dme_spd = 0.0;
        dme_ete = 0.0;
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

    // cout << outer_blink << " " << middle_blink << " " << inner_blink << endl;

    adf.update( dt );
    xponder.update( dt );
}


// Update current nav/adf radio stations based on current postition
void FGRadioStack::search() 
{
    static FGMkrBeacon::fgMkrBeacType last_beacon = FGMkrBeacon::NOBEACON;

    double lon = lon_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double lat = lat_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double elev = alt_node->getDoubleValue() * SG_FEET_TO_METER;

				// FIXME: the panel should handle this
				// don't worry about overhead for now,
				// since this is handled only periodically
    int dme_switch_pos = fgGetInt("/radios/dme/switch-position");
    if ( dme_switch_pos == 1 && navcom1.get_power_btn() ) {
        if ( dme_freq != navcom1.get_nav_freq() ) {
            dme_freq = navcom1.get_nav_freq();
            need_update = true;
        }
    } else if ( dme_switch_pos == 3 && navcom2.get_power_btn() ) {
        if ( dme_freq != navcom2.get_nav_freq() ) {
            dme_freq = navcom2.get_nav_freq();
            need_update = true;
        }
    } else {
        dme_freq = 0;
        dme_inrange = false;
    }

    FGILS ils;
    FGNav nav;

    navcom1.search();
    navcom2.search();

    ////////////////////////////////////////////////////////////////////////
    // DME
    ////////////////////////////////////////////////////////////////////////

    if ( current_ilslist->query( lon, lat, elev, dme_freq, &ils ) ) {
      if (ils.get_has_dme()) {
	dme_valid = true;
	dme_lon = ils.get_loclon();
	dme_lat = ils.get_loclat();
	dme_elev = ils.get_gselev();
	dme_range = FG_ILS_DEFAULT_RANGE;
	dme_effective_range = kludgeRange(dme_elev, elev, dme_range);
	dme_x = ils.get_dme_x();
	dme_y = ils.get_dme_y();
	dme_z = ils.get_dme_z();
      }
    } else if ( current_navlist->query( lon, lat, elev, dme_freq, &nav ) ) {
      if (nav.get_has_dme()) {
	dme_valid = true;
	dme_lon = nav.get_lon();
	dme_lat = nav.get_lat();
	dme_elev = nav.get_elev();
	dme_range = nav.get_range();
	dme_effective_range = kludgeRange(dme_elev, elev, dme_range);
	dme_x = nav.get_x();
	dme_y = nav.get_y();
	dme_z = nav.get_z();
      }
    } else {
      dme_valid = false;
      dme_dist = 0;
    }


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

    navcom1.search();
    navcom2.search();
    adf.search();
    xponder.search();
}
