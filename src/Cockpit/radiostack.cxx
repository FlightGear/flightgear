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


#include <simgear/math/sg_random.h>

#include <Aircraft/aircraft.hxx>
#include <Main/bfi.hxx>
#include <Navaids/ilslist.hxx>
#include <Navaids/navlist.hxx>
#include <Time/event.hxx>

#include "radiostack.hxx"

static int nav1_play_count = 0;
static time_t nav1_last_time = 0;


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
  double factor = ((aircraftElev*METER_TO_FEET) - stationElev) / 5000.0;
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


// model standard VOR/DME/TACAN service volumes as per AIM 1-1-8
static double adjustNavRange( double stationElev, double aircraftElev,
			      double nominalRange )
{
    // assumptions we model the standard service volume, plus
    // ... rather than specifying a cylinder, we model a cone that
    // contains the cylinder.  Then we put an upside down cone on top
    // to model diminishing returns at too-high altitudes.

    // altitude difference
    double alt = ( aircraftElev - stationElev ) * METER_TO_FEET;

    if ( nominalRange < 25.0 + FG_EPSILON ) {
	// Standard Terminal Service Volume
	if ( alt <= 1000.0 ) {
	    return nominalRange;
	} else if ( alt <= 12000.0 ) {
	    return nominalRange + nominalRange * ( alt - 1000.0 ) / 11000.0;
	} else if ( alt <= 18000.0 ) {
	    return nominalRange * 2 * ( 1 - ( alt - 12000.0 ) / 6000.0 );
	} else {
	    return 0.0;
	}
    } else if ( nominalRange < 50.0 + FG_EPSILON ) {
	// Standard Low Altitude Service Volume
	if ( alt <= 1000.0 ) {
	    return nominalRange;
	} else if ( alt <= 18000.0 ) {
	    return nominalRange + nominalRange * ( alt - 1000.0 ) / 17000.0;
	} else if ( alt <= 18000.0 ) {
	    return nominalRange * 2 * ( 1 - ( alt - 18000.0 ) / 9000.0 );
	} else {
	    return 0.0;
	}
    } else {
	// Standard High Altitude Service Volume
	double lc = nominalRange * 0.31;
	double mc = nominalRange * 0.77;
	double hc = nominalRange;

	if ( alt <= 1000.0 ) {
	    return lc;
	} else if ( alt <= 14500.0 ) {
	    return lc + (mc-lc) * ( alt - 1000.0 ) / 13500.0;
	} else if ( alt <= 18000.0 ) {
	    return mc + (hc-mc) * ( alt - 14500.0 ) / 3500.0;
	} else if ( alt <= 45000.0 ) {
	    return hc + hc * ( 1 - fabs(alt - 31500.0) / 13500.0 );
	} else if ( alt <= 60000.0 ) {
	    return mc + (hc-mc) * (60000.0 - alt) / 15000.0;
	} else if ( alt <= 75000.0 ) {
	    return mc * ( 75000.0 - alt ) / 15000.0;
	} else {
	    return 0.0;
	}
    }
}


// periodic radio station search wrapper
static void fgRadioSearch( void ) {
    current_radiostack->search();
}

// Constructor
FGRadioStack::FGRadioStack() {
    nav1_radial = 0.0;
    nav1_dme_dist = 0.0;
    nav2_radial = 0.0;
    nav2_dme_dist = 0.0;
    need_update = true;
    longitudeVal = fgGetValue("/position/longitude");
    latitudeVal = fgGetValue("/position/latitude");
    altitudeVal = fgGetValue("/position/altitude");
}


// Destructor
FGRadioStack::~FGRadioStack() 
{
    unbind();			// FIXME: should be called externally
}


