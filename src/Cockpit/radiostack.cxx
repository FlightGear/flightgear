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

#include <simgear/compiler.h>
#include <simgear/math/sg_random.h>

#include <Aircraft/aircraft.hxx>
#include <Navaids/ilslist.hxx>
#include <Navaids/mkrbeacons.hxx>
#include <Navaids/navlist.hxx>
#include <Time/event.hxx>

#include "radiostack.hxx"

static int nav1_play_count = 0;
static int nav2_play_count = 0;
static int adf_play_count = 0;
static time_t nav1_last_time = 0;
static time_t nav2_last_time = 0;
static time_t adf_last_time = 0;


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


// periodic radio station search wrapper
static void fgRadioSearch( void ) {
    current_radiostack->search();
}

// Constructor
FGRadioStack::FGRadioStack() :
    lon_node(fgGetNode("/position/longitude-deg", true)),
    lat_node(fgGetNode("/position/latitude-deg", true)),
    alt_node(fgGetNode("/position/altitude-ft", true)),
    need_update(true),
    nav1_radial(0.0),
    nav1_dme_dist(0.0),
    nav2_radial(0.0),
    nav2_dme_dist(0.0)
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
FGRadioStack::~FGRadioStack() 
{
    unbind();			// FIXME: should be called externally

    delete term_tbl;
    delete low_tbl;
    delete high_tbl;
}


void
FGRadioStack::init ()
{
    morse.init();
    beacon.init();
    blink.stamp();

    search();
    update();

    // Search radio database once per second
    global_events.Register( "fgRadioSearch()", fgRadioSearch,
			    fgEVENT::FG_EVENT_READY, 1000);
}

void
FGRadioStack::bind ()
{
				// User inputs
    fgTie("/radios/nav[0]/frequencies/selected-mhz", this,
	  &FGRadioStack::get_nav1_freq, &FGRadioStack::set_nav1_freq);
    fgTie("/radios/nav[0]/standby-mhz", this,
	  &FGRadioStack::get_nav1_alt_freq, &FGRadioStack::set_nav1_alt_freq);
    fgTie("/radios/nav[0]/radials/selected-deg", this,
	  &FGRadioStack::get_nav1_sel_radial,
	  &FGRadioStack::set_nav1_sel_radial);
    fgTie("/radios/nav[0]/volume", this,
	  &FGRadioStack::get_nav1_vol_btn,
	  &FGRadioStack::set_nav1_vol_btn);
    fgTie("/radios/nav[0]/ident", this,
	  &FGRadioStack::get_nav1_ident_btn,
	  &FGRadioStack::set_nav1_ident_btn);

				// Radio outputs
    fgTie("/radios/nav[0]/radials/actual-deg", this, &FGRadioStack::get_nav1_radial);
    fgTie("/radios/nav[0]/to-flag", this, &FGRadioStack::get_nav1_to_flag);
    fgTie("/radios/nav[0]/from-flag", this, &FGRadioStack::get_nav1_from_flag);
    fgTie("/radios/nav[0]/in-range", this, &FGRadioStack::get_nav1_inrange);
    fgTie("/radios/nav[0]/dme/distance-nm", this, &FGRadioStack::get_nav1_dme_dist);
    fgTie("/radios/nav[0]/dme/in-range", this,
	  &FGRadioStack::get_nav1_dme_inrange);
    fgTie("/radios/nav[0]/heading-needle-deflection", this,
	  &FGRadioStack::get_nav1_heading_needle_deflection);
    fgTie("/radios/nav[0]/gs-needle-deflection", this,
	  &FGRadioStack::get_nav1_gs_needle_deflection);

				// User inputs
    fgTie("/radios/nav[1]/frequencies/selected-mhz", this,
	  &FGRadioStack::get_nav2_freq, &FGRadioStack::set_nav2_freq);
    fgTie("/radios/nav[1]/standby-mhz", this,
	  &FGRadioStack::get_nav2_alt_freq, &FGRadioStack::set_nav2_alt_freq);
    fgTie("/radios/nav[1]/radials/selected-deg", this,
	  &FGRadioStack::get_nav2_sel_radial,
	  &FGRadioStack::set_nav2_sel_radial);
    fgTie("/radios/nav[1]/volume", this,
	  &FGRadioStack::get_nav2_vol_btn,
	  &FGRadioStack::set_nav2_vol_btn);
    fgTie("/radios/nav[1]/ident", this,
	  &FGRadioStack::get_nav2_ident_btn,
	  &FGRadioStack::set_nav2_ident_btn);

				// Radio outputs
    fgTie("/radios/nav[1]/radials/actual-deg", this, &FGRadioStack::get_nav2_radial);
    fgTie("/radios/nav[1]/to-flag", this, &FGRadioStack::get_nav2_to_flag);
    fgTie("/radios/nav[1]/from-flag", this, &FGRadioStack::get_nav2_from_flag);
    fgTie("/radios/nav[1]/in-range", this, &FGRadioStack::get_nav2_inrange);
    fgTie("/radios/nav[1]/dme/distance-nm", this, &FGRadioStack::get_nav2_dme_dist);
    fgTie("/radios/nav[1]/dme/in-range", this,
	  &FGRadioStack::get_nav2_dme_inrange);
    fgTie("/radios/nav[1]/heading-needle-deflection", this,
	  &FGRadioStack::get_nav2_heading_needle_deflection);
    fgTie("/radios/nav[1]/gs-needle-deflection", this,
	  &FGRadioStack::get_nav2_gs_needle_deflection);

				// User inputs
    fgTie("/radios/adf/frequencies/selected-khz", this,
	  &FGRadioStack::get_adf_freq, &FGRadioStack::set_adf_freq);
    fgTie("/radios/adf/frequencies/standby-khz", this,
	  &FGRadioStack::get_adf_alt_freq, &FGRadioStack::set_adf_alt_freq);
    fgTie("/radios/adf/rotation-deg", this,
	  &FGRadioStack::get_adf_rotation, &FGRadioStack::set_adf_rotation);
    fgTie("/radios/adf/volume", this,
	  &FGRadioStack::get_adf_vol_btn,
	  &FGRadioStack::set_adf_vol_btn);
    fgTie("/radios/adf/ident", this,
	  &FGRadioStack::get_adf_ident_btn,
	  &FGRadioStack::set_adf_ident_btn);

    fgTie("/radios/marker-beacon/inner", this,
	  &FGRadioStack::get_inner_blink);
    fgTie("/radios/marker-beacon/middle", this,
	  &FGRadioStack::get_middle_blink);
    fgTie("/radios/marker-beacon/outer", this,
	  &FGRadioStack::get_outer_blink);
}

