// kr-87.cxx -- class to impliment the King KR 87 Digital ADF
//
// Written by Curtis Olson, started April 2002.
//
// Copyright (C) 2002  Curtis L. Olson - curt@flightgear.org
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
#include <Time/FGEventMgr.hxx>

#include "kr_87.hxx"

#include <string>
SG_USING_STD(string);

static int adf_play_count = 0;
static time_t adf_last_time = 0;


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
FGKR_87::FGKR_87() :
    lon_node(fgGetNode("/position/longitude-deg", true)),
    lat_node(fgGetNode("/position/latitude-deg", true)),
    alt_node(fgGetNode("/position/altitude-ft", true)),
    need_update(true),
    adf_valid(false),
    adf_freq(0.0),
    adf_alt_freq(0.0),
    adf_vol_btn(0.0)
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
FGKR_87::~FGKR_87() 
{
    delete term_tbl;
    delete low_tbl;
    delete high_tbl;
}


void
FGKR_87::init ()
{
    morse.init();

    update(0);			// FIXME: use dt
}

void
FGKR_87::bind ()
{
				// User inputs
    fgTie("/radios/adf/frequencies/selected-khz", this,
	  &FGKR_87::get_adf_freq, &FGKR_87::set_adf_freq);
    fgSetArchivable("/radios/adf/frequencies/selected-khz");
    fgTie("/radios/adf/frequencies/standby-khz", this,
	  &FGKR_87::get_adf_alt_freq, &FGKR_87::set_adf_alt_freq);
    fgSetArchivable("/radios/adf/frequencies/standby-khz");
    fgTie("/radios/adf/rotation-deg", this,
	  &FGKR_87::get_adf_rotation, &FGKR_87::set_adf_rotation);
    fgSetArchivable("/radios/adf/rotation-deg");
    fgTie("/radios/adf/volume", this,
	  &FGKR_87::get_adf_vol_btn,
	  &FGKR_87::set_adf_vol_btn);
    fgSetArchivable("/radios/adf/volume");
    fgTie("/radios/adf/ident", this,
	  &FGKR_87::get_adf_ident_btn,
	  &FGKR_87::set_adf_ident_btn);
    fgSetArchivable("/radios/adf/ident");

                                // calculated values
    fgTie("/radios/adf/inrange", this, &FGKR_87::get_adf_inrange);
    fgTie("/radios/adf/heading", this, &FGKR_87::get_adf_heading);
}

void
FGKR_87::unbind ()
{
    fgUntie("/radios/adf/frequencies/selected-khz");
    fgUntie("/radios/adf/frequencies/standby-khz");
    fgUntie("/radios/adf/rotation-deg");
    fgUntie("/radios/adf/on");
    fgUntie("/radios/adf/ident");
    fgUntie("/radios/adf/inrange");
    fgUntie("/radios/adf/heading");
}