void
FGRadioStack::init ()
{
    morse.init();

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
    fgTie("/radios/nav1/frequencies/selected", this,
	  &FGRadioStack::get_nav1_freq, &FGRadioStack::set_nav1_freq);
    fgTie("/radios/nav1/frequencies/standby", this,
	  &FGRadioStack::get_nav1_alt_freq, &FGRadioStack::set_nav1_alt_freq);
    fgTie("/radios/nav1/radials/selected", this,
	  &FGRadioStack::get_nav1_sel_radial,
	  &FGRadioStack::set_nav1_sel_radial);
    fgTie("/radios/nav1/on", this,
	  &FGRadioStack::get_nav1_on_btn,
	  &FGRadioStack::set_nav1_on_btn);
    fgTie("/radios/nav1/ident", this,
	  &FGRadioStack::get_nav1_ident_btn,
	  &FGRadioStack::set_nav1_ident_btn);

				// Radio outputs
    fgTie("/radios/nav1/radials/actual", this, &FGRadioStack::get_nav1_radial);
    fgTie("/radios/nav1/to-flag", this, &FGRadioStack::get_nav1_to_flag);
    fgTie("/radios/nav1/from-flag", this, &FGRadioStack::get_nav1_from_flag);
    fgTie("/radios/nav1/in-range", this, &FGRadioStack::get_nav1_inrange);
    fgTie("/radios/nav1/dme/distance", this, &FGRadioStack::get_nav1_dme_dist);
    fgTie("/radios/nav1/dme/in-range", this,
	  &FGRadioStack::get_nav1_dme_inrange);
    fgTie("/radios/nav1/heading-needle-deflection", this,
	  &FGRadioStack::get_nav1_heading_needle_deflection);
    fgTie("/radios/nav1/gs-needle-deflection", this,
	  &FGRadioStack::get_nav1_gs_needle_deflection);

				// User inputs
    fgTie("/radios/nav2/frequencies/selected", this,
	  &FGRadioStack::get_nav2_freq, &FGRadioStack::set_nav2_freq);
    fgTie("/radios/nav2/frequencies/standby", this,
	  &FGRadioStack::get_nav2_alt_freq, &FGRadioStack::set_nav2_alt_freq);
    fgTie("/radios/nav2/radials/selected", this,
	  &FGRadioStack::get_nav2_sel_radial,
	  &FGRadioStack::set_nav2_sel_radial);
    fgTie("/radios/nav2/on", this,
	  &FGRadioStack::get_nav2_on_btn,
	  &FGRadioStack::set_nav2_on_btn);
    fgTie("/radios/nav2/ident", this,
	  &FGRadioStack::get_nav2_ident_btn,
	  &FGRadioStack::set_nav2_ident_btn);

				// Radio outputs
    fgTie("/radios/nav2/radials/actual", this, &FGRadioStack::get_nav2_radial);
    fgTie("/radios/nav2/to-flag", this, &FGRadioStack::get_nav2_to_flag);
    fgTie("/radios/nav2/from-flag", this, &FGRadioStack::get_nav2_from_flag);
    fgTie("/radios/nav2/in-range", this, &FGRadioStack::get_nav2_inrange);
    fgTie("/radios/nav2/dme/distance", this, &FGRadioStack::get_nav2_dme_dist);
    fgTie("/radios/nav2/dme/in-range", this,
	  &FGRadioStack::get_nav2_dme_inrange);
    fgTie("/radios/nav2/heading-needle-deflection", this,
	  &FGRadioStack::get_nav2_heading_needle_deflection);
    fgTie("/radios/nav2/gs-needle-deflection", this,
	  &FGRadioStack::get_nav2_gs_needle_deflection);

				// User inputs
    fgTie("/radios/adf/frequencies/selected", this,
	  &FGRadioStack::get_adf_freq, &FGRadioStack::set_adf_freq);
    fgTie("/radios/adf/frequencies/standby", this,
	  &FGRadioStack::get_adf_alt_freq, &FGRadioStack::set_adf_alt_freq);
    fgTie("/radios/adf/rotation", this,
	  &FGRadioStack::get_adf_rotation, &FGRadioStack::set_adf_rotation);
}

void
FGRadioStack::unbind ()
{
    fgUntie("/radios/nav1/frequencies/selected");
    fgUntie("/radios/nav1/frequencies/standby");
    fgUntie("/radios/nav1/radials/actual");
    fgUntie("/radios/nav1/radials/selected");
    fgUntie("/radios/nav1/on");
    fgUntie("/radios/nav1/ident");
    fgUntie("/radios/nav1/to-flag");
    fgUntie("/radios/nav1/from-flag");
    fgUntie("/radios/nav1/in-range");
    fgUntie("/radios/nav1/dme/distance");
    fgUntie("/radios/nav1/dme/in-range");
    fgUntie("/radios/nav1/heading-needle-deflection");
    fgUntie("/radios/nav1/gs-needle-deflection");

    fgUntie("/radios/nav2/frequencies/selected");
    fgUntie("/radios/nav2/frequencies/standby");
    fgUntie("/radios/nav2/radials/actual");
    fgUntie("/radios/nav2/radials/selected");
    fgUntie("/radios/nav2/on");
    fgUntie("/radios/nav2/ident");
    fgUntie("/radios/nav2/to-flag");
    fgUntie("/radios/nav2/from-flag");
    fgUntie("/radios/nav2/in-range");
    fgUntie("/radios/nav2/dme/distance");
    fgUntie("/radios/nav2/dme/in-range");
    fgUntie("/radios/nav2/heading-needle-deflection");
    fgUntie("/radios/nav2/gs-needle-deflection");

    fgUntie("/radios/adf/frequencies/selected");
    fgUntie("/radios/adf/frequencies/standby");
    fgUntie("/radios/adf/rotation");
}