void
FGRadioStack::unbind ()
{
    fgUntie("/radios/nav[0]/frequencies/selected-mhz");
    fgUntie("/radios/nav[0]/standby-mhz");
    fgUntie("/radios/nav[0]/radials/actual-deg");
    fgUntie("/radios/nav[0]/radials/selected-deg");
    fgUntie("/radios/nav[0]/on");
    fgUntie("/radios/nav[0]/ident");
    fgUntie("/radios/nav[0]/to-flag");
    fgUntie("/radios/nav[0]/from-flag");
    fgUntie("/radios/nav[0]/in-range");
    fgUntie("/radios/nav[0]/dme/distance-nm");
    fgUntie("/radios/nav[0]/dme/in-range");
    fgUntie("/radios/nav[0]/heading-needle-deflection");
    fgUntie("/radios/nav[0]/gs-needle-deflection");

    fgUntie("/radios/nav[1]/frequencies/selected-mhz");
    fgUntie("/radios/nav[1]/standby-mhz");
    fgUntie("/radios/nav[1]//radials/actual-deg");
    fgUntie("/radios/nav[1]/radials/selected-deg");
    fgUntie("/radios/nav[1]/on");
    fgUntie("/radios/nav[1]/ident");
    fgUntie("/radios/nav[1]/to-flag");
    fgUntie("/radios/nav[1]/from-flag");
    fgUntie("/radios/nav[1]/in-range");
    fgUntie("/radios/nav[1]/dme/distance-nm");
    fgUntie("/radios/nav[1]/dme/in-range");
    fgUntie("/radios/nav[1]/heading-needle-deflection");
    fgUntie("/radios/nav[1]/gs-needle-deflection");

    fgUntie("/radios/adf/frequencies/selected-khz");
    fgUntie("/radios/adf/frequencies/standby-khz");
    fgUntie("/radios/adf/rotation-deg");
    fgUntie("/radios/adf/on");
    fgUntie("/radios/adf/ident");

    fgUntie("/radios/marker-beacon/inner");
    fgUntie("/radios/marker-beacon/middle");
    fgUntie("/radios/marker-beacon/outer");
}