// Update the various nav values based on position and valid tuned in navs
void 
FGKR_87::update(double dt) 
{
    double lon = lon_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double lat = lat_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double elev = alt_node->getDoubleValue() * SG_FEET_TO_METER;

    need_update = false;

    Point3D aircraft = sgGeodToCart( Point3D( lon, lat, elev ) );
    Point3D station;
    double az1, az2, s;

    ////////////////////////////////////////////////////////////////////////
    // ADF
    ////////////////////////////////////////////////////////////////////////

    if ( adf_valid ) {
	// staightline distance
	station = Point3D( adf_x, adf_y, adf_z );
	adf_dist = aircraft.distance3D( station );

	// wgs84 heading
	geo_inverse_wgs_84( elev, lat * SGD_RADIANS_TO_DEGREES,
                            lon * SGD_RADIANS_TO_DEGREES, 
			    adf_lat, adf_lon,
			    &az1, &az2, &s );
	adf_heading = az1;
	// cout << " heading = " << adf_heading
        //      << " dist = " << adf_dist << endl;

	adf_effective_range = kludgeRange(adf_elev, elev, adf_range);
	if ( adf_dist < adf_effective_range * SG_NM_TO_METER ) {
	    adf_inrange = true;
	} else if ( adf_dist < 2 * adf_effective_range * SG_NM_TO_METER ) {
	    adf_inrange = sg_random() < 
		( 2 * adf_effective_range * SG_NM_TO_METER - adf_dist ) /
		(adf_effective_range * SG_NM_TO_METER);
	} else {
	    adf_inrange = false;
	}
    } else {
	adf_inrange = false;
    }

#ifdef ENABLE_AUDIO_SUPPORT
    if ( adf_valid && adf_inrange ) {
	// play station ident via audio system if on + ident,
	// otherwise turn it off
	if ( adf_vol_btn > 0.1 && adf_ident_btn ) {
	    FGSimpleSound *sound;
	    sound = globals->get_soundmgr()->find( "adf-ident" );
            if ( sound != NULL ) {
                sound->set_volume( adf_vol_btn );
            } else {
                SG_LOG( SG_COCKPIT, SG_ALERT, "Can't find adf-ident sound" );
            }
	    if ( adf_last_time <
		 globals->get_time_params()->get_cur_time() - 30 ) {
		adf_last_time = globals->get_time_params()->get_cur_time();
		adf_play_count = 0;
	    }
	    if ( adf_play_count < 4 ) {
		// play ADF ident
		if ( !globals->get_soundmgr()->is_playing("adf-ident") ) {
		    globals->get_soundmgr()->play_once( "adf-ident" );
		    ++adf_play_count;
		}
	    }
	} else {
	    globals->get_soundmgr()->stop( "adf-ident" );
	}
    }
#endif
}


// Update current nav/adf radio stations based on current postition
void FGKR_87::search() 
{
    double lon = lon_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double lat = lat_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double elev = alt_node->getDoubleValue() * SG_FEET_TO_METER;

				// FIXME: the panel should handle this
    FGNav nav;

    static string last_adf_ident = "";

    ////////////////////////////////////////////////////////////////////////
    // ADF.
    ////////////////////////////////////////////////////////////////////////

    if ( current_navlist->query( lon, lat, elev, adf_freq, &nav ) ) {
	char freq[128];
	snprintf( freq, 10, "%.0f", adf_freq );
	adf_ident = freq;
	adf_ident += nav.get_ident();
	// cout << "adf ident = " << adf_ident << endl;
	adf_valid = true;
	if ( last_adf_ident != adf_ident ) {
	    last_adf_ident = adf_ident;

	    adf_trans_ident = nav.get_trans_ident();
	    adf_lon = nav.get_lon();
	    adf_lat = nav.get_lat();
	    adf_elev = nav.get_elev();
	    adf_range = nav.get_range();
	    adf_effective_range = kludgeRange(adf_elev, elev, adf_range);
	    adf_x = nav.get_x();
	    adf_y = nav.get_y();
	    adf_z = nav.get_z();

#ifdef ENABLE_AUDIO_SUPPORT
	    if ( globals->get_soundmgr()->exists( "adf-ident" ) ) {
		globals->get_soundmgr()->remove( "adf-ident" );
	    }
	    FGSimpleSound *sound;
	    sound = morse.make_ident( adf_trans_ident, LO_FREQUENCY );
	    sound->set_volume( 0.3 );
	    globals->get_soundmgr()->add( sound, "adf-ident" );

	    int offset = (int)(sg_random() * 30.0);
	    adf_play_count = offset / 4;
	    adf_last_time = globals->get_time_params()->get_cur_time() -
		offset;
	    // cout << "offset = " << offset << " play_count = "
	    //      << adf_play_count << " adf_last_time = "
	    //      << adf_last_time << " current time = "
	    //      << globals->get_time_params()->get_cur_time() << endl;
#endif

	    // cout << "Found an adf station in range" << endl;
	    // cout << " id = " << nav.get_ident() << endl;
	}
    } else {
	adf_valid = false;
	adf_ident = "";
	adf_trans_ident = "";
#ifdef ENABLE_AUDIO_SUPPORT
	globals->get_soundmgr()->remove( "adf-ident" );
#endif
	last_adf_ident = "";
	// cout << "not picking up adf. :-(" << endl;
    }
}