// Update the various nav values based on position and valid tuned in navs
void 
FGRadioStack::update() 
{
    double lon = longitudeVal->getDoubleValue() * DEG_TO_RAD;
    double lat = latitudeVal->getDoubleValue() * DEG_TO_RAD;
    double elev = altitudeVal->getDoubleValue() * FEET_TO_METER;

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
	
	nav1_effective_range = kludgeRange(nav1_elev, elev, nav1_range);
	if ( nav1_loc_dist < nav1_effective_range * NM_TO_METER ) {
	    nav1_inrange = true;
	    
	    // wgs84 heading
	    geo_inverse_wgs_84( elev, lat * RAD_TO_DEG, lon * RAD_TO_DEG, 
				nav1_loclat, nav1_loclon,
				&az1, &az2, &s );
	    // cout << "az1 = " << az1 << " magvar = " << nav1_magvar << endl;
	    nav1_heading = az1 - nav1_magvar;
	    // Alex: nav1_heading = - (az1 - FGBFI::getMagVar() / RAD_TO_DEG);

	    // cout << " heading = " << nav1_heading
	    //      << " dist = " << nav1_dist << endl;
	} else {
	    nav1_inrange = false;
	}

	if ( nav1_loc ) {
	} else {
	    nav1_radial = nav1_sel_radial;
	}
    } else {
	nav1_inrange = false;
	nav1_dme_dist = 0.0;
	// cout << "not picking up vor. :-(" << endl;
    }

    if ( nav1_valid && nav1_inrange ) {
	// play station ident via audio system if on + ident,
	// otherwise turn it off
	if ( nav1_on_btn && nav1_ident_btn ) {
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

	nav2_effective_range = kludgeRange(nav2_elev, elev, nav2_range);
	if ( nav2_loc_dist < nav2_effective_range * NM_TO_METER ) {
	    nav2_inrange = true;

	    // wgs84 heading
	    geo_inverse_wgs_84( elev, lat * RAD_TO_DEG, lon * RAD_TO_DEG, 
				nav2_loclat, nav2_loclon,
				&az1, &az2, &s );
	    nav2_heading = az1 - nav2_magvar;
	    // Alex: nav2_heading = - (az1 - FGBFI::getMagVar() / RAD_TO_DEG);

	    // cout << " heading = " << nav2_heading
	    //      << " dist = " << nav2_dist << endl;
	} else {
	    nav2_inrange = false;
	}

	if ( !nav2_loc ) {
	    nav2_radial = nav2_sel_radial;
	}
    } else {
	nav2_inrange = false;
	// cout << "not picking up vor. :-(" << endl;
    }

    // adf
    if ( adf_valid ) {
	// staightline distance
	station = Point3D( adf_x, adf_y, adf_z );
	adf_dist = aircraft.distance3D( station );

	adf_effective_range = kludgeRange(adf_elev, elev, adf_range);
	if ( adf_dist < adf_effective_range * NM_TO_METER ) {
	    adf_inrange = true;

	    // wgs84 heading
	    geo_inverse_wgs_84( elev, lat * RAD_TO_DEG, lon * RAD_TO_DEG, 
				adf_lat, adf_lon,
				&az1, &az2, &s );
	    adf_heading = az1;

	    // cout << " heading = " << nav2_heading
	    //      << " dist = " << nav2_dist << endl;
	} else {
	    adf_inrange = false;
	}
    } else {
	adf_inrange = false;
    }
}