// model standard VOR/DME/TACAN service volumes as per AIM 1-1-8
double FGRadioStack::adjustNavRange( double stationElev, double aircraftElev,
			      double nominalRange )
{
    // extend out actual usable range to be 1.3x the published safe range
    const double usability_factor = 1.3;

    // assumptions we model the standard service volume, plus
    // ... rather than specifying a cylinder, we model a cone that
    // contains the cylinder.  Then we put an upside down cone on top
    // to model diminishing returns at too-high altitudes.

    // altitude difference
    double alt = ( aircraftElev * SG_METER_TO_FEET - stationElev );
    // cout << "aircraft elev = " << aircraftElev * SG_METER_TO_FEET
    //      << " station elev = " << stationElev << endl;

    if ( nominalRange < 25.0 + SG_EPSILON ) {
	// Standard Terminal Service Volume
	return term_tbl->interpolate( alt ) * usability_factor;
    } else if ( nominalRange < 50.0 + SG_EPSILON ) {
	// Standard Low Altitude Service Volume
	// table is based on range of 40, scale to actual range
	return low_tbl->interpolate( alt ) * nominalRange / 40.0
	    * usability_factor;
    } else {
	// Standard High Altitude Service Volume
	// table is based on range of 130, scale to actual range
	return high_tbl->interpolate( alt ) * nominalRange / 130.0
	    * usability_factor;
    }
}


// model standard ILS service volumes as per AIM 1-1-9
double FGRadioStack::adjustILSRange( double stationElev, double aircraftElev,
				     double offsetDegrees, double distance )
{
    // assumptions we model the standard service volume, plus

    // altitude difference
    // double alt = ( aircraftElev * SG_METER_TO_FEET - stationElev );
    double offset = fabs( offsetDegrees );

    if ( offset < 10 ) {
	return FG_ILS_DEFAULT_RANGE;
    } else if ( offset < 35 ) {
	return 10 + (35 - offset) * (FG_ILS_DEFAULT_RANGE - 10) / 25;
    } else if ( offset < 45 ) {
	return (45 - offset);
    } else {
	return 0;
    }
}


// Update the various nav values based on position and valid tuned in navs
void 
FGRadioStack::update() 
{
    double lon = lon_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double lat = lat_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double elev = alt_node->getDoubleValue() * SG_FEET_TO_METER;

    need_update = false;

    Point3D aircraft = sgGeodToCart( Point3D( lon, lat, elev ) );
    Point3D station;
    double az1, az2, s;

    if ( nav1_valid ) {
	station = Point3D( nav1_x, nav1_y, nav1_z );
	nav1_loc_dist = aircraft.distance3D( station );

	if ( nav1_has_dme ) {
	    // staightline distance
	    station = Point3D( nav1_dme_x, nav1_dme_y, nav1_dme_z );
	    nav1_dme_dist = aircraft.distance3D( station );
	} else {
	    nav1_dme_dist = 0.0;
	}

	if ( nav1_has_gs ) {
	    station = Point3D( nav1_gs_x, nav1_gs_y, nav1_gs_z );
	    nav1_gs_dist = aircraft.distance3D( station );
	} else {
	    nav1_gs_dist = 0.0;
	}
	
	// wgs84 heading
	geo_inverse_wgs_84( elev, lat * SGD_RADIANS_TO_DEGREES, lon * SGD_RADIANS_TO_DEGREES, 
			    nav1_loclat, nav1_loclon,
			    &az1, &az2, &s );
	// cout << "az1 = " << az1 << " magvar = " << nav1_magvar << endl;
	nav1_heading = az1 - nav1_magvar;
	// cout << " heading = " << nav1_heading
	//      << " dist = " << nav1_dist << endl;

	if ( nav1_loc ) {
	    double offset = nav1_heading - nav1_radial;
	    while ( offset < -180.0 ) { offset += 360.0; }
	    while ( offset > 180.0 ) { offset -= 360.0; }
	    // cout << "ils offset = " << offset << endl;
	    nav1_effective_range = adjustILSRange(nav1_elev, elev, offset,
						  nav1_loc_dist * SG_METER_TO_NM );
	} else {
	    nav1_effective_range = adjustNavRange(nav1_elev, elev, nav1_range);
	}
	// cout << "nav1 range = " << nav1_effective_range
	//      << " (" << nav1_range << ")" << endl;

	if ( nav1_loc_dist < nav1_effective_range * SG_NM_TO_METER ) {
	    nav1_inrange = true;
	} else if ( nav1_loc_dist < 2 * nav1_effective_range * SG_NM_TO_METER ) {
	    nav1_inrange = sg_random() < 
		( 2 * nav1_effective_range * SG_NM_TO_METER - nav1_loc_dist ) /
		(nav1_effective_range * SG_NM_TO_METER);
	} else {
	    nav1_inrange = false;
	}

	if ( !nav1_loc ) {
	    nav1_radial = nav1_sel_radial;
	}
    } else {
	nav1_inrange = false;
	nav1_dme_dist = 0.0;
	// cout << "not picking up vor. :-(" << endl;
    }

#ifdef ENABLE_AUDIO_SUPPORT
    if ( nav1_valid && nav1_inrange ) {
	// play station ident via audio system if on + ident,
	// otherwise turn it off
	if ( nav1_vol_btn > 0.1 && nav1_ident_btn ) {
	    FGSimpleSound *sound;
	    sound = globals->get_soundmgr()->find( "nav1-vor-ident" );
	    sound->set_volume( nav1_vol_btn * 0.3 );
	    sound = globals->get_soundmgr()->find( "nav1-dme-ident" );
	    sound->set_volume( nav1_vol_btn * 0.3 );
	    if ( nav1_last_time <
		 globals->get_time_params()->get_cur_time() - 30 ) {
		nav1_last_time = globals->get_time_params()->get_cur_time();
		nav1_play_count = 0;
	    }
	    if ( nav1_play_count < 4 ) {
		// play VOR ident
		if ( !globals->get_soundmgr()->is_playing("nav1-vor-ident") ) {
		    globals->get_soundmgr()->play_once( "nav1-vor-ident" );
		    ++nav1_play_count;
		}
	    } else if ( nav1_play_count < 5 && nav1_has_dme ) {
		// play DME ident
		if ( !globals->get_soundmgr()->is_playing("nav1-vor-ident") &&
		     !globals->get_soundmgr()->is_playing("nav1-dme-ident") ) {
		    globals->get_soundmgr()->play_once( "nav1-dme-ident" );
		    ++nav1_play_count;
		}
	    }
	} else {
	    globals->get_soundmgr()->stop( "nav1-vor-ident" );
	    globals->get_soundmgr()->stop( "nav1-dme-ident" );
	}
    }
#endif

    if ( nav2_valid ) {
	station = Point3D( nav2_x, nav2_y, nav2_z );
	nav2_loc_dist = aircraft.distance3D( station );

	if ( nav2_has_dme ) {
	    // staightline distance
	    station = Point3D( nav2_dme_x, nav2_dme_y, nav2_dme_z );
	    nav2_dme_dist = aircraft.distance3D( station );
	} else {
	    nav2_dme_dist = 0.0;
	}

	if ( nav2_has_gs ) {
	    station = Point3D( nav2_gs_x, nav2_gs_y, nav2_gs_z );
	    nav2_gs_dist = aircraft.distance3D( station );
	} else {
	    nav2_gs_dist = 0.0;
	}

	// wgs84 heading
	geo_inverse_wgs_84( elev, lat * SGD_RADIANS_TO_DEGREES, lon * SGD_RADIANS_TO_DEGREES, 
			    nav2_loclat, nav2_loclon,
			    &az1, &az2, &s );
	nav2_heading = az1 - nav2_magvar;
	// cout << " heading = " << nav2_heading
	//      << " dist = " << nav2_dist << endl;

	if ( nav2_loc ) {
	    double offset = nav2_heading - nav2_radial;
	    while ( offset < -180.0 ) { offset += 360.0; }
	    while ( offset > 180.0 ) { offset -= 360.0; }
	    // cout << "ils offset = " << offset << endl;
	    nav2_effective_range = adjustILSRange(nav2_elev, elev, offset,
						  nav2_loc_dist * SG_METER_TO_NM );
	} else {
	    nav2_effective_range = adjustNavRange(nav2_elev, elev, nav2_range);
	}
	// cout << "nav2 range = " << nav2_effective_range
	//      << " (" << nav2_range << ")" << endl;

	if ( nav2_loc_dist < nav2_effective_range * SG_NM_TO_METER ) {
	    nav2_inrange = true;
	} else if ( nav2_loc_dist < 2 * nav2_effective_range * SG_NM_TO_METER ) {
	    nav2_inrange = sg_random() < 
		( 2 * nav2_effective_range * SG_NM_TO_METER - nav2_loc_dist ) /
		(nav2_effective_range * SG_NM_TO_METER);
	} else {
	    nav2_inrange = false;
	}

	if ( !nav2_loc ) {
	    nav2_radial = nav2_sel_radial;
	}
    } else {
	nav2_inrange = false;
	nav2_dme_dist = 0.0;
	// cout << "not picking up vor. :-(" << endl;
    }

#ifdef ENABLE_AUDIO_SUPPORT
    if ( nav2_valid && nav2_inrange ) {
	// play station ident via audio system if on + ident,
	// otherwise turn it off
	if ( nav2_vol_btn > 0.1 && nav2_ident_btn ) {
	    FGSimpleSound *sound;
	    sound = globals->get_soundmgr()->find( "nav2-vor-ident" );
	    sound->set_volume( nav2_vol_btn * 0.3 );
	    sound = globals->get_soundmgr()->find( "nav2-dme-ident" );
	    sound->set_volume( nav2_vol_btn * 0.3 );
	    if ( nav2_last_time <
		 globals->get_time_params()->get_cur_time() - 30 ) {
		nav2_last_time = globals->get_time_params()->get_cur_time();
		nav2_play_count = 0;
	    }
	    if ( nav2_play_count < 4 ) {
		// play VOR ident
		if ( !globals->get_soundmgr()->is_playing("nav2-vor-ident") ) {
		    globals->get_soundmgr()->play_once( "nav2-vor-ident" );
		    ++nav2_play_count;
		}
	    } else if ( nav2_play_count < 5 && nav2_has_dme ) {
		// play DME ident
		if ( !globals->get_soundmgr()->is_playing("nav2-vor-ident") &&
		     !globals->get_soundmgr()->is_playing("nav2-dme-ident") ) {
		    globals->get_soundmgr()->play_once( "nav2-dme-ident" );
		    ++nav2_play_count;
		}
	    }
	} else {
	    globals->get_soundmgr()->stop( "nav2-vor-ident" );
	    globals->get_soundmgr()->stop( "nav2-dme-ident" );
	}
    }
#endif

    // adf
    if ( adf_valid ) {
	// staightline distance
	station = Point3D( adf_x, adf_y, adf_z );
	adf_dist = aircraft.distance3D( station );

	// wgs84 heading
	geo_inverse_wgs_84( elev, lat * SGD_RADIANS_TO_DEGREES, lon * SGD_RADIANS_TO_DEGREES, 
			    adf_lat, adf_lon,
			    &az1, &az2, &s );
	adf_heading = az1;
	// cout << " heading = " << nav2_heading
	//      << " dist = " << nav2_dist << endl;

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
	    sound->set_volume( adf_vol_btn * 0.3 );
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
void FGRadioStack::search() 
{
    static FGMkrBeacon::fgMkrBeacType last_beacon = FGMkrBeacon::NOBEACON;

    double lon = lon_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double lat = lat_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double elev = alt_node->getDoubleValue() * SG_FEET_TO_METER;

    // nav1
    FGILS ils;
    FGNav nav;

    static string last_nav1_ident = "";
    static string last_nav2_ident = "";
    static string last_adf_ident = "";
    static bool last_nav1_vor = false;
    static bool last_nav2_vor = false;
    if ( current_ilslist->query( lon, lat, elev, nav1_freq, &ils ) ) {
	nav1_ident = ils.get_locident();
	nav1_valid = true;
	if ( last_nav1_ident != nav1_ident || last_nav1_vor ) {
	    nav1_trans_ident = ils.get_trans_ident();
	    last_nav1_ident = nav1_ident;
	    last_nav1_vor = false;
	    nav1_loc = true;
	    nav1_has_dme = ils.get_has_dme();
	    nav1_has_gs = ils.get_has_gs();

	    nav1_loclon = ils.get_loclon();
	    nav1_loclat = ils.get_loclat();
	    nav1_gslon = ils.get_gslon();
	    nav1_gslat = ils.get_gslat();
	    nav1_dmelon = ils.get_dmelon();
	    nav1_dmelat = ils.get_dmelat();
	    nav1_elev = ils.get_gselev();
	    nav1_magvar = 0;
	    nav1_range = FG_ILS_DEFAULT_RANGE;
	    nav1_effective_range = nav1_range;
	    nav1_target_gs = ils.get_gsangle();
	    nav1_radial = ils.get_locheading();
	    while ( nav1_radial <   0.0 ) { nav1_radial += 360.0; }
	    while ( nav1_radial > 360.0 ) { nav1_radial -= 360.0; }
	    nav1_x = ils.get_x();
	    nav1_y = ils.get_y();
	    nav1_z = ils.get_z();
	    nav1_gs_x = ils.get_gs_x();
	    nav1_gs_y = ils.get_gs_y();
	    nav1_gs_z = ils.get_gs_z();
	    nav1_dme_x = ils.get_dme_x();
	    nav1_dme_y = ils.get_dme_y();
	    nav1_dme_z = ils.get_dme_z();

#ifdef ENABLE_AUDIO_SUPPORT
	    if ( globals->get_soundmgr()->exists( "nav1-vor-ident" ) ) {
		globals->get_soundmgr()->remove( "nav1-vor-ident" );
	    }
	    FGSimpleSound *sound;
	    sound = morse.make_ident( nav1_trans_ident, LO_FREQUENCY );
	    sound->set_volume( 0.3 );
	    globals->get_soundmgr()->add( sound, "nav1-vor-ident" );

	    if ( globals->get_soundmgr()->exists( "nav1-dme-ident" ) ) {
		globals->get_soundmgr()->remove( "nav1-dme-ident" );
	    }
	    sound = morse.make_ident( nav1_trans_ident, HI_FREQUENCY );
	    sound->set_volume( 0.3 );
	    globals->get_soundmgr()->add( sound, "nav1-dme-ident" );

	    int offset = (int)(sg_random() * 30.0);
	    nav1_play_count = offset / 4;
	    nav1_last_time = globals->get_time_params()->get_cur_time() -
		offset;
	    // cout << "offset = " << offset << " play_count = "
	    //      << nav1_play_count
	    //      << " nav1_last_time = " << nav1_last_time
	    //      << " current time = "
	    //      << globals->get_time_params()->get_cur_time() << endl;
#endif

	    // cout << "Found an ils station in range" << endl;
	    // cout << " id = " << ils.get_locident() << endl;
	}
    } else if ( current_navlist->query( lon, lat, elev, nav1_freq, &nav ) ) {
	nav1_ident = nav.get_ident();
	nav1_valid = true;
	if ( last_nav1_ident != nav1_ident || !last_nav1_vor ) {
	    last_nav1_ident = nav1_ident;
	    last_nav1_vor = true;
	    nav1_trans_ident = nav.get_trans_ident();
	    nav1_loc = false;
	    nav1_has_dme = nav.get_has_dme();
	    nav1_has_gs = false;
	    nav1_loclon = nav.get_lon();
	    nav1_loclat = nav.get_lat();
	    nav1_elev = nav.get_elev();
	    nav1_magvar = nav.get_magvar();
	    nav1_range = nav.get_range();
	    nav1_effective_range = adjustNavRange(nav1_elev, elev, nav1_range);
	    nav1_target_gs = 0.0;
	    nav1_radial = nav1_sel_radial;
	    nav1_x = nav1_dme_x = nav.get_x();
	    nav1_y = nav1_dme_y = nav.get_y();
	    nav1_z = nav1_dme_z = nav.get_z();

#ifdef ENABLE_AUDIO_SUPPORT
	    if ( globals->get_soundmgr()->exists( "nav1-vor-ident" ) ) {
		globals->get_soundmgr()->remove( "nav1-vor-ident" );
	    }
	    FGSimpleSound *sound;
	    sound = morse.make_ident( nav1_trans_ident, LO_FREQUENCY );
	    sound->set_volume( 0.3 );
	    globals->get_soundmgr()->add( sound, "nav1-vor-ident" );

	    if ( globals->get_soundmgr()->exists( "nav1-dme-ident" ) ) {
		globals->get_soundmgr()->remove( "nav1-dme-ident" );
	    }
	    sound = morse.make_ident( nav1_trans_ident, HI_FREQUENCY );
	    sound->set_volume( 0.3 );
	    globals->get_soundmgr()->add( sound, "nav1-dme-ident" );

	    int offset = (int)(sg_random() * 30.0);
	    nav1_play_count = offset / 4;
	    nav1_last_time = globals->get_time_params()->get_cur_time() -
		offset;
	    // cout << "offset = " << offset << " play_count = "
	    //      << nav1_play_count << " nav1_last_time = "
	    //      << nav1_last_time << " current time = "
	    //      << globals->get_time_params()->get_cur_time() << endl;
#endif

	    // cout << "Found a vor station in range" << endl;
	    // cout << " id = " << nav.get_ident() << endl;
	}
    } else {
	nav1_valid = false;
	nav1_ident = "";
	nav1_radial = 0;
	nav1_dme_dist = 0;
	nav1_trans_ident = "";
	last_nav1_ident = "";
#ifdef ENABLE_AUDIO_SUPPORT
	globals->get_soundmgr()->remove( "nav1-vor-ident" );
	globals->get_soundmgr()->remove( "nav1-dme-ident" );
#endif
	// cout << "not picking up vor1. :-(" << endl;
    }

    if ( current_ilslist->query( lon, lat, elev, nav2_freq, &ils ) ) {
	nav2_ident = ils.get_locident();
	nav2_valid = true;
	if ( last_nav2_ident != nav2_ident || last_nav2_vor ) {
	    last_nav2_ident = nav2_ident;
	    last_nav2_vor = false;
	    nav2_trans_ident = ils.get_trans_ident();
	    nav2_loc = true;
	    nav2_has_dme = ils.get_has_dme();
	    nav2_has_gs = ils.get_has_gs();

	    nav2_loclon = ils.get_loclon();
	    nav2_loclat = ils.get_loclat();
	    nav2_elev = ils.get_gselev();
	    nav2_magvar = 0;
	    nav2_range = FG_ILS_DEFAULT_RANGE;
	    nav2_effective_range = nav2_range;
	    nav2_target_gs = ils.get_gsangle();
	    nav2_radial = ils.get_locheading();
	    while ( nav2_radial <   0.0 ) { nav2_radial += 360.0; }
	    while ( nav2_radial > 360.0 ) { nav2_radial -= 360.0; }
	    nav2_x = ils.get_x();
	    nav2_y = ils.get_y();
	    nav2_z = ils.get_z();
	    nav2_gs_x = ils.get_gs_x();
	    nav2_gs_y = ils.get_gs_y();
	    nav2_gs_z = ils.get_gs_z();
	    nav2_dme_x = ils.get_dme_x();
	    nav2_dme_y = ils.get_dme_y();
	    nav2_dme_z = ils.get_dme_z();

#ifdef ENABLE_AUDIO_SUPPORT
	    if ( globals->get_soundmgr()->exists( "nav2-vor-ident" ) ) {
		globals->get_soundmgr()->remove( "nav2-vor-ident" );
	    }
	    FGSimpleSound *sound;
	    sound = morse.make_ident( nav2_trans_ident, LO_FREQUENCY );
	    sound->set_volume( 0.3 );
	    globals->get_soundmgr()->add( sound, "nav2-vor-ident" );

	    if ( globals->get_soundmgr()->exists( "nav2-dme-ident" ) ) {
		globals->get_soundmgr()->remove( "nav2-dme-ident" );
	    }
	    sound = morse.make_ident( nav2_trans_ident, HI_FREQUENCY );
	    sound->set_volume( 0.3 );
	    globals->get_soundmgr()->add( sound, "nav2-dme-ident" );

	    int offset = (int)(sg_random() * 30.0);
	    nav2_play_count = offset / 4;
	    nav2_last_time = globals->get_time_params()->get_cur_time() -
		offset;
	    // cout << "offset = " << offset << " play_count = "
	    //      << nav2_play_count << " nav2_last_time = "
	    //      << nav2_last_time << " current time = "
	    //      << globals->get_time_params()->get_cur_time() << endl;
#endif

	    // cout << "Found an ils station in range" << endl;
	    // cout << " id = " << ils.get_locident() << endl;
	}
    } else if ( current_navlist->query( lon, lat, elev, nav2_freq, &nav ) ) {
	nav2_ident = nav.get_ident();
	nav2_valid = true;
	if ( last_nav2_ident != nav2_ident || !last_nav2_vor ) {
	    last_nav2_ident = nav2_ident;
	    last_nav2_vor = true;
	    nav2_trans_ident = nav.get_trans_ident();
	    nav2_loc = false;
	    nav2_has_dme = nav.get_has_dme();
	    nav2_has_dme = false;
	    nav2_loclon = nav.get_lon();
	    nav2_loclat = nav.get_lat();
	    nav2_elev = nav.get_elev();
	    nav2_magvar = nav.get_magvar();
	    nav2_range = nav.get_range();
	    nav2_effective_range = adjustNavRange(nav2_elev, elev, nav2_range);
	    nav2_target_gs = 0.0;
	    nav2_radial = nav2_sel_radial;
	    nav2_x = nav2_dme_x = nav.get_x();
	    nav2_y = nav2_dme_y = nav.get_y();
	    nav2_z = nav2_dme_z = nav.get_z();

#ifdef ENABLE_AUDIO_SUPPORT
	    if ( globals->get_soundmgr()->exists( "nav2-vor-ident" ) ) {
		globals->get_soundmgr()->remove( "nav2-vor-ident" );
	    }
	    FGSimpleSound *sound;
	    sound = morse.make_ident( nav2_trans_ident, LO_FREQUENCY );
	    sound->set_volume( 0.3 );
	    globals->get_soundmgr()->add( sound, "nav2-vor-ident" );

	    if ( globals->get_soundmgr()->exists( "nav2-dme-ident" ) ) {
		globals->get_soundmgr()->remove( "nav2-dme-ident" );
	    }
	    sound = morse.make_ident( nav2_trans_ident, HI_FREQUENCY );
	    sound->set_volume( 0.3 );
	    globals->get_soundmgr()->add( sound, "nav2-dme-ident" );

	    int offset = (int)(sg_random() * 30.0);
	    nav2_play_count = offset / 4;
	    nav2_last_time = globals->get_time_params()->get_cur_time() -
		offset;
	    // cout << "offset = " << offset << " play_count = "
	    //      << nav2_play_count << " nav2_last_time = "
	    //      << nav2_last_time << " current time = "
	    //      << globals->get_time_params()->get_cur_time() << endl;
#endif

	    // cout << "Found a vor station in range" << endl;
	    // cout << " id = " << nav.get_ident() << endl;
	}
    } else {
	nav2_valid = false;
	nav2_ident = "";
	nav2_radial = 0;
	nav2_dme_dist = 0;
	nav2_trans_ident = "";
	last_nav2_ident = "";
#ifdef ENABLE_AUDIO_SUPPORT
	globals->get_soundmgr()->remove( "nav2-vor-ident" );
	globals->get_soundmgr()->remove( "nav2-dme-ident" );
#endif
	// cout << "not picking up vor1. :-(" << endl;
    }

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

    // adf
    if ( current_navlist->query( lon, lat, elev, adf_freq, &nav ) ) {
	char freq[128];
#if defined( _MSC_VER )
	_snprintf( freq, 10, "%.0f", adf_freq );
#else
	snprintf( freq, 10, "%.0f", adf_freq );
#endif
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


// return the amount of heading needle deflection, returns a value
// clamped to the range of ( -10 , 10 )
double FGRadioStack::get_nav1_heading_needle_deflection() const {
    double r;

    if ( nav1_inrange ) {
        r = nav1_heading - nav1_radial;
	// cout << "Radial = " << nav1_radial 
	//      << "  Bearing = " << nav1_heading << endl;
    
	while ( r >  180.0 ) { r -= 360.0;}
	while ( r < -180.0 ) { r += 360.0;}
	if ( fabs(r) > 90.0 ) {
	    r = ( r<0.0 ? -r-180.0 : -r+180.0 );
	    if ( nav1_loc ) {
		r = -r;
	    }
	}

	// According to Robin Peel, the ILS is 4x more sensitive than a vor
	if ( nav1_loc ) { r *= 4.0; }
	if ( r < -10.0 ) { r = -10.0; }
	if ( r >  10.0 ) { r = 10.0; }
    } else {
	r = 0.0;
    }

    return r;
}

// return the amount of heading needle deflection, returns a value
// clamped to the range of ( -10 , 10 )
double FGRadioStack::get_nav2_heading_needle_deflection() const {
    double r;

    if ( nav2_inrange ) {
        r = nav2_heading - nav2_radial;
	// cout << "Radial = " << nav2_radial 
	//      << "  Bearing = " << nav2_heading << endl;
    
	while (r> 180.0) r-=360.0;
	while (r<-180.0) r+=360.0;
	if ( fabs(r) > 90.0 )
	    r = ( r<0.0 ? -r-180.0 : -r+180.0 );
	// According to Robin Peel, the ILS is 4x more sensitive than a vor
	if ( nav2_loc ) r *= 4.0;
	if ( r < -10.0 ) r = -10.0;
	if ( r > 10.0 ) r = 10.0;
    } else {
	r = 0.0;
    }

    return r;
}

// return the amount of glide slope needle deflection (.i.e. the
// number of degrees we are off the glide slope * 5.0
double FGRadioStack::get_nav1_gs_needle_deflection() const {
    if ( nav1_inrange && nav1_has_gs ) {
	double x = nav1_gs_dist;
	double y = (fgGetDouble("/position/altitude-ft") - nav1_elev)
            * SG_FEET_TO_METER;
	double angle = atan2( y, x ) * SGD_RADIANS_TO_DEGREES;
	return (nav1_target_gs - angle) * 5.0;
    } else {
	return 0.0;
    }
}


// return the amount of glide slope needle deflection (.i.e. the
// number of degrees we are off the glide slope * 5.0
double FGRadioStack::get_nav2_gs_needle_deflection() const {
    if ( nav2_inrange && nav2_has_gs ) {
	double x = nav2_gs_dist;
	double y = (fgGetDouble("/position/altitude-ft") - nav2_elev)
            * SG_FEET_TO_METER;
	double angle = atan2( y, x ) * SGD_RADIANS_TO_DEGREES;
	return (nav2_target_gs - angle) * 5.0;
    } else {
	return 0.0;
    }
}


/**
 * Return true if the NAV1 TO flag should be active.
 */
bool 
FGRadioStack::get_nav1_to_flag () const
{
  if (nav1_inrange) {
    double offset = fabs(nav1_heading - nav1_radial);
    if (nav1_loc)
      return true;
    else
      return (offset <= 90.0 || offset >= 270.0);
  } else {
    return false;
  }
}


/**
 * Return true if the NAV1 FROM flag should be active.
 */
bool
FGRadioStack::get_nav1_from_flag () const
{
  if (nav1_inrange) {
    double offset = fabs(nav1_heading - nav1_radial);
    if (nav1_loc)
      return false;
    else
      return (offset > 90.0 && offset < 270.0);
  } else {
    return false;
  }
}


/**
 * Return true if the NAV2 TO flag should be active.
 */
bool 
FGRadioStack::get_nav2_to_flag () const
{
  if (nav2_inrange) {
    double offset = fabs(nav2_heading - nav2_radial);
    if (nav2_loc)
      return true;
    else
      return (offset <= 90.0 || offset >= 270.0);
  } else {
    return false;
  }
}


/**
 * Return true if the NAV2 FROM flag should be active.
 */
bool
FGRadioStack::get_nav2_from_flag () const
{
  if (nav2_inrange) {
    double offset = fabs(nav2_heading - nav2_radial);
    if (nav2_loc)
      return false;
    else
      return (offset > 90.0 && offset < 270.0);
  } else {
    return false;
  }
}