// Update current nav/adf radio stations based on current postition
void FGRadioStack::search() 
{
    double lon = longitudeVal->getDoubleValue() * DEG_TO_RAD;
    double lat = latitudeVal->getDoubleValue() * DEG_TO_RAD;
    double elev = altitudeVal->getDoubleValue() * FEET_TO_METER;

    // nav1
    FGILS ils;
    FGNav nav;

    static string last_nav1_ident = "";
    static string last_nav2_ident = "";
    static bool last_nav1_vor = false;
    static bool last_nav2_vor = false;
    if ( current_ilslist->query( lon, lat, elev, nav1_freq, &ils ) ) {
	nav1_ident = ils.get_locident();
	if ( last_nav1_ident != nav1_ident || last_nav1_vor ) {
	    nav1_trans_ident = ils.get_trans_ident();
	    last_nav1_ident = nav1_ident;
	    last_nav1_vor = false;
	    nav1_valid = true;
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
	    cout << "offset = " << offset << " play_count = " << nav1_play_count
		 << " nav1_last_time = " << nav1_last_time << " current time = "
		 << globals->get_time_params()->get_cur_time() << endl;

	    cout << "Found an ils station in range" << endl;
	    cout << " id = " << ils.get_locident() << endl;
	}
    } else if ( current_navlist->query( lon, lat, elev, nav1_freq, &nav ) ) {
	nav1_ident = nav.get_ident();
	if ( last_nav1_ident != nav1_ident || !last_nav1_vor ) {
	    last_nav1_ident = nav1_ident;
	    last_nav1_vor = true;
	    nav1_trans_ident = nav.get_trans_ident();
	    nav1_valid = true;
	    nav1_loc = false;
	    nav1_has_dme = nav.get_has_dme();
	    nav1_has_gs = false;
	    nav1_loclon = nav.get_lon();
	    nav1_loclat = nav.get_lat();
	    nav1_elev = nav.get_elev();
	    nav1_magvar = nav.get_magvar();
	    nav1_range = nav.get_range();
	    nav1_effective_range = kludgeRange(nav1_elev, elev, nav1_range);
	    nav1_target_gs = 0.0;
	    nav1_radial = nav1_sel_radial;
	    nav1_x = nav1_dme_x = nav.get_x();
	    nav1_y = nav1_dme_y = nav.get_y();
	    nav1_z = nav1_dme_z = nav.get_z();

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
	    cout << "offset = " << offset << " play_count = " << nav1_play_count
		 << " nav1_last_time = " << nav1_last_time << " current time = "
		 << globals->get_time_params()->get_cur_time() << endl;

	    cout << "Found a vor station in range" << endl;
	    cout << " id = " << nav.get_ident() << endl;
	}
    } else {
	nav1_valid = false;
	nav1_ident = "";
	nav1_radial = 0;
	nav1_dme_dist = 0;
	globals->get_soundmgr()->remove( "nav1-vor-ident" );
	globals->get_soundmgr()->remove( "nav1-dme-ident" );
	cout << "not picking up vor1. :-(" << endl;
    }

    if ( current_ilslist->query( lon, lat, elev, nav2_freq, &ils ) ) {
	nav2_ident = ils.get_locident();
	if ( last_nav2_ident != nav2_ident || last_nav2_vor ) {
	    last_nav2_ident = nav2_ident;
	    last_nav2_vor = false;
	    nav2_trans_ident = ils.get_trans_ident();
	    nav2_valid = true;
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
	    // cout << "Found an ils station in range" << endl;
	    // cout << " id = " << ils.get_locident() << endl;
	}
    } else if ( current_navlist->query( lon, lat, elev, nav2_freq, &nav ) ) {
	nav2_ident = nav.get_ident();
	if ( last_nav2_ident != nav2_ident || !last_nav2_vor ) {
	    last_nav2_ident = nav2_ident;
	    last_nav2_vor = true;
	    nav2_trans_ident = nav.get_trans_ident();
	    nav2_valid = true;
	    nav2_loc = false;
	    nav2_has_dme = nav.get_has_dme();
	    nav2_has_dme = false;
	    nav2_loclon = nav.get_lon();
	    nav2_loclat = nav.get_lat();
	    nav2_elev = nav.get_elev();
	    nav2_magvar = nav.get_magvar();
	    nav2_range = nav.get_range();
	    nav2_effective_range = kludgeRange(nav2_elev, elev, nav2_range);
	    nav2_target_gs = 0.0;
	    nav2_radial = nav2_sel_radial;
	    nav2_x = nav2_dme_x = nav.get_x();
	    nav2_y = nav2_dme_y = nav.get_y();
	    nav2_z = nav2_dme_z = nav.get_z();
	    // cout << "Found a vor station in range" << endl;
	    // cout << " id = " << nav.get_ident() << endl;
	}
    } else {
	nav2_valid = false;
	nav2_ident = "";
	nav2_radial = 0;
	nav2_dme_dist = 0;
	// cout << "not picking up vor2. :-(" << endl;
    }

    // adf
    if ( current_navlist->query( lon, lat, elev, adf_freq, &nav ) ) {
	adf_valid = true;

	adf_lon = nav.get_lon();
	adf_lat = nav.get_lat();
	adf_elev = nav.get_elev();
	adf_range = nav.get_range();
	adf_effective_range = kludgeRange(adf_elev, elev, adf_range);
	adf_x = nav.get_x();
	adf_y = nav.get_y();
	adf_z = nav.get_z();
	// cout << "Found an adf station in range" << endl;
	// cout << " id = " << nav.get_ident() << endl;
    } else {
	adf_valid = false;
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
	double y = (FGBFI::getAltitude() - nav1_elev) * FEET_TO_METER;
	double angle = atan2( y, x ) * RAD_TO_DEG;
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
	double y = (FGBFI::getAltitude() - nav2_elev) * FEET_TO_METER;
	double angle = atan2( y, x ) * RAD_TO_DEG;
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

